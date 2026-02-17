#pragma once
#include <memory>
#include <string>
#include <vector>

#include "RobotManager.h"
#include "robots/Robot.h"
#include "DockerREST.h"

#define UAN_SEVEN_CIU "172.18."  // TEMP, non so ancora se voglio pestare i piedi alla rete 172.17 o se separarmi sulla 172.18

namespace spqr {

class Team {
    public:
        Team(const std::string& name, int number, const std::string& sockPath = "/var/run/docker.sock") : name(name), number(number), curlClient(sockPath) {}

        ~Team() {
            if (has_subnet()) {
                remove_subnet();
            }
        }

        inline std::string subnet_name() {  // TODO togliere se usato solo una volta
            return "CIRCUS_" + name + "_network";
        }

        inline bool has_subnet() {
            return subnetId.size() > 0;
        }

        void create_subnet();
        void remove_subnet();
    
        std::string name;
        int number;
        std::vector<std::shared_ptr<Robot>> robots;
        CURLClient curlClient;
        std::string subnetId = "";
};

class TeamManager {
    public:
        // Singleton class
        static TeamManager& instance() {
            static TeamManager mgr;
            return mgr;
        }

        void registerTeam(std::shared_ptr<Team> team) {
            std::lock_guard<std::mutex> lock(mutex_);

            for (const std::shared_ptr<Robot>& robot : team->robots)
                RobotManager::instance().registerRobot(robot);

            teams_.push_back(std::move(team));
        }

        std::vector<std::shared_ptr<Team>> getTeams() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return teams_;
        }

        size_t count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return teams_.size();
        }

        void clear() {
            std::lock_guard lock(mutex_);
            RobotManager::instance().clear();

            for (const std::shared_ptr<Team>& team : teams_)
                team->robots.clear();

            teams_.clear();
        }

        void createSubnets() {
            for (std::shared_ptr<Team> team : teams_) {
                team->create_subnet();
                if (team->robots.size() == 0) {
                    std::cout << "Warning: the team is empty on 'connecting to subnets' time. It shouldn't at this step." << std::endl;
                }
                for (std::shared_ptr<Robot> r : team->robots) {
                    r->container->connect(team, r->number);
                }
            }
        }

    private:
        TeamManager() = default;
        ~TeamManager() = default;

        TeamManager(const TeamManager&) = delete;
        TeamManager& operator=(const TeamManager&) = delete;

        mutable std::mutex mutex_;
        std::vector<std::shared_ptr<Team>> teams_;
};

}  // namespace spqr
