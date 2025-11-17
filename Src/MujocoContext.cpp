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
    if (!initializeCameraContext()) {
        throw std::runtime_error("Failed to initialize camera context");
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
        cameraContext = std::move(other.cameraContext);

        other.model = nullptr;
        other.data = nullptr;
        other.dataSnapshot = nullptr;
    }
    return *this;
}

bool MujocoContext::initializeCameraContext() {
    auto& ctx = cameraContext;

    ctx.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx.eglDisplay == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return false;
    }

    EGLint major, minor;
    if (!eglInitialize(ctx.eglDisplay, &major, &minor)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return false;
    }

    const EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                                    EGL_PBUFFER_BIT,
                                    EGL_RENDERABLE_TYPE,
                                    EGL_OPENGL_BIT,
                                    EGL_RED_SIZE,
                                    8,
                                    EGL_GREEN_SIZE,
                                    8,
                                    EGL_BLUE_SIZE,
                                    8,
                                    EGL_DEPTH_SIZE,
                                    24,
                                    EGL_NONE};

    EGLConfig eglConfig;
    EGLint numConfigs;
    if (!eglChooseConfig(ctx.eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        return false;
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        std::cerr << "Failed to bind OpenGL API" << std::endl;
        return false;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 2, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};

    ctx.eglContext = eglCreateContext(ctx.eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (ctx.eglContext == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        return false;
    }

    const EGLint pbufferAttribs[] = {EGL_WIDTH, 1920, EGL_HEIGHT, 1080, EGL_NONE};

    ctx.eglSurface = eglCreatePbufferSurface(ctx.eglDisplay, eglConfig, pbufferAttribs);
    if (ctx.eglSurface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL pbuffer surface" << std::endl;
        return false;
    }

    // Make current temporarily to initialize mjrContext
    if (!eglMakeCurrent(ctx.eglDisplay, ctx.eglSurface, ctx.eglSurface, ctx.eglContext)) {
        std::cerr << "Failed to make EGL context current" << std::endl;
        return false;
    }

    mjr_defaultContext(&ctx.mjContext);
    mjr_makeContext(model, &ctx.mjContext, mjFONTSCALE_100);
    mjv_defaultOption(&ctx.opt);

    // Unbind
    eglMakeCurrent(ctx.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    return true;
}

CameraContext::~CameraContext() {
    cleanup();
}

void CameraContext::cleanup() {
    if (eglDisplay != EGL_NO_DISPLAY) {
        if (eglContext != EGL_NO_CONTEXT) {
            eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
            mjr_freeContext(&mjContext);
            eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }

        if (eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(eglDisplay, eglSurface);
        }

        if (eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay, eglContext);
        }

        eglTerminate(eglDisplay);
    }
}

void MujocoContext::updateSnapshot() {
    std::lock_guard<std::mutex> lock(snapshotMutex);
    mj_copyData(dataSnapshot, model, data);
}

}  // namespace spqr
