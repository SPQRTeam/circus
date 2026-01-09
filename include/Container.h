#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "curl/curl.h"

namespace spqr {

enum class ContainerState { NONE, IDLE, RUNNING, REMOVED };
class Container {
    public:
        // TODO: is the path always correct for Unix systems??
        Container(const std::string& name, const std::string& sockPath = "/var/run/docker.sock");
        ~Container();

        void create(const std::string& robot_name, const std::string& image, const std::vector<std::string>& binds);

        void start();
        void stop();
        void remove();

        // Terminal/exec methods
        std::string createExec(const std::vector<std::string>& cmd, bool attachStdin = true, bool attachStdout = true, bool attachStderr = true,
                               bool tty = true);
        std::string startExec(const std::string& exec_id, bool tty = true);
        void writeToExec(const std::string& data);

        std::string getId() const {
            return id;
        }

    private:
        std::string request(const std::string& method, const std::string& endpoint, const long expected_response,
                            const nlohmann::json* body = nullptr);

        std::string id;
        ContainerState state;
        std::string name;

        std::string sockPath;
        CURL* curl_handle;
};
}  // namespace spqr
