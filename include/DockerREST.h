#pragma once

#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "curl/curl.h"

#define POST "POST"
#define GET "GET"
#define DELETE "DELETE"
#define PUT "PUT"

/*
Endpoints and parameters:
https://docs.docker.com/reference/api/engine/version/v1.39/
*/

#define CREATE_OK_RESPONSE 201
#define GET_OK_RESPONSE 200
#define START_OK_RESPONSE 204
#define STOP_OK_RESPONSE 204
#define DELETE_OK_RESPONSE 204
#define CONNECT_OK_RESPONSE 200
#define DISCONNECT_OK_RESPONSE 200

namespace spqr {

inline std::string create_container_endpoint(const std::string& name) {
    return "/containers/create?name=" + name;
}

inline std::string start_container_endpoint(const std::string& id) {
    return "/containers/" + id + "/start";
}

inline std::string stop_container_endpoint(const std::string& id) {
    return "/containers/" + id + "/stop?t=0";  // TODO: forcing a SIGKILL trigger to 0 seconds as workaround. If the application
                                               // is well behaved, this shouldn't be necessary
}

inline std::string remove_container_endpoint(const std::string& id) {
    return "/containers/" + id;
}

inline std::string force_remove_container_endpoint(const std::string& name) {
    return "/containers/" + name + "?force=1";
}

const std::string create_network_endpoint = "/networks/create";

inline std::string inspect_network_endpoint(const std::string& name) {
    return "/networks/" + name;
}

inline std::string connect_network_endpoint(const std::string& id) {
    return "/networks/" + id + "/connect";
}

inline std::string disconnect_network_endpoint(const std::string& id) {
    return "/networks/" + id + "/disconnect";
}

inline std::string remove_network_endpoint(const std::string& id) {
    return "/networks/" + id;
}

class CURLClient {
    public:
        CURLClient(const std::string& sockPath) : sockPath(sockPath) {
            curl_handle = curl_easy_init();
            if (!curl_handle)
                throw std::runtime_error("Failed to init curl handle");
        }
        ~CURLClient() {
            if (curl_handle)
                curl_easy_cleanup(curl_handle);
        };

        std::string request(const std::string& method, const std::string& endpoint, const long expected_response,
                            const nlohmann::json* body = nullptr) {
            curl_easy_reset(curl_handle);
            std::string url = "http://localhost" + endpoint;

            curl_easy_setopt(curl_handle, CURLOPT_UNIX_SOCKET_PATH, sockPath.c_str());
            curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, method.c_str());

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

            const auto write_callback = +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
                std::string* resp = static_cast<std::string*>(userdata);
                resp->append(ptr, size * nmemb);
                return size * nmemb;
            };

            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
            std::string response;
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response);

            std::string postData;
            if (body != nullptr) {
                postData = body->dump();
                curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData.c_str());
                curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, postData.size());
            }

            long response_code = 0;
            CURLcode res = curl_easy_perform(curl_handle);
            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
            curl_slist_free_all(headers);

            if (res != CURLE_OK)
                throw std::runtime_error(std::string("Curl error: ") + curl_easy_strerror(res));

            if (expected_response && response_code != expected_response)
                throw std::runtime_error("Docker API request to " + endpoint + " failed: HTTP " + std::to_string(response_code) + ". " + response);

            return response;
        }

        CURL* curl_handle;
        std::string sockPath;
};

}  // namespace spqr
