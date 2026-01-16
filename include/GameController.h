#pragma once

#include <map>
#include <string>

#include "MujocoContext.h"
#include "robots/Robot.h"
#include "Team.h"

namespace spqr {

enum GamePhase { INITIAL, READY, SET, PLAY };
enum Penalty { NONE_PENALTY, LEAVING_THE_FIELD, PUSHING, FOUL, ILLEGAL_DEFENSE }; // TODO: check true penalties

class RobotInGame {
    public:
        RobotInGame(std::shared_ptr<Robot> robot) : robot_(robot) {};
        ~RobotInGame() = default;

        std::shared_ptr<Robot> getRobot() const {
            return robot_;
        }

        void setPenalized(Penalty penalty, int gameTimeWhenPenalized) {
            isPenalized_ = (penalty != NONE_PENALTY);
            currentPenalty_ = penalty;
            timeOfLastPenalty_ = (isPenalized_) ? gameTimeWhenPenalized : -1;
        }

    private:
        std::shared_ptr<Robot> robot_;
        bool isPenalized_ = false;
        int timeOfLastPenalty_ = -1;
        Penalty currentPenalty_ = NONE_PENALTY;
};

class TeamInGame {
    public:
        TeamInGame(std::shared_ptr<Team> team) : team_(team) {};
        ~TeamInGame() = default;

        std::shared_ptr<Team> getTeam() const {
            return team_;
        }

        void addRobotInGame(std::shared_ptr<Robot> robot) {
            robotsInGame_.emplace_back(robot);
        }
        
        RobotInGame& getRobotInGame(int robotId) {
            for (RobotInGame& rig : robotsInGame_) {
                if (rig.getRobot()->number == robotId) {
                    return rig;
                }
            }
            throw std::runtime_error("Robot with ID " + std::to_string(robotId) + " not found in team '" + team_->name + "'.");
        }

        void setScore(int score) {
            score_ = score;
        }

        int getScore() const {
            return score_;
        }

    private:
        std::shared_ptr<Team> team_;
        std::vector<RobotInGame> robotsInGame_;
        int score_ = 0;
};


class GameController {
    public:
        GameController(MujocoContext *mujContext);
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
        std::tuple<int, int> getScore() const {
            int redScore = 0;
            int blueScore = 0;
            for (const TeamInGame& t : teamsInGame_) {
                if (t.getTeam()->name == "Red") {
                    redScore = t.getScore();
                } else if (t.getTeam()->name == "Blue") {
                    blueScore = t.getScore();
                }
            }
            return std::make_tuple(redScore, blueScore);
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
        std::string handlePenalizeRobot(std::string team, int robotId, Penalty penalty);
        std::string handleGoal();
        void updateSimTime();
        void updateGameTime(double time);
        void updateScore(int redTeamScore, int blueTeamScore);
        void update();

    private:
        bool checkFieldBounds(double x, double y);

        MujocoContext *mujContext_;
        std::vector<TeamInGame> teamsInGame_;

        GamePhase currentPhase_ = INITIAL;
        double simTime_ = 0.0;
        double gameTime_ = 0.0;
        double lastGameTimeUpdateSimTime_ = 0.0;
        int gameDuration_ = 600;  // default 10 minutes

        float minX = -8.0f;
        float maxX = 8.0f;
        float minY = -5.5f;
        float maxY = 5.5f;
};

}  // namespace spqr
