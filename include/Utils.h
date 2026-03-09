#pragma once

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

namespace spqr {
YAML::Node loadYamlFile(const char* path);
void writeYamlFile(const YAML::Node& root, const char* path);  // untested probably

// TODO templatizzare
std::string tryString(YAML::Node node, std::string message);
bool tryBool(YAML::Node node, std::string message);
}  // namespace spqr
