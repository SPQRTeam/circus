#include "Team.h"
#include <curl/curl.h>

#include <stdexcept>
#include <string>
#include "DockerREST.h"

namespace spqr {

void Team::create_subnet() {
    if (has_subnet()) {
        throw std::runtime_error("Attempted to create a subnet when one already exists for this team.");
    }

    nlohmann::json payload;
    std::string subnet = UAN_SEVEN_CIU + std::to_string(number) + ".0/24";
    std::string gateway = UAN_SEVEN_CIU + std::to_string(number) + ".1";
    payload["Name"] = subnet_name();
    payload["IPAM"]["Config"] = {{  // https://docs.docker.com/reference/api/engine/version/v1.53/#tag/Network/operation/NetworkCreate
        {"Subnet", subnet},
        {"IPRange", subnet},
        {"Gateway", gateway}
    }};
    // payload["Options"] = {  // una di queste opzioni rompe la rete, studiare con cautela in caso ci servissero!
    //     {"com.docker.network.bridge.default_bridge", "true"},
    //     {"com.docker.network.bridge.enable_icc", "true"},
    //     {"com.docker.network.bridge.enable_ip_masquerade", "true"},
    //     {"com.docker.network.bridge.host_binding_ipv4", "0.0.0.0"},
    //     // {"com.docker.network.bridge.name", "docker0"},
    //     {"com.docker.network.driver.mtu", "1500"}
    // };

    std::string resp_raw = curlClient.request(POST, create_network_endpoint, CREATE_OK_RESPONSE, &payload);

    nlohmann::json resp = nlohmann::json::parse(resp_raw);
    if (!resp.contains("Id"))
        throw std::runtime_error("Docker subnet create failed");

    subnetId = resp["Id"];
}

void Team::remove_subnet() {
    if (!has_subnet()) {
        throw std::runtime_error("Attempted to remove this team's subnet when it has none.");
    }
    curlClient.request(DELETE, remove_network_endpoint(subnetId), DELETE_OK_RESPONSE);
}

}