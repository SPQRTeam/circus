#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <chrono>
#include <map>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Constants.h"
#include "MujocoContext.h"
#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

class Team;  // Forward declaration

class RobotManager {
    public:
        // Singleton class
        static RobotManager& instance() {
            static RobotManager mgr;
            return mgr;
        }

        void registerRobot(std::shared_ptr<Robot> robot);
        std::vector<std::shared_ptr<Robot>> getRobots() const;
        size_t count() const;
        void update();
        void clear();

        void setRobotFd(const std::string& name, int fd);
        int getRobotFd(const std::string& name) const;   // -1 se sconosciuto
        void removeRobotFd(int fd);

        void sendStateMessagesSHM();
        void receiveCommandMessagesSHM();
        void receiveCommandMessagesSocket();

        void initializeSocket(int port);
        void waitRobotConnectionsSocket();
        void waitRobotConnectionsSHM();

        bool areAllRobotsReady() const;
        bool areAllRobotsConnected() const;

        void bindMujoco(MujocoContext* mujContext);

        std::shared_ptr<Robot> create(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& pos,
                                      const Eigen::Vector3d& ori, const std::string& colorName, const std::shared_ptr<Team> team,
                                      const std::string& role = "Striker");

        void startContainers(const std::string& fwkCfgPath = spqr::frameworkConfigPath, const std::string& pathsCfgPath = spqr::pathsConfigPath,
                             const std::string& connectMode = "shm");

        void setAreAllRobotsReadyCallback(std::function<void()> cb);
        void applyCommands();

    private:
        RobotManager() = default;
        ~RobotManager() = default;

        RobotManager(const RobotManager&) = delete;
        RobotManager& operator=(const RobotManager&) = delete;

        std::atomic<bool> serverRunning_ = false;
        std::thread serverThread_;

        mutable std::mutex mutex_;
        std::vector<std::shared_ptr<Robot>> robots_;
        std::map<std::string, int> robotFdMap_;
        int serverFd_ = -1;
        std::vector<pollfd> pollFds_;
        std::unordered_map<int, msgpack::unpacker> unpackers_;
        std::function<void()> areAllRobotsReadyCallback_;

        using RobotCreator = std::function<std::shared_ptr<Robot>(const std::string&, const std::string&, uint8_t, const Eigen::Vector3d&,
                                                                  const Eigen::Vector3d&, const std::string&, const std::shared_ptr<Team>&)>;

        std::unordered_map<std::string, RobotCreator> robotFactory
            = {{"Booster-K1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& colorName,
                                 auto&& team) { return std::make_shared<BoosterK1>(name, type, number, pos, ori, colorName, team); }},
               {"Booster-T1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& colorName, auto&& team) {
                    return std::make_shared<BoosterT1>(name, type, number, pos, ori, colorName, team);
                }}};
};

}  // namespace spqr
