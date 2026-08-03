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

#include "Container.h"
#include "MujocoContext.h"
#include "sensors/Sensor.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

class Team;  // Forward declaration

class Robot {
    public:
        Robot(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& initPosition,
              const Eigen::Vector3d& initOrientation, const std::string& colorName, const std::shared_ptr<Team>& team,
              const std::string& role = "Striker")
            : name(name), type(type), number(number), initPosition(initPosition), initOrientation(initOrientation), colorName(colorName), team(team), role(role) {
            if (colorName == "red") {
                color = {130, 36, 51};
            } else if (colorName == "blue") {
                color = {0, 103, 120};
            } else {
                throw std::runtime_error("Team color currently unsupported: " + colorName);
            }
        }
        virtual ~Robot() = default;
        virtual void bindMujoco(MujocoContext* mujContext) = 0;
        virtual void update() = 0;
        virtual void receiveMessage(const std::map<std::string, msgpack::object>& message) = 0;
        virtual std::map<std::string, msgpack::object> sendMessage() = 0;
        // Publishes this robot's per-tick state to shared memory, for the case where
        // circus and simbridge run on the same machine. Mirrors sendMessage()'s role
        // for the socket path; a robot that only needs the socket path implements
        // this with an empty body.
        virtual void publishSharedState() = 0;
        // Reads and applies the latest command from shared memory, if a new one has
        // arrived since the last call. Returns true iff a new command was applied
        // this call. Mirrors receiveMessage()'s role for the socket path.
        virtual bool receiveSharedCommand() = 0;
        virtual std::map<std::string, Sensor*> getSensors() = 0;
        virtual void applyCommands() = 0;

        std::string name;
        std::string type;
        uint8_t number;
        Eigen::Vector3d initPosition;
        Eigen::Vector3d initOrientation;  // Euler angles
        std::string colorName;
        std::string role;
        std::tuple<int, int, int> color;
        std::unique_ptr<Container> container;
        std::shared_ptr<Team> team;

        msgpack::zone buffer_zone_;
        mutable std::mutex mutex_;

        bool isConnected = false;
        bool isReady = false;
};

}  // namespace spqr
