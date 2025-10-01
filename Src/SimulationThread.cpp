#include "SimulationThread.h"
#include "Robot.h"
#include "Joint.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data)
    : model_(model), data_(data), running_(true) {}

void SimulationThread::run() {
    double sim_dt = 0.002;     // TODO: config

    if (model_) {
        sim_dt = model_->opt.timestep;
        if (sim_dt <= 0)
            sim_dt = 0.002;
    }

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();

    double torque = 5.0;
    auto last_flip = clock::now();

    while (running_){

        // DEBUG!!!!
        K1* rk = (K1*) RobotManager::instance().getRobots()[0].get();
        T1* tk = (T1*) RobotManager::instance().getRobots()[1].get();

        if (std::chrono::duration<double>(clock::now() - last_flip).count() >= 2.0) {
            torque = -torque;           // flip sign
            last_flip = clock::now();
        }

        rk->joints->set_torque({{JointValue::SHOULDER_LEFT_ROLL, torque}});
        tk->joints->set_torque({{JointValue::SHOULDER_LEFT_ROLL, torque}});
        // END DEBUG!!!!

        mj_step(model_, data_);
        next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(sim_dt));
        std::this_thread::sleep_until(next_step_time);

        if (clock::now() > next_step_time) next_step_time = clock::now();
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

}  // namespace spqr
