#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "curl/curl.h"

namespace spqr {
class Container {
   public:
    // TODO: is the path always correct for Unix systems??
    explicit Container(const std::string& name, const std::string& sockPath = "/var/run/docker.sock");
    ~Container();

    void create(const std::string& image,
                const std::vector<std::string>& entrypoint = {});

    void start();
    void stop();
    void remove();

   private:
    enum class ContainerState { NONE, IDLE, RUNNING, REMOVED };

    std::string request(const std::string& method, const std::string& endpoint, const long expected_response,
                        const nlohmann::json* body = nullptr);

    std::string id;
    ContainerState state;
    std::string name;

    std::string sockPath;
    CURL* curl_handle;
};
}