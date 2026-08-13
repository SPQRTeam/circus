#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <filesystem>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <mutex>
#include <string>

#include "Container.h"
#include "MujocoContext.h"
#include "sensors/Sensor.h"
#include "ipc/utils.h"

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
            // Reuses the per-robot commands SHM segment as the connect signal: simbridge's
            // BridgeNode constructor synchronously creates this file (via
            // SharedMemoryWriter::configure()) before it dials the socket -- or, in "shm"
            // connect mode, instead of dialing it at all -- so its presence on tmpfs alone
            // proves the robot process is up, with no dedicated connect channel needed.
            connect_shm_path_ = "/dev/shm/circus_ipc/" + name + "_commands.shm";
        }
        virtual ~Robot() = default;
        virtual void bindMujoco(MujocoContext* mujContext) = 0;
        virtual void update() = 0;

        virtual void sendMessageSocket(int fd) final {
            auto message = packMessage();
            msgpack::sbuffer sbuf;
            msgpack::pack(sbuf, message);
            if (sbuf.size() > 0) {
                send_all(fd, sbuf.data(), sbuf.size());
            }
        }

        virtual void receiveMessageSocket(const std::map<std::string, msgpack::object>& message) = 0;

        // Publishes this robot's per-tick state to shared memory, for the case where
        // circus and simbridge run on the same machine. Mirrors packMessage()'s role
        // for the socket path; a robot that only needs the socket path implements
        // this with an empty body.
        virtual void sendMessageSHM() = 0;
        // Reads and applies the latest command from shared memory, if a new one has
        // arrived since the last call. Returns true iff a new command was applied
        // this call. Mirrors receiveMessageSocket()'s role for the socket path.
        virtual bool receiveMessageSHM() = 0;

        // True once this robot's simbridge process has announced itself via shared
        // memory (SimulationThread::waitRobotConnectionsSHM(), the "shm" connect-mode
        // counterpart to the socket handshake in waitRobotConnections()).
        bool hasConnectSignal() const {
            return std::filesystem::exists(connect_shm_path_);
        }

        virtual std::map<std::string, Sensor*> getSensors() = 0;
        virtual void applyCommands() = 0;

    private:
        virtual std::map<std::string, msgpack::object> packMessage() = 0;

        std::string connect_shm_path_;

    public:

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
