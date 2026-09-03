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
#include "Constants.h"
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
            : name(name),
              type(type),
              number(number),
              initPosition(initPosition),
              initOrientation(initOrientation),
              colorName(colorName),
              team(team),
              role(role) {
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
            receive_shm_path = spqr::sharedMemoryPath + name + "_commands.shm";
            send_shm_path = spqr::sharedMemoryPath + name + "_state.shm";
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

        // publishImages: whether this call should also publish this robot's
        // camera frames (on the robots that have a camera SHM channel), not just
        // its low-rate-independent state. See SimulationThread::run() for the
        // substep-vs-control-step cadence this flag encodes.
        virtual void sendMessageSHM(bool publishImages) = 0;
        virtual bool receiveMessageSHM() = 0;

        // True once this robot's simbridge process has announced itself via shared
        // memory (SimulationThread::waitRobotConnectionsSHM(), the "shm" connect-mode
        // counterpart to the socket handshake in waitRobotConnections()).
        bool hasConnectSignalSHM() const {
            return std::filesystem::exists(receive_shm_path);
        }

        virtual std::map<std::string, Sensor*> getSensors() = 0;
        virtual void applyCommands() = 0;

    private:
        virtual std::map<std::string, msgpack::object> packMessage() = 0;

    protected:
        std::string receive_shm_path;
        std::string send_shm_path;

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
