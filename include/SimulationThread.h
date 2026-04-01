#pragma once

#include <mujoco/mujoco.h>

#include <QThread>
#include <vector>
#include <mutex>
#include <map>

#include "robots/Robot.h"


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
        void initializeSocket(int port);
        void waitRobotConnections();
        void receiveCommandMessages();


    signals:
        void stepCompleted();
        void maxSimulationTimeReached();
        void allRobotsReadySignal();

    private:
        const mjModel* model_;
        mjData* data_;
        std::atomic<bool> running_;
        std::atomic<bool> paused_;
        int maxSimulationTime_ = -1;  // -1 means no limit
        std::map<std::string, int> entity_fd_map;

        std::function<void()> areAllRobotsReadyCallback_;

        // Socket for communication stuff
        int server_fd;
        std::vector<std::shared_ptr<Robot>> robots_;
        std::vector<pollfd> fds;
        mutable std::mutex mutex_;

        ssize_t send_all(int fd, char* buf, size_t len);

        void areAllRobotsReadyWrapper();
        bool areAllRobotsReady() const;
        bool areAllRobotsConnected() const;



};

}  // namespace spqr
