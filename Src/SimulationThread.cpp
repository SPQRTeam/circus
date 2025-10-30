#include "SimulationThread.h"

#include "Joint.h"
#include "Robot.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data)
    : model_(model), data_(data), running_(true) {}

void SimulationThread::run() {
    double sim_dt = 0.002;  // TODO: config

    if (model_) {
        sim_dt = model_->opt.timestep;
        if (sim_dt <= 0)
            sim_dt = 0.002;
    }

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();
    while (running_) {
        mj_step(model_, data_);
        RobotManager::instance().update();

        next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(sim_dt));
        std::this_thread::sleep_until(next_step_time);

        if (clock::now() > next_step_time)
            next_step_time = clock::now();
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

}  // namespace spqr
