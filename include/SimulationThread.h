#pragma once

#include <mujoco/mujoco.h>

#include <QThread>

namespace spqr {

class SimulationThread : public QThread {
        Q_OBJECT
    public:
        SimulationThread(const mjModel* model, mjData* data);
        void run() override;
        void stop();
        void pause();
        void play();
        bool isPaused();
        void setMaxSimulationTime(int maxTime);

    signals:
        void stepCompleted();
        void maxSimulationTimeReached();

    private:
        const mjModel* model_;
        mjData* data_;
        std::atomic<bool> running_;
        std::atomic<bool> paused_;
        int maxSimulationTime_ = -1;  // -1 means no limit
};

}  // namespace spqr
