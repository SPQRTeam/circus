// Include/Camera.h
#pragma once

#include <mujoco/mujoco.h>

#include <chrono>
#include <cstring>
#include <msgpack.hpp>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <iostream>

#include "MujocoContext.h"
#include "Sensor.h"

namespace spqr {

class Camera : public Sensor {
   public:
    Camera(MujocoContext* mujContext, const char* cameraName)
        : Sensor(30.0f),
          model_(mujContext->model),
          data_(mujContext->data) {
        // Find the camera in the MuJoCo model
        cam_.type = mjCAMERA_FIXED;
        cam_.fixedcamid = mj_name2id(model_, mjOBJ_CAMERA, cameraName);

        if (cam_.fixedcamid < 0) {
            throw std::runtime_error(std::string("Camera not found: ") + cameraName);
        }

        // Get camera resolution from model
        w_ = model_->cam_resolution[cam_.fixedcamid * 2];
        h_ = model_->cam_resolution[cam_.fixedcamid * 2 + 1];

        // Allocate image buffer (RGB, 3 bytes per pixel)
        image_.resize(w_ * h_ * 3);

        // Create a scene for this camera's viewpoint
        mjv_defaultScene(&scene_);
        mjv_makeScene(model_, &scene_, 1000);

        // Initialize per-camera EGL context
        if (!initializeCameraContext()) {
            throw std::runtime_error("Failed to initialize per-camera rendering context");
        }
    }

    ~Camera() {
        cleanupCameraContext();
        mjv_freeScene(&scene_);
    }

    // Called when sensor needs to update (respects frequency limiting)
    void doUpdate() override {
        // Track update frequency per camera instance
        auto now = std::chrono::steady_clock::now();
        updateCount_++;

        // Print statistics every second
        auto timeSinceLastPrint = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPrintTime_).count();
        if (timeSinceLastPrint >= 1000) {
            float actualHz = (updateCount_ * 1000.0f) / timeSinceLastPrint;
            std::cout << "[Camera " << cam_.fixedcamid << "] Updates in last " << timeSinceLastPrint << "ms: " << updateCount_
                      << " (actual frequency: " << actualHz << " Hz)" << std::endl;
            updateCount_ = 0;
            lastPrintTime_ = now;
        }

        // Make this camera's EGL context current
        if (!eglMakeCurrent(cameraCtx_.eglDisplay, cameraCtx_.eglSurface, cameraCtx_.eglSurface, cameraCtx_.eglContext)) {
            std::cerr << "Failed to make camera EGL context current" << std::endl;
            return;
        }

        // 1. Define the viewport (rendering area)
        mjrRect viewport = {0, 0, w_, h_};

        // 2. Update the scene from this camera's viewpoint
        mjv_updateScene(model_, data_, &cameraCtx_.opt, nullptr, &cam_, mjCAT_ALL, &scene_);

        // 3. Render the scene to the framebuffer
        mjr_render(viewport, &scene_, &cameraCtx_.mjContext);

        // 4. Read pixels from GPU to CPU
        std::vector<uint8_t> temp(w_ * h_ * 3);
        mjr_readPixels(temp.data(), nullptr, viewport, &cameraCtx_.mjContext);

        // 5. Flip image vertically (OpenGL origin is bottom-left, we want top-left)
        flipImageVertically(temp.data(), w_, h_, 3);

        // 6. Copy to the image buffer (with lock, for thread safety)
        {
            std::lock_guard<std::mutex> lock(imageMutex_);
            image_ = std::move(temp);
        }

        // Unbind context to allow other cameras to render
        eglMakeCurrent(cameraCtx_.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    // Called when sending message to Docker container
    msgpack::object doSerialize(msgpack::zone& z) override {
        std::lock_guard<std::mutex> lock(imageMutex_);
        std::vector<uint8_t> img_copy(image_.begin(), image_.end());
        return msgpack::object(img_copy, z);
    }

    // Called from GUI thread to display camera feed
    void copyImageTo(std::vector<uint8_t>& buffer) const {
        std::lock_guard<std::mutex> lock(imageMutex_);
        buffer = image_;
    }

    int getWidth() const {
        return w_;
    }
    int getHeight() const {
        return h_;
    }

   private:
    void flipImageVertically(uint8_t* data, int width, int height, int channels) {
        int row_size = width * channels;
        std::vector<uint8_t> temp_row(row_size);

        for (int y = 0; y < height / 2; ++y) {
            uint8_t* top = data + y * row_size;
            uint8_t* bottom = data + (height - 1 - y) * row_size;

            std::memcpy(temp_row.data(), top, row_size);
            std::memcpy(top, bottom, row_size);
            std::memcpy(bottom, temp_row.data(), row_size);
        }
    }

    bool initializeCameraContext() {
        cameraCtx_.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (cameraCtx_.eglDisplay == EGL_NO_DISPLAY) {
            std::cerr << "Failed to get EGL display for camera" << std::endl;
            return false;
        }

        EGLint major, minor;
        if (!eglInitialize(cameraCtx_.eglDisplay, &major, &minor)) {
            std::cerr << "Failed to initialize EGL for camera" << std::endl;
            return false;
        }

        const EGLint configAttribs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                                        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE, 24,
                                        EGL_NONE};

        EGLConfig eglConfig;
        EGLint numConfigs;
        if (!eglChooseConfig(cameraCtx_.eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
            std::cerr << "Failed to choose EGL config for camera" << std::endl;
            return false;
        }

        if (!eglBindAPI(EGL_OPENGL_API)) {
            std::cerr << "Failed to bind OpenGL API for camera" << std::endl;
            return false;
        }

        const EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION, 2, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};

        cameraCtx_.eglContext = eglCreateContext(cameraCtx_.eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
        if (cameraCtx_.eglContext == EGL_NO_CONTEXT) {
            std::cerr << "Failed to create EGL context for camera" << std::endl;
            return false;
        }

        const EGLint pbufferAttribs[] = {EGL_WIDTH, w_, EGL_HEIGHT, h_, EGL_NONE};

        cameraCtx_.eglSurface = eglCreatePbufferSurface(cameraCtx_.eglDisplay, eglConfig, pbufferAttribs);
        if (cameraCtx_.eglSurface == EGL_NO_SURFACE) {
            std::cerr << "Failed to create EGL pbuffer surface for camera" << std::endl;
            return false;
        }

        // Make current temporarily to initialize mjrContext
        if (!eglMakeCurrent(cameraCtx_.eglDisplay, cameraCtx_.eglSurface, cameraCtx_.eglSurface, cameraCtx_.eglContext)) {
            std::cerr << "Failed to make EGL context current for camera initialization" << std::endl;
            return false;
        }

        mjr_defaultContext(&cameraCtx_.mjContext);
        mjr_makeContext(model_, &cameraCtx_.mjContext, mjFONTSCALE_100);
        mjv_defaultOption(&cameraCtx_.opt);

        // Unbind
        eglMakeCurrent(cameraCtx_.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        return true;
    }

    void cleanupCameraContext() {
        if (cameraCtx_.eglDisplay != EGL_NO_DISPLAY) {
            if (cameraCtx_.eglContext != EGL_NO_CONTEXT) {
                eglMakeCurrent(cameraCtx_.eglDisplay, cameraCtx_.eglSurface, cameraCtx_.eglSurface, cameraCtx_.eglContext);
                mjr_freeContext(&cameraCtx_.mjContext);
                eglMakeCurrent(cameraCtx_.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }

            if (cameraCtx_.eglSurface != EGL_NO_SURFACE) {
                eglDestroySurface(cameraCtx_.eglDisplay, cameraCtx_.eglSurface);
            }

            if (cameraCtx_.eglContext != EGL_NO_CONTEXT) {
                eglDestroyContext(cameraCtx_.eglDisplay, cameraCtx_.eglContext);
            }

            eglTerminate(cameraCtx_.eglDisplay);
        }
    }

    // MuJoCo data (read-only, shared across sensors)
    const mjModel* model_;
    mjData* data_;

    // Per-camera rendering context
    CameraContext cameraCtx_{};

    // Camera configuration
    int w_, h_;
    mjvCamera cam_{};

    // Per-camera scene (each camera has its own viewpoint)
    mjvScene scene_{};

    // Image buffer (written by doUpdate, read by doSerialize/copyImageTo)
    std::vector<uint8_t> image_;
    mutable std::mutex imageMutex_;

    // Update frequency tracking
    int updateCount_ = 0;
    std::chrono::steady_clock::time_point lastPrintTime_ = std::chrono::steady_clock::now();
};

}  // namespace spqr
