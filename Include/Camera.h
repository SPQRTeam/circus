#pragma once
#include <mujoco/mujoco.h>

#include "MujocoContext.h"
#include "Sensor.h"

namespace spqr {
class Camera : public Sensor {
   public:
    Camera(MujocoContext* mujContext, const char* cameraName) : mujContext(mujContext) {
        cam.type = mjCAMERA_FIXED;
        cam.fixedcamid = mj_name2id(mujContext->model, mjOBJ_CAMERA, cameraName);

        if (cam.fixedcamid < 0)
            throw std::runtime_error(std::string("Camera not found: ") + cameraName);
    }

    void doUpdate() override {
    };

    msgpack::object doSerialize(msgpack::zone& z) override {
        throw std::runtime_error("not implemented");
    }

   private:
    MujocoContext* mujContext;
    std::vector<uint8_t> image;
    mjvCamera cam{};
};
}  // namespace spqr
