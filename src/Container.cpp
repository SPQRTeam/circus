#include "Container.h"

#include <cassert>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <string>

#include "Constants.h"

#include "robots/Robot.h"
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

// for the forward declarations
#include "robots/Robot.h"
#include "Team.h"

namespace spqr {

Container::Container(const std::string& name, const std::string& sockPath) : name(name), curlClient(sockPath), state(ContainerState::NONE) {}

Container::~Container() {
    if (isConnected()) {
        disconnect();
    }
    switch (state) {
        case ContainerState::RUNNING:
            stop();
            remove();
            break;
        case ContainerState::IDLE:
            remove();
            break;
        case ContainerState::REMOVED:
            break;
        case ContainerState::NONE:
            break;
    }
}

void Container::create(const std::shared_ptr<Robot>& robot, const std::string& image, const std::vector<std::string>& binds) {
    nlohmann::json payload;
    payload["Image"] = image;

    payload["HostConfig"] = {{"Binds", binds},
                             {"IpcMode", "host"},
                             {"CapAdd", {"SYS_NICE", "IPC_LOCK"}},
                             {"SecurityOpt", {"seccomp=unconfined"}},
                             {"Ulimits", nlohmann::json::array({{{"Name", "memlock"}, {"Soft", -1}, {"Hard", -1}}})},
                             {"Privileged", true}};

    payload["Env"] = {
        "ROBOT_NAME=" + robot->name,
        "SERVER_IP=172.17.0.1",
        "CIRCUS_PORT=" + std::to_string(frameworkCommunicationPort),
        "TEAM_NUMBER=" + std::to_string(robot->team->number),
        "PLAYER_NUMBER=" + std::to_string(robot->number),
        "TEAM_COLOR=" + robot->colorName
    };

    payload["Tty"] = true;
    payload["OpenStdin"] = true;

    std::string endpoint = create_container_endpoint(name);
    std::string resp_raw = curlClient.request(POST, endpoint, CREATE_OK_RESPONSE, &payload);

    nlohmann::json resp = nlohmann::json::parse(resp_raw);
    if (!resp.contains("Id"))
        throw std::runtime_error("Docker create failed");

    id = resp["Id"];
    state = ContainerState::IDLE;
}

void Container::start() {
    if (state != ContainerState::IDLE)
        throw std::runtime_error("Failed to start container " + name + " which is not IDLE.");

    const std::string endpoint = start_container_endpoint(id);
    curlClient.request(POST, endpoint, START_OK_RESPONSE);
    state = ContainerState::RUNNING;
}

void Container::stop() {
    if (state != ContainerState::RUNNING)
        throw std::runtime_error("Failed to stop container " + name + " which is not RUNNING.");

    const std::string endpoint = stop_container_endpoint(id);
    curlClient.request(POST, endpoint, 0);
    state = ContainerState::IDLE;
}

void Container::remove() {
    if (state != ContainerState::IDLE)
        throw std::runtime_error("Failed to remove container " + name + " which is not IDLE.");

    const std::string endpoint = remove_container_endpoint(id);
    curlClient.request(DELETE, endpoint, DELETE_OK_RESPONSE);
    state = ContainerState::REMOVED;
}

void Container::connect(std::shared_ptr<Team> team, int robotNumber) {
    if (!team->has_subnet()) {
        throw std::runtime_error("Attempted to connect a robot to a team's subnet when it has none.");
    }
    if (isConnected()) {
        throw std::runtime_error("Already connected.");
    }

    nlohmann::json payload;
    payload["Container"] = id;
    payload["EndpointConfig"] = {
        {"IPAddress", UAN_SEVEN_CIU + std::to_string(team->number) + "." + std::to_string(robotNumber + 10)},
        {"Gateway", UAN_SEVEN_CIU + std::to_string(team->number) + ".1"},
        {"IPPrefixLen", 16},
        {"IPAMConfig", {
            {"IPv4Address", UAN_SEVEN_CIU + std::to_string(team->number) + "." + std::to_string(robotNumber + 10)}
        }}
    };

    const std::string endpoint = connect_network_endpoint(team->subnetId);
    curlClient.request(POST, endpoint, CONNECT_OK_RESPONSE, &payload);

    connected_subnet_id = team->subnetId;
}

void Container::disconnect() {
    if (!isConnected()) {
        throw std::runtime_error("Already disconnected.");
    }

    nlohmann::json payload;
    payload["Container"] = id;

    const std::string endpoint = disconnect_network_endpoint(connected_subnet_id);
    curlClient.request(POST, endpoint, DISCONNECT_OK_RESPONSE, &payload);

    connected_subnet_id = "";
}

}  // namespace spqr
