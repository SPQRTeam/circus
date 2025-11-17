// Include/Camera.h
#pragma once

#include <mujoco/mujoco.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <msgpack.hpp>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "MujocoContext.h"
#include "Sensor.h"

namespace spqr {

class Camera : public Sensor {
   public:
    Camera(MujocoContext* mujContext, const char* cameraName)
        : Sensor(30.0f), mujContext(mujContext), mujModel(mujContext->model) {
        // Find the camera in the MuJoCo model
        cam.type = mjCAMERA_FIXED;
        cam.fixedcamid = mj_name2id(mujModel, mjOBJ_CAMERA, cameraName);

        if (cam.fixedcamid < 0) {
            throw std::runtime_error(std::string("Camera not found: ") + cameraName);
        }

        // Get camera resolution from model
        w = mujModel->cam_resolution[cam.fixedcamid * 2];
        h = mujModel->cam_resolution[cam.fixedcamid * 2 + 1];

        // Allocate image buffer (RGB, 3 bytes per pixel)
        image.resize(w * h * 3);
        // Create a scene for this camera's viewpoint
        mjv_defaultScene(&scene);
        mjv_makeScene(mujModel, &scene, 1000);

        // Initialize per-camera EGL context
        if (!initializeCameraContext()) {
            throw std::runtime_error("Failed to initialize per-camera rendering context");
        }
    }

    ~Camera() {
        cleanupCameraContext();
        mjv_freeScene(&scene);
    }

    // Called when sensor needs to update (respects frequency limiting)
    void doUpdate() override {
        // Track update frequency per camera instance
        auto now = std::chrono::steady_clock::now();
        updateCount++;

        // Print statistics every second
        auto timeSinceLastPrint
            = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPrintTime).count();
        if (timeSinceLastPrint >= 1000) {
            float actualHz = (updateCount * 1000.0f) / timeSinceLastPrint;
            std::cout << "[Camera " << cam.fixedcamid << "] Updates in last " << timeSinceLastPrint
                      << "ms: " << updateCount << " (actual frequency: " << actualHz << " Hz)" << std::endl;
            updateCount = 0;
            lastPrintTime = now;
        }

        // Make this camera's EGL context current
        if (!eglMakeCurrent(cameraContext.eglDisplay, cameraContext.eglSurface, cameraContext.eglSurface,
                            cameraContext.eglContext)) {
            std::cerr << "Failed to make camera EGL context current" << std::endl;
            return;
        }

        // 1. Define the viewport (rendering area)
        mjrRect viewport = {0, 0, w, h};

        // 2. Update the scene from this camera's viewpoint using snapshot data
        mjData* snapshot = mujContext->getSnapshot();
        mjv_updateScene(mujModel, snapshot, &cameraContext.opt, nullptr, &cam, mjCAT_ALL, &scene);

        // 3. Render the scene to the framebuffer
        mjr_render(viewport, &scene, &cameraContext.mjContext);

        // 4. Read pixels from GPU to CPU
        std::vector<uint8_t> temp(w * h * 3);
        mjr_readPixels(temp.data(), nullptr, viewport, &cameraContext.mjContext);

        // 5. Flip image vertically (OpenGL origin is bottom-left, we want top-left)
        flipImageVertically(temp.data(), w, h, 3);

        // 6. Copy to the image buffer (with lock, for thread safety)
        {
            std::lock_guard<std::mutex> lock(imageMutex);
            image = std::move(temp);
        }

        // Unbind context to allow other cameras to render
        eglMakeCurrent(cameraContext.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    // Called when sending message to Docker container
    msgpack::object doSerialize(msgpack::zone& z) override {
        std::lock_guard<std::mutex> lock(imageMutex);
        std::vector<uint8_t> img_copy(image.begin(), image.end());
        return msgpack::object(img_copy, z);
    }

    // Called from GUI thread to display camera feed
    void copyImageTo(std::vector<uint8_t>& buffer) const {
        std::lock_guard<std::mutex> lock(imageMutex);
        buffer = image;
    }

    int getWidth() const {
        return w;
    }
    int getHeight() const {
        return h;
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
        cameraContext.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (cameraContext.eglDisplay == EGL_NO_DISPLAY) {
            std::cerr << "Failed to get EGL display for camera" << std::endl;
            return false;
        }

        EGLint major, minor;
        if (!eglInitialize(cameraContext.eglDisplay, &major, &minor)) {
            std::cerr << "Failed to initialize EGL for camera" << std::endl;
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
        if (!eglChooseConfig(cameraContext.eglDisplay, configAttribs, &eglConfig, 1, &numConfigs)) {
            std::cerr << "Failed to choose EGL config for camera" << std::endl;
            return false;
        }

        if (!eglBindAPI(EGL_OPENGL_API)) {
            std::cerr << "Failed to bind OpenGL API for camera" << std::endl;
            return false;
        }

        const EGLint contextAttribs[]
            = {EGL_CONTEXT_MAJOR_VERSION, 2, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};

        cameraContext.eglContext
            = eglCreateContext(cameraContext.eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
        if (cameraContext.eglContext == EGL_NO_CONTEXT) {
            std::cerr << "Failed to create EGL context for camera" << std::endl;
            return false;
        }

        const EGLint pbufferAttribs[] = {EGL_WIDTH, w, EGL_HEIGHT, h, EGL_NONE};

        cameraContext.eglSurface
            = eglCreatePbufferSurface(cameraContext.eglDisplay, eglConfig, pbufferAttribs);
        if (cameraContext.eglSurface == EGL_NO_SURFACE) {
            std::cerr << "Failed to create EGL pbuffer surface for camera" << std::endl;
            return false;
        }

        // Make current temporarily to initialize mjrContext
        if (!eglMakeCurrent(cameraContext.eglDisplay, cameraContext.eglSurface, cameraContext.eglSurface,
                            cameraContext.eglContext)) {
            std::cerr << "Failed to make EGL context current for camera initialization" << std::endl;
            return false;
        }

        mjr_defaultContext(&cameraContext.mjContext);
        mjr_makeContext(mujModel, &cameraContext.mjContext, mjFONTSCALE_100);
        mjv_defaultOption(&cameraContext.opt);

        // Unbind
        eglMakeCurrent(cameraContext.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        return true;
    }

    void cleanupCameraContext() {
        if (cameraContext.eglDisplay != EGL_NO_DISPLAY) {
            if (cameraContext.eglContext != EGL_NO_CONTEXT) {
                eglMakeCurrent(cameraContext.eglDisplay, cameraContext.eglSurface, cameraContext.eglSurface,
                               cameraContext.eglContext);
                mjr_freeContext(&cameraContext.mjContext);
                eglMakeCurrent(cameraContext.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            }

            if (cameraContext.eglSurface != EGL_NO_SURFACE) {
                eglDestroySurface(cameraContext.eglDisplay, cameraContext.eglSurface);
            }

            if (cameraContext.eglContext != EGL_NO_CONTEXT) {
                eglDestroyContext(cameraContext.eglDisplay, cameraContext.eglContext);
            }

            eglTerminate(cameraContext.eglDisplay);
        }
    }

    // MuJoCo data (read-only, shared across sensors)
    MujocoContext* mujContext;
    mjModel* mujModel;

    // Per-camera rendering context
    CameraContext cameraContext{};

    // Camera configuration
    int w, h;
    mjvCamera cam{};

    // Per-camera scene (each camera has its own viewpoint)
    mjvScene scene{};

    // Image buffer (written by doUpdate, read by doSerialize/copyImageTo)
    std::vector<uint8_t> image;
    mutable std::mutex imageMutex;

    // Update frequency tracking
    int updateCount = 0;
    std::chrono::steady_clock::time_point lastPrintTime = std::chrono::steady_clock::now();
};

}  // namespace spqr
