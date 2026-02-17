#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <iostream>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
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
              const Eigen::Vector3d& initOrientation, const std::string& colorName, const std::shared_ptr<Team>& team)
            : name(name), type(type), number(number), initPosition(initPosition), initOrientation(initOrientation), colorName(colorName), team(team) {
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
        virtual std::map<std::string, Sensor*> getSensors() = 0;

        std::string name;
        std::string type;
        uint8_t number;
        Eigen::Vector3d initPosition;
        Eigen::Vector3d initOrientation;  // Euler angles
        std::string colorName;
        std::tuple<int, int, int> color;
        std::unique_ptr<Container> container;
        std::shared_ptr<Team> team;

        msgpack::zone buffer_zone_;

        bool isConnected = false;
        bool isReady = false;
};

}  // namespace spqr
