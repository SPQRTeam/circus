#pragma once
#include <mujoco/mjrender.h>
#include <mujoco/mjvisualize.h>
#include <mujoco/mujoco.h>

#include <msgpack.hpp>
#include <vector>
#include <string>

#include "MujocoContext.h"
#include "sensors/Sensor.h"

#include <fstream>

namespace spqr {

class Camera : public Sensor {
   public:
    Camera(MujocoContext* mujContext, const char* cameraName) : mujContext(mujContext) {
        cam.type = mjCAMERA_FIXED;
        cam.fixedcamid = mj_name2id(mujContext->model, mjOBJ_CAMERA, cameraName);

        if (cam.fixedcamid < 0)
            throw std::runtime_error(std::string("Camera not found: ") + cameraName);

        w = mujContext->model->cam_resolution[2 * cam.fixedcamid + 0];
        h = mujContext->model->cam_resolution[2 * cam.fixedcamid + 1];

        image.resize(w * h * 3);
    }

    void doUpdate() override {
        // mjrRect viewport = {0, 0, w, h};
        // mjv_updateScene(mujContext->model, mujContext->data, &mujContext->opt, nullptr, &cam, mjCAT_ALL, &mujContext->scene);
        // mjr_render(viewport, &mujContext->scene, &mujContext->ctx);
        // mjr_readPixels(image.data(), nullptr, viewport, &mujContext->ctx);
    }

    void updateCamera(int mainWidth, int mainHeight, int index) {
        mjrRect viewport = {mainWidth*index, mainHeight, w, h};
        mjv_updateScene(mujContext->model, mujContext->data, &mujContext->opt, nullptr, &cam, mjCAT_ALL, &mujContext->scene);
        mjr_render(viewport, &mujContext->scene, &mujContext->ctx);
        mjr_readPixels(image.data(), nullptr, viewport, &mujContext->ctx);
    }

    void saveImage(const std::string& filename) {
        
    }

    msgpack::object doSerialize(msgpack::zone& z) override {
        std::vector<uint8_t> img_copy(image.begin(), image.end());
        return msgpack::object(img_copy, z);
    }

   private:
    int w, h;
    MujocoContext* mujContext;
    std::vector<uint8_t> image;
    mjvCamera cam{};
};

}  // namespace spqr
