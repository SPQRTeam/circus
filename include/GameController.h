#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <tuple>

#include "MujocoContext.h"
#include "robots/Robot.h"
#include "Team.h"

namespace spqr {

enum GamePhase { INITIAL, READY, SET, PLAYING };
inline std::string gamePhaseToString(GamePhase phase) {
    switch (phase) {
        case INITIAL: return "INITIAL";
        case READY: return "READY";
        case SET: return "SET";
        case PLAYING: return "PLAYING";
        default: return "UNKNOWN_PHASE";
    }
}

enum Penalty { NONE_PENALTY, LEAVING_THE_FIELD, PUSHING, FOUL, ILLEGAL_POSITION };
inline std::string penaltyToString(Penalty penalty) {
    switch (penalty) {
        case NONE_PENALTY: return "NONE_PENALTY";
        case LEAVING_THE_FIELD: return "LEAVING_THE_FIELD";
        case PUSHING: return "PUSHING";
        case FOUL: return "FOUL";
        case ILLEGAL_POSITION: return "ILLEGAL_POSITION";
        default: return "UNKNOWN_PENALTY";
    }
}

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

        Penalty getPenalty() const {
            return currentPenalty_;
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

        void addRobotInGame(RobotInGame rig) {
            robotsInGame_.emplace_back(rig);
        }
        
        RobotInGame* getRobotInGame(int robotId) {
            for (RobotInGame& rig : robotsInGame_) {
                if (rig.getRobot()->number == robotId) {
                    return &rig;
                }
            }
            return nullptr;
        }

        std::vector<RobotInGame>& getRobotsInGame() {
            return robotsInGame_;
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
        // Singleton access
        static GameController& instance() {
            static GameController gc;
            return gc;
        }

        void bindMujoco(MujocoContext* mujContext);
        void reset();

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
                std::string teamName = t.getTeam()->name;
                std::transform(teamName.begin(), teamName.end(), teamName.begin(), ::tolower);
                if (teamName == "red") {
                    redScore = t.getScore();
                } else if (teamName == "blue") {
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
        GameController() = default;
        ~GameController() = default;
        GameController(const GameController&) = delete;
        GameController& operator=(const GameController&) = delete;

        bool checkFieldBounds(double x, double y);

        MujocoContext *mujContext_ = nullptr;
        std::vector<TeamInGame> teamsInGame_;

        GamePhase currentPhase_ = INITIAL;
        double simTime_ = 0.0;
        double gameTime_ = 0.0;
        int gameDuration_ = 600;          // 10 minutes
        double lastUpdateGameTime_ = 0.0; // Sim time of last game time update
        double lastUpdateScore_ = 0.0;    // Sim time of last score update

        float minX = -8.0f;
        float maxX = 8.0f;
        float minY = -5.5f;
        float maxY = 5.5f;

        bool request_mjforward = false;
};

}  // namespace spqr
