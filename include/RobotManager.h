#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

        void bindMujoco(MujocoContext* mujContext);

        std::shared_ptr<Robot> create(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& pos,
                                      const Eigen::Vector3d& ori, const std::string& colorName, const std::shared_ptr<Team> team);

        void startContainers();

        void startCommunicationServer(int port);
        void stopCommunicationServer();

        void setAreAllRobotsReadyCallback(std::function<void()> cb);

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;

    // Source - https://stackoverflow.com/a
    // Posted by Arun, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-01-12, License - CC BY-SA 3.0
    // TCP Communication, it sends before the size of the message and then the message itself
    ssize_t send_all(int fd, char *buf, size_t len)
    {
        // First, send the size of the message
        uint32_t msg_size = htonl(len);
        ssize_t bytes_sent = send(fd, &msg_size, sizeof(msg_size), 0);
        if (bytes_sent != sizeof(msg_size)) {
            return -1;
        }

        ssize_t total = 0; // how many bytes we've sent
        size_t bytesleft = len; // how many we have left to send
        ssize_t n = 0;
        while(total < len) {
            n = send(fd, buf+total, bytesleft, 0);
            if (n == -1) { 
                /* print/log error details */
                return -1;
            }
            total += n;
            bytesleft -= n;
        }
        return total; 
    }

    void _serverInternal(int port);

    void areAllRobotsReadyWrapper();
    bool areAllRobotsReady() const;

    std::atomic<bool> serverRunning_ = false;
    std::thread serverThread_;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;
    std::function<void()> areAllRobotsReadyCallback_;

        using RobotCreator
            = std::function<std::shared_ptr<Robot>(const std::string&, const std::string&, uint8_t, const Eigen::Vector3d&, const Eigen::Vector3d&,
                                                   const std::string&, const std::shared_ptr<Team>&)>;

        std::unordered_map<std::string, RobotCreator> robotFactory
            = {
                {"Booster-K1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& colorName, auto&& team) {
                    return std::make_shared<BoosterK1>(name, type, number, pos, ori, colorName, team);
                }},
                {"Booster-T1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& colorName, auto&& team) {
                return std::make_shared<BoosterT1>(name, type, number, pos, ori, colorName, team);
                }}
            };
};

}  // namespace spqr
