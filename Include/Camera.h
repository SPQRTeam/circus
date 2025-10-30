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
    }

    void update() {
        mjv_updateScene(mujContext->model, mujContext->data, &mujContext->opt, nullptr, &cam, mjCAT_ALL,
                        &mujContext->scene);
    };

    msgpack::object serialize(msgpack::zone& z){
        throw std::runtime_error("not implemented");
    }

   private:
    MujocoContext* mujContext;

    mjvCamera cam{};
};
}  // namespace spqr
