#pragma once

#include "MujocoContext.h"

namespace spqr {

enum GamePhase {
    INITIAL,
    READY,
    SET,
    PLAY
};

class GameController {
    public:
        GameController(MujocoContext *mujContext) : mujContext_(mujContext) {}
        ~GameController() = default;

        GamePhase getCurrentPhase() const { return currentPhase_; }
        double getSimTime() const { return simTime_; }
        double getGameTime() const { return gameTime_; }
        int getGameDuration() const { return gameDuration_; }
        int getScoreRedTeam() const { return scoreRedTeam_; }
        int getScoreBlueTeam() const { return scoreBlueTeam_; }
        void setGameDuration(int duration) { gameDuration_ = duration; }

        void updateGamePhase(std::string command) {
            if (command == "initial") {
                currentPhase_ = INITIAL;
            } else if (command == "ready") {
                currentPhase_ = READY;
            } else if (command == "set") {
                currentPhase_ = SET;
            } else if (command == "play") {
                currentPhase_ = PLAY;
            }
        }

        void updateSimTime() {
            if (mujContext_ && mujContext_->data) {
                simTime_ = mujContext_->data->time;
            }
        }

        void updateGameTime(double time) {
            gameTime_ = time;
        }

        void updateScore(int redTeamScore, int blueTeamScore) {
            scoreRedTeam_ = redTeamScore;
            scoreBlueTeam_ = blueTeamScore;
        }

    private:
        MujocoContext *mujContext_;
        GamePhase currentPhase_ = INITIAL;
        double simTime_ = 0.0;
        double gameTime_ = 0.0;
        int gameDuration_ = 600; // default 10 minutes
        int scoreRedTeam_ = 0;
        int scoreBlueTeam_ = 0;
};

}