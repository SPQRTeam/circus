#pragma once
#include <mujoco/mjrender.h>
#include <mujoco/mjvisualize.h>
#include <mujoco/mujoco.h>

#include <QImage>
#include <QOpenGLFunctions>
#include <cstring>
#include <filesystem>
#include <msgpack.hpp>
#include <string>
#include <vector>

#include "MujocoContext.h"
#include "sensors/Sensor.h"

namespace spqr {

class Camera : public Sensor {
    public:
        Camera(MujocoContext* mujContext, const char* cameraName) : mujContext(mujContext), cameraName_(cameraName) {
            cam.type = mjCAMERA_FIXED;
            cam.fixedcamid = mj_name2id(mujContext->model, mjOBJ_CAMERA, cameraName);

            if (cam.fixedcamid < 0)
                throw std::runtime_error(std::string("Camera not found: ") + cameraName);

            w = mujContext->model->cam_resolution[2 * cam.fixedcamid + 0];
            h = mujContext->model->cam_resolution[2 * cam.fixedcamid + 1];

            image.resize(w * h * 3);

            // Create output directory
            std::filesystem::create_directories("output");
        }

        void doUpdate() override {
            // Camera rendering must happen in the OpenGL thread (paintGL), not here
            // This method is called from the simulation thread which has no GL context
        }

        void renderAndCapture() {
            renderOffscreen();

            frameCounter_++;
            if (frameCounter_ >= 60) {
                // saveImage("output/" + cameraName_ + "_" + std::to_string(imageCounter_++) + ".png");
                frameCounter_ = 0;
            }
        }

        void saveImage(const std::string& filename) {
            QImage qimg(image.data(), w, h, w * 3, QImage::Format_RGB888);
            QImage flipped = qimg.flipped(Qt::Vertical);
            flipped.save(QString::fromStdString(filename));
        }

        const mjvCamera& getCamera() const {
            return cam;
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::vector<uint8_t> img_copy(image.begin(), image.end());
            return msgpack::object(img_copy, z);
        }

    private:
        void renderOffscreen() {
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

            // Update scene with this camera's viewpoint
            mjv_updateScene(mujContext->model, mujContext->data, &mujContext->opt, nullptr, &cam, mjCAT_ALL, &tempScene);

            // Render the scene
            mjr_render(viewport, &tempScene, &mujContext->ctx);

            // Read pixels and resize to camera resolution
            std::vector<uint8_t> tempImage(viewWidth * viewHeight * 3);
            mjr_readPixels(tempImage.data(), nullptr, viewport, &mujContext->ctx);

            // Convert to QImage and resize to desired camera resolution
            QImage qimg(tempImage.data(), viewWidth, viewHeight, viewWidth * 3, QImage::Format_RGB888);
            QImage resized = qimg.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            // Copy resized data back to image buffer
            memcpy(image.data(), resized.constBits(), w * h * 3);

            // Restore window buffer
            mjr_setBuffer(mjFB_WINDOW, &mujContext->ctx);

            // Free temporary scene
            mjv_freeScene(&tempScene);
        }

        int w, h;
        MujocoContext* mujContext;
        std::vector<uint8_t> image;
        mjvCamera cam{};
        std::string cameraName_;
        int frameCounter_ = 0;
        int imageCounter_ = 0;
};

}  // namespace spqr
