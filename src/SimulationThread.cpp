#include "SimulationThread.h"

namespace spqr {

SimulationThread::SimulationThread(MujocoContext* mujContext) : mujContext_(mujContext), running_(false) {}

SimulationThread::~SimulationThread() {
    if (running_) {
        stop();
    }
}

void SimulationThread::run() {
    if (!mujContext_ || !mujContext_->model)
        throw std::runtime_error("Cannot start simulation without mujoco model");

    running_ = true;

    double sim_dt = mujContext_->model->opt.timestep;

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();

    while (running_) {
        // Step the simulation (updates live data)
        mj_step(mujContext_->model, mujContext_->data);

        // Create snapshot for sensors to use
        mujContext_->updateSnapshot();

        // Update all robots (which update their sensors using the snapshot)
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
