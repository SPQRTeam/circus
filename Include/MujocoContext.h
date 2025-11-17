#pragma once

#include <EGL/egl.h>
#include <mujoco/mjrender.h>
#include <mujoco/mjvisualize.h>
#include <mujoco/mujoco.h>

#include <string>

namespace spqr {

struct CameraContext {
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;
    mjrContext mjContext{};
    mjvOption opt{};

    ~CameraContext();
    void cleanup();
};

struct MujocoContext {
    mjModel* model = nullptr;
    mjData* data = nullptr;

    // Viewport rendering (used by Qt)
    mjrContext ctx{};
    mjvCamera cam{};
    mjvOption opt{};
    mjvScene scene{};

    // Camera rendering
    CameraContext cameraContext;

    MujocoContext(const std::string& xmlString);
    ~MujocoContext();

    // Copying could potentially lead to freeing the model or data twice.
    // Deleting the copy constructors prevents this.
    MujocoContext(const MujocoContext&) = delete;
    MujocoContext& operator=(const MujocoContext&) = delete;
    MujocoContext& operator=(MujocoContext&& other) noexcept;

   private:
    bool initializeCameraContext();
};
}  // namespace spqr
