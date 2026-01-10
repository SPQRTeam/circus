#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "MujocoContext.h"
#include "Team.h"

namespace spqr {

enum GamePhase { INITIAL, READY, SET, PLAY };

class GameController {
    public:
        GameController(MujocoContext *mujContext) : mujContext_(mujContext) {}
        ~GameController() = default;

        GamePhase getCurrentPhase() const {
            return currentPhase_;
        }
        double getSimTime() const {
            return simTime_;
        }
        double getGameTime() const {
            return gameTime_;
        }
        int getGameDuration() const {
            return gameDuration_;
        }
        int getScoreRedTeam() const {
            return scoreRedTeam_;
        }
        int getScoreBlueTeam() const {
            return scoreBlueTeam_;
        }
        void setGameDuration(int duration) {
            gameDuration_ = duration;
        }

        std::map<std::string, std::string> availableCommands() const;
        bool isCommandValid(const std::string &command) const;
        std::string handleCommand(std::string command);
        std::string handleGamePhase(std::string phase);
        std::string handleMoveRobot(std::string team, int robotId, double x, double y, double theta);
        std::string handleMoveBall(double x, double y);
        void updateSimTime();
        void updateGameTime(double time);
        void updateScore(int redTeamScore, int blueTeamScore);

    private:
        std::string getAvailableTeams();
        std::string getAvailableRobots(std::shared_ptr<Team> team);
        bool checkFieldBounds(double x, double y);

        MujocoContext *mujContext_;
        GamePhase currentPhase_ = INITIAL;
        double simTime_ = 0.0;
        double gameTime_ = 0.0;
        int gameDuration_ = 600;  // default 10 minutes

        int scoreRedTeam_ = 0;
        int scoreBlueTeam_ = 0;

        float minX = -8.0f;
        float maxX = 8.0f;
        float minY = -5.5f;
        float maxY = 5.5f;
};

}  // namespace spqr
