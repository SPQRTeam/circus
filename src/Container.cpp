#include "Container.h"

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <cassert>
#include <cstdlib>
#include <string>

#include "CircusNetwork.h"
#include "Constants.h"
#include "robots/Robot.h"

// for the forward declarations
#include "Team.h"
#include "robots/Robot.h"

namespace spqr {

Container::Container(const std::string& name, const std::string& sockPath) : name(name), curlClient(sockPath), state(ContainerState::NONE) {}

Container::~Container() {
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
    auto envOrDefault = [](const char* key, const char* fallback) -> std::string {
        const char* value = std::getenv(key);
        return (value && *value) ? std::string(value) : std::string(fallback);
    };

    nlohmann::json payload;
    payload["Image"] = image;

    std::vector<std::string> binds_with_x11 = binds;
    binds_with_x11.push_back("/tmp/.X11-unix:/tmp/.X11-unix:rw");

    std::string xauth_path = envOrDefault("XAUTHORITY", "/root/.Xauthority");
    binds_with_x11.push_back(xauth_path + ":/root/.Xauthority:rw");

    payload["HostConfig"] = {{"Binds", binds_with_x11},
                             {"IpcMode", "host"},
                             {"CapAdd", {"SYS_NICE", "IPC_LOCK"}},
                             {"SecurityOpt", {"seccomp=unconfined"}},
                             {"Ulimits", nlohmann::json::array({{{"Name", "memlock"}, {"Soft", -1}, {"Hard", -1}}})},
                             {"Privileged", true},
                             {"NetworkMode", CIRCUS_NETWORK_NAME}};

    payload["NetworkingConfig"]
        = {{"EndpointsConfig",
            {{CIRCUS_NETWORK_NAME,
              {{"IPAMConfig", {{"IPv4Address", UAN_SEVEN_CIU + std::to_string(robot->team->number) + "." + std::to_string(robot->number + 10)}}}}}}}};

    payload["Env"] = {"ROBOT_NAME=" + robot->name,
                      "SERVER_IP=172.17.0.1",
                      "CIRCUS_PORT=" + std::to_string(frameworkCommunicationPort),
                      "TEAM_NUMBER=" + std::to_string(robot->team->number),
                      "PLAYER_NUMBER=" + std::to_string(robot->number),
                      "TEAM_COLOR=" + robot->colorName,
                      "DISPLAY=" + envOrDefault("DISPLAY", ":0"),
                      "QT_X11_NO_MITSHM=1",
                      "XAUTHORITY=/root/.Xauthority",
                      "XDG_RUNTIME_DIR=/run/user/0",
                      "ROBOT_STACK=booster",
                      "CIRCUS_IMAGE_SHM_DIR=/dev/shm/circus_ipc",
                      "JOYSTICK_DEVICE=" + envOrDefault("JOYSTICK_DEVICE", "/dev/input/js0")};

    payload["Entrypoint"] = {"/bin/bash", "-lc"};
    payload["Cmd"] = {"/app/entrypoint.sh"};

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

}  // namespace spqr
