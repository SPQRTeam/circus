#include "MujocoContext.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace spqr {

MujocoContext::MujocoContext(const std::string& xmlString) {
    char error[1024] = {0};
    std::filesystem::current_path(PROJECT_ROOT);

    std::unique_ptr<mjSpec, std::function<void(mjSpec*)>> spec(
        mj_parseXMLString(xmlString.c_str(), nullptr, error, sizeof(error)),
        [](mjSpec* s) { mj_deleteSpec(s); });

    if (!spec) {
        throw std::runtime_error(std::string("Failed to parse the generated XML. ") + error);
    }

    model = mj_compile(spec.get(), nullptr);
    if (!model) {
        throw std::runtime_error(std::string("Failed to compile mujoco specs. ") + mjs_getError(spec.get()));
    }

    data = mj_makeData(model);
    dataSnapshot = mj_makeData(model);  // Create snapshot copy

    // Viewport rendering setup
    mjv_defaultOption(&opt);
    mjv_defaultCamera(&cam);
    mjv_makeScene(model, &scene, 10000);

    // Camera rendering setup
    if (!initializeSharedEGL()) {
        throw std::runtime_error("Failed to initialize shared EGL context");
    }
}

MujocoContext::~MujocoContext() {
    if (dataSnapshot)
        mj_deleteData(dataSnapshot);
    if (data)
        mj_deleteData(data);
    if (model)
        mj_deleteModel(model);
    mjv_freeScene(&scene);
}

MujocoContext& MujocoContext::operator=(MujocoContext&& other) noexcept {
    if (this != &other) {
        if (dataSnapshot)
            mj_deleteData(dataSnapshot);
        if (data)
            mj_deleteData(data);
        if (model)
            mj_deleteModel(model);
        mjv_freeScene(&scene);

        model = other.model;
        data = other.data;
        dataSnapshot = other.dataSnapshot;
        cam = other.cam;
        opt = other.opt;
        scene = other.scene;
        sharedEGL = std::move(other.sharedEGL);

        other.model = nullptr;
        other.data = nullptr;
        other.dataSnapshot = nullptr;
    }
    return *this;
}

bool MujocoContext::initializeSharedEGL() {
    sharedEGL.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (sharedEGL.eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(sharedEGL.eglDisplay, &major, &minor)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return false;
    }

    return true;
}

SharedEGLDisplay::~SharedEGLDisplay() {
    cleanup();
}

void SharedEGLDisplay::cleanup() {
    if (eglDisplay != EGL_NO_DISPLAY) {
        eglTerminate(eglDisplay);
        eglDisplay = EGL_NO_DISPLAY;
    }
}

void MujocoContext::updateSnapshot() {
    std::lock_guard<std::mutex> lock(snapshotMutex);
    mj_copyData(dataSnapshot, model, data);
}

}  // namespace spqr
