#include <stdexcept>
#include <string>

#include "DockerREST.h"

#define CIRCUS_NETWORK_NAME "CIRCUS_network"
#define UAN_SEVEN_CIU "172.21."  // Per coerenza

namespace spqr {

class CircusNetwork {
    public:
        // Singleton class
        static CircusNetwork& instance() {
            static CircusNetwork mgr;
            return mgr;
        }

        inline bool isUp() {
            return networkId.size() > 0;
        }

        std::string getId() {
            if (!isUp()) {
                throw std::runtime_error("Attempted to get the network ID when it's not initialized.");
            }
            return networkId;
        }

        void init() {
            if (isUp()) {
                throw std::runtime_error("Attempted to initialize the network when it is already up.");
            }

            // Check if the network already exists (e.g. leftover from a previous crash)
            std::string inspect_raw = curlClient.request(GET, inspect_network_endpoint(CIRCUS_NETWORK_NAME), 0);
            nlohmann::json inspect = nlohmann::json::parse(inspect_raw);
            if (inspect.contains("Id")) {
                networkId = inspect["Id"];
                return;
            }

            nlohmann::json payload;
            std::string subnet = UAN_SEVEN_CIU "0.0/16";
            std::string gateway = UAN_SEVEN_CIU "0.1";
            payload["Name"] = CIRCUS_NETWORK_NAME;
            payload["IPAM"]["Config"] = {{// https://docs.docker.com/reference/api/engine/version/v1.53/#tag/Network/operation/NetworkCreate
                                          {"Subnet", subnet},
                                          {"IPRange", subnet},
                                          {"Gateway", gateway}}};
            payload["Options"] = {{"com.docker.network.bridge.name", "docker-circus"},
                                  {"com.docker.network.bridge.enable_ip_masquerade", "false"},
                                  {"com.docker.network.driver.mtu", "1500"},
                                  {"com.docker.network.container_iface_prefix", "circus"}};

            std::string resp_raw = curlClient.request(POST, create_network_endpoint, CREATE_OK_RESPONSE, &payload);

            nlohmann::json resp = nlohmann::json::parse(resp_raw);
            if (!resp.contains("Id"))
                throw std::runtime_error("Docker subnet create failed");

            networkId = resp["Id"];
        }

    private:
        CircusNetwork(const std::string& sockPath = "/var/run/docker.sock") : curlClient(sockPath){};

        ~CircusNetwork() {
            if (isUp()) {
                curlClient.request(DELETE, remove_network_endpoint(networkId), DELETE_OK_RESPONSE);
            }
        };

        std::string networkId = "";
        CURLClient curlClient;

    public:
        CircusNetwork(CircusNetwork const&) = delete;
        void operator=(CircusNetwork const&) = delete;
};

}  // namespace spqr
