#pragma once

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

// TODO creare Utils.cpp

namespace spqr {
YAML::Node loadYamlFile(const char* path);

// TODO templatizzare
std::string tryString(YAML::Node node, std::string message);
bool tryBool(YAML::Node node, std::string message);
}  // namespace spqr
