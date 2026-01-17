#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <tuple>

#include "MujocoContext.h"
#include "robots/Robot.h"
#include "Team.h"

namespace spqr {

enum GamePhase { INITIAL, READY, SET, PLAYING, FINISH };
inline std::string gamePhaseToString(GamePhase phase) {
    switch (phase) {
        case INITIAL: return "INITIAL";
        case READY: return "READY";
        case SET: return "SET";
        case PLAYING: return "PLAYING";
        case FINISH: return "FINISH";
        default: return "UNKNOWN_PHASE";
    }
}

enum GameSubPhase { NONESUBPHASE, KICKOFF, KICKIN, CORNERKICK, GOALKICK, PENALTYKICK, PUSHINGFREEKICK };
inline std::string gameSubPhaseToString(GameSubPhase subPhase) {
    switch (subPhase) {
        case NONESUBPHASE: return "NONESUBPHASE";
        case KICKOFF: return "KICKOFF";
        case KICKIN: return "KICKIN";
        case CORNERKICK: return "CORNERKICK";
        case GOALKICK: return "GOALKICK";
        case PENALTYKICK: return "PENALTYKICK";
        case PUSHINGFREEKICK: return "PUSHINGFREEKICK";
        default: return "UNKNOWN_SUBPHASE";
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
        GameSubPhase getCurrentSubPhase() const {
            return currentSubPhase_;
        }
        double getSimTime() const {
            return simTime_;
        }
        double getGameTime() const {
            return gameTime_;
        }
        double getCurrentPhaseElapsedTime() const {
            return currentPhaseElapsedTime_;
        }
        double getCurrentSubPhaseElapsedTime() const {
            return currentSubPhaseElapsedTime_;
        }

        int getInitialPhaseDuration() const {
            return initialPhaseDuration_;
        }   
        int getReadyPhaseDuration() const {
            return readyPhaseDuration_;
        }
        int getSetPhaseDuration() const {
            return setPhaseDuration_;
        }
        int getPlayingPhaseDuration() const {
            return playingPhaseDuration_;
        }
        int getSubPhaseDuration() const {
            return subPhaseDuration_;
        }

        void setPlayingPhaseDuration(int duration) {
            playingPhaseDuration_ = duration;
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

        std::map<std::string, std::string> availableCommands() const;
        bool isCommandValid(const std::string &command) const;
        std::string handleCommand(std::string command);
        std::string handleGamePhase(std::string phase);
        std::string handleGameSubPhase(std::string subPhase, std::string team);
        std::string handleMoveRobot(std::string team, int robotId, double x, double y, double theta);
        std::string handleMoveBall(double x, double y);
        std::string handlePenalizeRobot(std::string team, int robotId, Penalty penalty);
        std::tuple<std::string, std::string> handleBallEvent();
        std::tuple<double, double> getBallPosition() const;
        void updateSimTime();
        void updateGameTime(double time);
        void updateScore(int redTeamScore, int blueTeamScore);
        void updateBallContact();
        void update();

    private:
        GameController() = default;
        ~GameController() = default;
        GameController(const GameController&) = delete;
        GameController& operator=(const GameController&) = delete;

        MujocoContext *mujContext_ = nullptr;
        std::vector<TeamInGame> teamsInGame_;

        double simTime_ = 0.0;                           // Simulation time in seconds
        double gameTime_ = 0.0;                          // Game time in seconds
        
        GamePhase currentPhase_ = INITIAL;               // Current game phase
        int initialPhaseDuration_ = 30;                  // seconds
        int readyPhaseDuration_ = 45;                    // seconds
        int setPhaseDuration_ = 15;                      // seconds
        int playingPhaseDuration_ = 600;                 // seconds  (10 minutes)
        double currentPhaseElapsedTime_ = 0.0;           // seconds
        double lastUpdatecurrentPhaseElapsedTime_ = 0.0; // Sim time of last phase elapsed time update

        GameSubPhase currentSubPhase_ = NONESUBPHASE;         // Current game sub-phase
        int subPhaseDuration_ = 30;                      // seconds
        double currentSubPhaseElapsedTime_ = 0.0;        // seconds
        double lastUpdateSubPhaseElapsedTime_ = 0.0;     // Sim time of last sub-phase elapsed time update

        double lastUpdateGameTime_ = 0.0;                // Sim time of last game time update
        double lastUpdateScore_ = 0.0;                   // Sim time of last score update

        std::string lastTeamToScore_ = "none";           // Last team to score <"Red", "Blue", "none">
        std::string lastBallContactTeam_ = "none";       // Last team to contact the ball <"Red", "Blue", "none">
        std::string kickOffTeam_ = "red";                // Team to kickoff <"Red", "Blue">

        bool request_mjforward = false;                  // Flag to request mj_forward() call in update()

        std::map<std::string, float> fieldDimensions = {
            {"width", 14.0f},  // x dimension
            {"height", 9.0f},   // y dimension
            {"center_radius", 1.5f}, // center circle radius
            {"goal_area_width", 1.0f}, // goal area width
            {"goal_area_height", 4.0f}, // goal area height
            {"penalty_area_width", 3.0f}, // penalty area width
            {"penalty_area_height", 6.5f},   // penalty area width
            {"goal_width", 2.6f}, // goal width
            {"goal_height", 1.8f}, // goal height
            {"goal_depth", 0.6f}, // goal depth
            {"line_width", 0.08f}, // line thickness
            {"penalty_mark_distance", 2.1f},  // distance of penalty mark from goal line"
            {"ball_radius", 0.11f}  // ball radius
        };

        bool checkFieldBounds(double x, double y) {
            return (x >= -fieldDimensions["width"]/2 && x <= fieldDimensions["width"]/2 &&
                    y >= -fieldDimensions["height"]/2 && y <= fieldDimensions["height"]/2);
        }

        static std::string toLower(const std::string& str) {
            std::string result = str;
            std::transform(result.begin(), result.end(), result.begin(), ::tolower);
            return result;
        }
};

}  // namespace spqr
