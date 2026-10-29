#pragma once

#include <mujoco/mujoco.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "sensors/Sensor.h"

namespace spqr {

// Trivially-copyable mirror of everything doSerialize() sends, for the shared-memory state.
struct CameraInfoData {
        uint32_t height = 0;
        uint32_t width = 0;
        char distortionModel[16] = {};
        double d[5] = {};
        double k[9] = {};
        double r[9] = {};
        double p[12] = {};
        uint32_t binningX = 0;
        uint32_t binningY = 0;
        uint32_t roiXOffset = 0;
        uint32_t roiYOffset = 0;
        uint32_t roiHeight = 0;
        uint32_t roiWidth = 0;
        uint32_t roiDoRectify = 0;
};

// Intrinsics parameters of MuJoCo camera.
// Values are derived once from cam_fovy/cam_resolution (ideal pinhole, no distortion,
// since MuJoCo's OpenGL renderer doesn't model lens distortion) and never change at runtime.
class CameraInfo : public Sensor {
    public:
        CameraInfo(mjModel* mujModel, const char* cameraName) {
            int camId = mj_name2id(mujModel, mjOBJ_CAMERA, cameraName);
            if (camId < 0)
                throw std::runtime_error(std::string("Camera not found: ") + cameraName);

            width_ = mujModel->cam_resolution[2 * camId + 0];
            height_ = mujModel->cam_resolution[2 * camId + 1];

            double fovyRad = mujModel->cam_fovy[camId] * M_PI / 180.0;

            fx_ = (height_ / 2.0) / std::tan(fovyRad / 2.0);
            fy_ = fx_;
            cx_ = width_ / 2.0;
            cy_ = height_ / 2.0;
        }

        void doUpdate() override {}

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::map<std::string, msgpack::object> data;
            data["height"] = msgpack::object(height_, z);
            data["width"] = msgpack::object(width_, z);
            data["distortion_model"] = msgpack::object(std::string("plumb_bob"), z);
            data["d"] = msgpack::object(std::vector<double>{0.0, 0.0, 0.0, 0.0, 0.0}, z);
            data["k"] = msgpack::object(std::vector<double>{fx_, 0.0, cx_, 0.0, fy_, cy_, 0.0, 0.0, 1.0}, z);
            data["r"] = msgpack::object(std::vector<double>{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}, z);
            data["p"] = msgpack::object(std::vector<double>{fx_, 0.0, cx_, 0.0, 0.0, fy_, cy_, 0.0, 0.0, 0.0, 1.0, 0.0}, z);
            data["binning_x"] = msgpack::object(0, z);
            data["binning_y"] = msgpack::object(0, z);

            std::map<std::string, msgpack::object> roi;
            roi["x_offset"] = msgpack::object(0, z);
            roi["y_offset"] = msgpack::object(0, z);
            roi["height"] = msgpack::object(0, z);
            roi["width"] = msgpack::object(0, z);
            roi["do_rectify"] = msgpack::object(false, z);
            data["roi"] = msgpack::object(roi, z);

            return msgpack::object(data, z);
        }

        CameraInfoData toSharedState() const {
            CameraInfoData data;
            data.height = static_cast<uint32_t>(height_);
            data.width = static_cast<uint32_t>(width_);
            std::strncpy(data.distortionModel, "plumb_bob", sizeof(data.distortionModel) - 1);
            const double k[9] = {fx_, 0.0, cx_, 0.0, fy_, cy_, 0.0, 0.0, 1.0};
            const double r[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
            const double p[12] = {fx_, 0.0, cx_, 0.0, 0.0, fy_, cy_, 0.0, 0.0, 0.0, 1.0, 0.0};
            std::memcpy(data.k, k, sizeof(k));
            std::memcpy(data.r, r, sizeof(r));
            std::memcpy(data.p, p, sizeof(p));
            return data;
        }

        int getWidth() const {
            return width_;
        }
        int getHeight() const {
            return height_;
        }
        double getFx() const {
            return fx_;
        }
        double getFy() const {
            return fy_;
        }
        double getCx() const {
            return cx_;
        }
        double getCy() const {
            return cy_;
        }

    private:
        int width_, height_;
        double fx_, fy_, cx_, cy_;
};

}  // namespace spqr
