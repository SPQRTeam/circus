#include "RobotManager.h"

#include <filesystem>

#include "Constants.h"
#include "Team.h"  // needed for the forward declaration in the .h
#include "Utils.h"

namespace spqr {

void RobotManager::applyCommands() {
    for (std::shared_ptr<Robot> r : robots_) {
        r->applyCommands();
    }
}

void RobotManager::registerRobot(std::shared_ptr<Robot> robot) {
    std::lock_guard<std::mutex> lock(mutex_);

    robots_.push_back(std::move(robot));
}

std::vector<std::shared_ptr<Robot>> RobotManager::getRobots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return robots_;
}

size_t RobotManager::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return robots_.size();
}

void RobotManager::update() {
    std::lock_guard lock(mutex_);
    for (std::shared_ptr<Robot> r : robots_) {
        r->update();
    }
}

void RobotManager::clear() {
    std::lock_guard lock(mutex_);
    for (std::shared_ptr<Robot> r : robots_) {
        // Drop ownership first
        r->container.reset();
        r->team.reset();
    }
    robots_.clear();
}

void RobotManager::bindMujoco(MujocoContext* mujContext) {
    for (std::shared_ptr<Robot> r : robots_)
        r->bindMujoco(mujContext);
}

std::shared_ptr<Robot> RobotManager::create(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& pos,
                                            const Eigen::Vector3d& ori, const std::string& colorName, const std::shared_ptr<Team> team) {
    auto it = robotFactory.find(type);
    if (it != robotFactory.end())
        return it->second(name, type, number, pos, ori, colorName, team);
    return nullptr;
}

void RobotManager::startContainers(const std::string& fwkCfgPath,
                                   const std::string& pathsCfgPath) {
    YAML::Node configRoot = loadYamlFile(fwkCfgPath.c_str());

    if (!configRoot["image"])
        throw std::runtime_error("Missing 'image' key in YAML file");

    std::string image = tryString(configRoot["image"], "'image' must be a string: ");

    if (!configRoot["volumes"] || !configRoot["volumes"].IsSequence())
        throw std::runtime_error("'volumes' key missing or not a sequence");

    // Paths in framework config can be relative to PIXI_PROJECT_ROOT.
    const char* pixi_project_root = std::getenv("FRAMEWORK_PATH") ?
        std::getenv("FRAMEWORK_PATH") : std::getenv("PIXI_PROJECT_ROOT");

    std::vector<std::string> binds;
    std::optional<YAML::Node> pathsRoot;
    for (const auto& v : configRoot["volumes"]) {
        std::string v2 = tryString(v, "Volume entry must be a string: ");
        if (v2.starts_with("<")) {
            if (!pathsRoot) pathsRoot = loadYamlFile(pathsCfgPath.c_str());
            int end = v2.find('>');
            std::string name = v2.substr(1, end - 1);

            if (!(*pathsRoot)[name]) {
                throw std::runtime_error("Entry doesn't exist in path_constants: " + name);
            }

            std::string name_str = tryString((*pathsRoot)[name], "path_constants entries must be strings: ");
            v2.replace(0, end + 1, name_str);
        } else {
            // Resolve relative / empty host paths against PIXI_PROJECT_ROOT.
            auto colon = v2.find(':');
            if (colon != std::string::npos) {
                std::string host = v2.substr(0, colon);
                if (host.empty() || host[0] != '/') {
                    if (!pixi_project_root)
                        throw std::runtime_error("PIXI_PROJECT_ROOT and FRAMEWORK_PATH environment variables are not set");
                    namespace fs = std::filesystem;
                    fs::path hp = fs::weakly_canonical(fs::path(pixi_project_root) / host);
                    v2 = hp.string() + v2.substr(colon);
                }
            }
        }
        binds.push_back(v2);
    }
    for (std::shared_ptr<Robot> r : robots_) {
        r->container = std::make_unique<Container>("CIRCUS_" + r->name + "_container");
        r->container->create(r, image, binds);
        r->container->start();
    }
    std::cout << "Containers started successfully!" << std::endl;
}

bool RobotManager::areAllRobotsConnected() const {
    for (auto& r : robots_) {
        if (!r->isConnected) {
            return false;
        }
    }
    return true;
}

bool RobotManager::areAllRobotsReady() const {
    for (const auto& r : robots_)
        if (!r->isReady)
            return false;
    std::cout << "All robots are ready!" << std::endl;
    return true;
}

}  // namespace spqr
