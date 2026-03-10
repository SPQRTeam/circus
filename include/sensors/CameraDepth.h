#pragma once
#include <mujoco/mjrender.h>
#include <mujoco/mjvisualize.h>
#include <mujoco/mujoco.h>

#include <QImage>
#include <QOpenGLFunctions>
#include <msgpack.hpp>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "MujocoContext.h"
#include "sensors/Sensor.h"

namespace spqr {

class CameraDepth : public Sensor {
    public:
        CameraDepth(MujocoContext* mujContext, const char* cameraName) : mujContext(mujContext), cameraName_(cameraName) {
            cam.type = mjCAMERA_FIXED;
            cam.fixedcamid = mj_name2id(mujContext->model, mjOBJ_CAMERA, cameraName);

            if (cam.fixedcamid < 0)
                throw std::runtime_error(std::string("Camera not found: ") + cameraName);

            w = mujContext->model->cam_resolution[2 * cam.fixedcamid + 0];
            h = mujContext->model->cam_resolution[2 * cam.fixedcamid + 1];
            fovy_deg = mujContext->model->cam_fovy[cam.fixedcamid];

            depthNormalized.resize(w * h);
            depth.resize(w * h);
        }

        void doUpdate() override {
            // Camera rendering must happen in the OpenGL thread (paintGL), not here
            // This method is called from the simulation thread which has no GL context
        }

        void render() {
            // Create temporary scene for this camera to avoid modifying the main scene
            mjvScene tempScene;
            mjv_defaultScene(&tempScene);
            mjv_makeScene(mujContext->model, &tempScene, 1000);

            // Get offscreen buffer dimensions
            int offWidth = mujContext->ctx.offWidth;
            int offHeight = mujContext->ctx.offHeight;

            // Calculate viewport to fit camera aspect ratio within offscreen buffer
            float camAspect = (float)w / (float)h;
            float bufAspect = (float)offWidth / (float)offHeight;

            int viewWidth, viewHeight;
            if (camAspect > bufAspect) {
                // Camera is wider - fit to width
                viewWidth = offWidth;
                viewHeight = (int)(offWidth / camAspect);
            } else {
                // Camera is taller - fit to height
                viewHeight = offHeight;
                viewWidth = (int)(offHeight * camAspect);
            }

            mjrRect viewport = {0, 0, viewWidth, viewHeight};
            mjr_setBuffer(mjFB_OFFSCREEN, &mujContext->ctx);

            // Create a copy of visualization options to disable number geoms (group 4) for robot cameras
            mjvOption tempOpt = mujContext->opt;
            tempOpt.geomgroup[4] = 0;  // Hide group 4 (robot number labels) from robot cameras

            // Update scene with this camera's viewpoint
            mjv_updateScene(mujContext->model, mujContext->data, &tempOpt, nullptr, &cam, mjCAT_ALL, &tempScene);

            // Render the scene
            mjr_render(viewport, &tempScene, &mujContext->ctx);

            // Read pixels and resize to camera resolution
            std::vector<float> tempDepth(viewWidth * viewHeight);
            mjr_readPixels(nullptr, tempDepth.data(), viewport, &mujContext->ctx);

            // MuJoCo map.znear/zfar are expressed relative to model extent.
            //DEPTH FIX - MICHELE : Verify.
            // MuJoCo stores vis.map.znear/zfar as fractions of model->stat.extent, not absolute meters.
            // Multiply by stat.extent to convert them to metric near/far distances used by depth linearization.

            const float extent = static_cast<float>(mujContext->model->stat.extent);
            const float znear = static_cast<float>(mujContext->model->vis.map.znear) * extent;
            const float zfar = static_cast<float>(mujContext->model->vis.map.zfar) * extent;
            constexpr float kDepthMaxMeters = 10.0f;  // Keep in sync with SimBridge mono8 decoding.
            float max_u16 = static_cast<float>(std::numeric_limits<uint16_t>::max());

            // Resample offscreen depth to camera resolution and convert to metric depth.
            for (int y = 0; y < h; y++) {
                int srcY = static_cast<int>((static_cast<float>(y) / static_cast<float>(h)) * static_cast<float>(viewHeight));
                srcY = std::clamp(srcY, 0, viewHeight - 1);
                int srcRow = (viewHeight - 1 - srcY) * viewWidth;  // flip y-axis
                int dstRow = y * w;
                for (int x = 0; x < w; x++) {
                    int srcX = static_cast<int>((static_cast<float>(x) / static_cast<float>(w)) * static_cast<float>(viewWidth));
                    srcX = std::clamp(srcX, 0, viewWidth - 1);
                    float z_raw = tempDepth[srcRow + srcX];
                    float z_converted = (znear * zfar) / (zfar - z_raw * (zfar - znear));
                    depthNormalized[dstRow + x] = z_converted;

                    float normalizedDepth = std::clamp(z_converted / kDepthMaxMeters, 0.0f, 1.0f);
                    depth[dstRow + x] = static_cast<uint16_t>(normalizedDepth * max_u16);
                }
            }

            // Restore window buffer
            mjr_setBuffer(mjFB_WINDOW, &mujContext->ctx);

            // Free temporary scene
            mjv_freeScene(&tempScene);
        }

        void saveImage(const std::string& filename) const {
            QImage qimg(reinterpret_cast<const uchar*>(depth.data()), w, h, w * 2, QImage::Format_Grayscale16);
            qimg.save(QString::fromStdString(filename));
        }

        const mjvCamera& getCamera() const {
            return cam;
        }

        const std::vector<float>& getDepthNormalized() const {
            return depthNormalized;
        }

        const std::vector<uint16_t>& getDepth() const {
            std::lock_guard<std::mutex> lock(depthMutex_);
            return depth;
        }

        std::vector<unsigned char> getDepth8bit() const {
            std::lock_guard<std::mutex> lock(depthMutex_);
            std::vector<unsigned char> depth8(depth.size());
            for (size_t i = 0; i < depth.size(); ++i) {
                depth8[i] = static_cast<unsigned char>(depth[i] >> 8);  // Convert uint16 to uint8
            }
            return depth8;
        }

        int getWidth() const {
            return w;
        }

        int getHeight() const {
            return h;
        }

        double getFovyDeg() const {
            return fovy_deg;
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::lock_guard<std::mutex> lock(depthMutex_);
            std::vector<uint16_t> img_copy(depth.begin(), depth.end());
            return msgpack::object(img_copy, z);
        }

    private:
        int w, h;
        double fovy_deg;
        MujocoContext* mujContext;
        mutable std::mutex depthMutex_;
        std::vector<float> depthNormalized;
        std::vector<uint16_t> depth;
        mjvCamera cam{};
        std::string cameraName_;
};

}  // namespace spqr
