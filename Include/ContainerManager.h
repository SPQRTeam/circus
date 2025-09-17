#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "curl/curl.h"

// This class isn't thread safe for now. Avoid multiple instances. We might turn this into a singleton.
class ContainerManager {
   public:
    // TODO: is the path always correct for Unix systems??
    explicit ContainerManager(const std::string& sockPath = "/var/run/docker.sock");

    ~ContainerManager();

    void create(const std::string& name, const std::string& image,
                const std::vector<std::string>& entrypoint = {});

    void start(const std::string& name);
    void stop(const std::string& name);
    void remove(const std::string& name);

   private:
    enum class ContainerState { IDLE, RUNNING, REMOVED };

    struct ContainerInfo {
        std::string id;
        ContainerState state;
    };

    std::string request(const std::string& method, const std::string& endpoint, const long expected_response,
                        const nlohmann::json* body = nullptr);

    std::string sockPath;
    CURL* curl_handle;
    std::unordered_map<std::string, ContainerInfo> containersRegistry;
};
