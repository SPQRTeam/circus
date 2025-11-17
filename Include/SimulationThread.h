#pragma once

#include <mujoco/mujoco.h>

#include <QThread>
#include <atomic>

#include "MujocoContext.h"
#include "Robot.h"

namespace spqr {

class SimulationThread : public QThread {
    Q_OBJECT
   public:
    SimulationThread(const mjModel* model, mjData* data, CameraContext& cameraCtx);
    ~SimulationThread() override;

    void run() override;
    void stop();

   private:
    const mjModel* model_;
    mjData* data_;
    CameraContext& cameraContext_;
    std::atomic<bool> running_;
};

}  // namespace spqr
