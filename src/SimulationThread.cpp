#include "SimulationThread.h"

#include <stdexcept>

#include "RobotManager.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data) : model_(model), data_(data), running_(true), paused_(false) {}

void SimulationThread::run() {
    if (!model_)
        throw std::runtime_error("Cannot start simulation without mujoco model");

    double sim_dt = model_->opt.timestep;

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();
    while (running_) {
        if (!paused_) {
            mj_step(model_, data_);
            RobotManager::instance().update();

            // Emit signal to update QML
            emit stepCompleted();

            next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(sim_dt));
            std::this_thread::sleep_until(next_step_time);

            if (clock::now() > next_step_time)
                next_step_time = clock::now();
        } else {
            // When paused, sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Reset next_step_time when paused to avoid catching up when playd
            next_step_time = clock::now();
        }
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

void SimulationThread::pause() {
    paused_ = true;
}

void SimulationThread::play() {
    paused_ = false;
}

bool SimulationThread::isPaused() {
    return paused_;
}

}  // namespace spqr
