#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "DockerREST.h"

namespace spqr {

class Robot;  // forwards declaration
class Team;  // Forward declaration

enum class ContainerState { NONE, IDLE, RUNNING, REMOVED };
class Container {
    public:
        // TODO: is the path always correct for Unix systems??
        Container(const std::string& name, const std::string& sockPath = "/var/run/docker.sock");
        ~Container();

        void create(const std::shared_ptr<Robot>& robot, const std::string& image, const std::vector<std::string>& binds);

        void start();
        void stop();
        void remove();

        void connect(std::shared_ptr<Team> team, int robotNumber);
        void disconnect();

        inline bool isConnected() {
            return connected_subnet_id.size() > 0;
        }

        std::string getId() const {
            return id;
        }

    private:
        std::string id;
        ContainerState state;
        std::string name;
        std::string connected_subnet_id = "";

        CURLClient curlClient;
};
}  // namespace spqr
