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
    SimulationThread(MujocoContext* mujContext);
    ~SimulationThread() override;

    void run() override;
    void stop();

   private:
    MujocoContext* mujContext_;
    std::atomic<bool> running_;
};

}  // namespace spqr
