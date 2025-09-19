#pragma once

#include <sys/types.h>

#include <Eigen/Eigen>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Container.h"
#include <vector>
#include <mutex>

namespace spqr {

struct Team;  // Forward declaration

struct Robot {
    std::string name;
    std::string type;
    uint8_t number;
    Eigen::Vector3d position;
    Eigen::Vector3d orientation;  // Euler angles
    std::unique_ptr<Container> container;
    std::shared_ptr<Team> team;
};

class RobotManager {
   public:
    // Singleton class
    static RobotManager& instance() {
        static RobotManager mgr;
        return mgr;
    }

    void registerRobot(std::shared_ptr<Robot> robot) {
        std::lock_guard<std::mutex> lock(mutex_);
        robots_.push_back(std::move(robot));
    }

    std::vector<std::shared_ptr<Robot>> getRobots() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_.size();
    }

    void clear() {
        std::lock_guard lock(mutex_);
        for (std::shared_ptr<Robot> r : robots_) {
            // Drop ownership first
            r->container.reset();
            r->team.reset();
        }
        robots_.clear();
    }

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;
};

}  // namespace spqr
