#include "SimulationThread.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data)
    : model_(model), data_(data), running_(true) {}

void SimulationThread::run() {
    while (running_)
        mj_step(model_, data_);
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

}  // namespace spqr
