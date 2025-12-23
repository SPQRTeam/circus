# include "Utils.h"

namespace spqr {
    YAML::Node loadYamlFile(const char* path) {
        try {
            return YAML::LoadFile(path);
        } catch (const YAML::BadFile& e) {
            throw std::runtime_error("Failed to open YAML file: " + std::string(path));
        } catch (const YAML::ParserException& e) {
            throw std::runtime_error("Failed to parse YAML file: " + std::string(e.what()));
        }
    }
    
    // TODO templatizzare
    std::string tryString(YAML::Node node, std::string message) {
        try {
            return node.as<std::string>();
        } catch (const YAML::Exception& e) {
            throw std::runtime_error(message + std::string(e.what()));
        }
    }
    bool tryBool(YAML::Node node, std::string message) {
        try {
            return node.as<bool>();
        } catch (const YAML::Exception& e) {
            throw std::runtime_error(message + std::string(e.what()));
        }
    }
}
