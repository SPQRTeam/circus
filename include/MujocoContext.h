#pragma once

#include <EGL/egl.h>
#include <mujoco/mjrender.h>
#include <mujoco/mjvisualize.h>
#include <mujoco/mujoco.h>

#include <mutex>
#include <string>

namespace spqr {

// RAII guard that holds a lock while accessing the snapshot
class SnapshotGuard {
   public:
    SnapshotGuard(std::mutex& mutex, mjData* data) : lock_(mutex), data_(data) {}

    // Get raw pointer to snapshot data
    mjData* get() const {
        return data_;
    }

    // Allow pointer-like access
    mjData* operator->() const {
        return data_;
    }

    // Prevent copying and moving (lock_guard is non-copyable and non-movable)
    SnapshotGuard(const SnapshotGuard&) = delete;
    SnapshotGuard& operator=(const SnapshotGuard&) = delete;
    SnapshotGuard(SnapshotGuard&&) = delete;
    SnapshotGuard& operator=(SnapshotGuard&&) = delete;

   private:
    std::lock_guard<std::mutex> lock_;
    mjData* data_;
};

// Shared EGL display for all cameras (owned by MujocoContext)
struct SharedEGLDisplay {
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;

    ~SharedEGLDisplay();
    void cleanup();
};

// Per-camera rendering context (owned by each Camera instance)
struct CameraContext {
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;  // Borrowed from SharedEGLDisplay
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;
    mjrContext mjContext{};
    mjvOption opt{};
};

struct MujocoContext {
    mjModel* model = nullptr;
    mjData* data = nullptr;          // Live simulation data (written by simulation thread)
    mjData* dataSnapshot = nullptr;  // Read-only snapshot for sensors (read by worker threads)

    // Viewport rendering (used by Qt)
    mjrContext ctx{};
    mjvCamera cam{};
    mjvOption opt{};
    mjvScene scene{};

    // Shared EGL display for camera rendering
    SharedEGLDisplay sharedEGL;

    // Thread synchronization for snapshot updates
    mutable std::mutex snapshotMutex;

    MujocoContext(const std::string& xmlString);
    ~MujocoContext();

    // Update the snapshot with current simulation state
    // Called by simulation thread after mj_step
    void updateSnapshot();

    // Get thread-safe read access to snapshot
    // Returns a guard that holds the lock while the snapshot is being accessed
    SnapshotGuard getSnapshot() const {
        return SnapshotGuard(snapshotMutex, dataSnapshot);
    }

    // Copying could potentially lead to freeing the model or data twice.
    // Deleting the copy constructors prevents this.
    MujocoContext(const MujocoContext&) = delete;
    MujocoContext& operator=(const MujocoContext&) = delete;
    MujocoContext& operator=(MujocoContext&& other) noexcept;

   private:
    bool initializeSharedEGL();
};
}  // namespace spqr
