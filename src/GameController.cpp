#include "GameController.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace spqr {

// Helper function to convert string to lowercase
static std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void GameController::bindMujoco(MujocoContext* mujContext) {
    mujContext_ = mujContext;

    // Rebuild teams in game from current TeamManager state
    teamsInGame_.clear();
    for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
        teamsInGame_.emplace_back(TeamInGame(team));
        TeamInGame& teamInGame = teamsInGame_.back();
        for (const std::shared_ptr<Robot>& robot : team->robots) {
            teamInGame.addRobotInGame(RobotInGame(robot));
        }
    }
}

void GameController::reset() {
    mujContext_ = nullptr;
    teamsInGame_.clear();
    currentPhase_ = INITIAL;
    simTime_ = 0.0;
    gameTime_ = 0.0;
    lastUpdateGameTime_ = 0.0;
    lastUpdateScore_ = 0.0;
} 

std::map<std::string, std::string> GameController::availableCommands() const {
    return 
        {
        {"initial", "Set game phase to INITIAL"},
        {"ready", "Set game phase to READY"},
        {"set", "Set game phase to SET"},
        {"playing", "Set game phase to PLAYING"},
        {"finish", "Set game phase to FINISH"},
        {"mvr", "Move robot command: mvr <team> <robot_id> <x> <y> <theta> [m, m, deg]"},
        {"mvb", "Move ball command: mvb <x> <y> [m, m]"},
        {"penalize", "Penalize robot command: penalize <team> <robot_id> <penalty_type>. Penalty types: LEAVING_THE_FIELD, PUSHING, FOUL, ILLEGAL_POSITION"},
        {"unpenalize", "Unpenalize robot command: unpenalize <team> <robot_id>. Sets robot penalty to NONE_PENALTY"}
        };
}

bool GameController::isCommandValid(const std::string& command) const {
    auto commands = availableCommands();
    for (const auto& [cmd, desc] : commands) {
        if (command.rfind(cmd, 0) == 0) {
            return true;
        }
    }
    return false;
}

std::string GameController::handleCommand(std::string command) {
    if (!isCommandValid(command)) {
        return "Unknown command: " + command;
    }

    if ((command.rfind("initial", 0) == 0) || 
        (command.rfind("ready", 0) == 0) || 
        (command.rfind("set", 0) == 0) || 
        (command.rfind("playing", 0) == 0) || 
        (command.rfind("finish", 0) == 0)
    ) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        return handleGamePhase(cmd);
    }

    else if (command.rfind("mvr", 0) == 0) {
        // mvr <team> <robot_id> <x> <y> <theta> [m, m, deg]
        std::istringstream iss(command);
        std::string cmd, team;
        int robotId;
        double x, y, theta;

        if (!(iss >> cmd >> team >> robotId >> x >> y >> theta)) {
            return "Invalid mvr command format. Usage: mvr <team> <robot_id> <x> <y> <theta>";
        }

        // lowercase the team name for consistency
        team = toLower(team);

        return handleMoveRobot(team, robotId, x, y, theta);
    }

    else if (command.rfind("mvb", 0) == 0) {
        // mvb <x> <y> [m, m]
        std::istringstream iss(command);
        std::string cmd;
        double x, y;
        iss >> cmd >> x >> y;
        return handleMoveBall(x, y);
    }

    else if (command.rfind("penalize", 0) == 0) {
        // penalize <team> <robot_id> <penalty_type>
        std::istringstream iss(command);
        std::string cmd, team, penaltyStr;
        int robotId;

        if (!(iss >> cmd >> team >> robotId >> penaltyStr)) {
            return "Invalid penalize command format. Usage: penalize <team> <robot_id> <penalty_type>";
        }

        // lowercase the team name for consistency
        team = toLower(team);
        penaltyStr = toLower(penaltyStr);

        Penalty penalty;
        if (penaltyStr == "leaving_the_field")     penalty = LEAVING_THE_FIELD;
        else if (penaltyStr == "pushing")          penalty = PUSHING;
        else if (penaltyStr == "foul")             penalty = FOUL;
        else if (penaltyStr == "illegal_position") penalty = ILLEGAL_POSITION;
        else return "Invalid penalty type: " + penaltyStr;

        return handlePenalizeRobot(team, robotId, penalty);
    }

    else if (command.rfind("unpenalize", 0) == 0) {
        // unpenalize <team> <robot_id>
        std::istringstream iss(command);
        std::string cmd, team;
        int robotId;

        if (!(iss >> cmd >> team >> robotId)) {
            return "Invalid unpenalize command format. Usage: unpenalize <team> <robot_id>";
        }

        // lowercase the team name for consistency
        team = toLower(team);

        return handlePenalizeRobot(team, robotId, NONE_PENALTY);
    }

    return "Unknown command: " + command;
}

std::string GameController::handleGamePhase(std::string phase) {
    if (phase == "initial")      currentPhase_ = INITIAL;
    else if (phase == "ready")   currentPhase_ = READY;
    else if (phase == "set")     currentPhase_ = SET;
    else if (phase == "playing") currentPhase_ = PLAYING;
    else if (phase == "finish")  currentPhase_ = FINISH;
    else return "Invalid game phase: " + phase;
    
    currentPhaseElapsedTime_ = 0.0;

    // Convert phase to uppercase for display
    std::string upperPhase = phase;
    std::transform(upperPhase.begin(), upperPhase.end(), upperPhase.begin(), ::toupper);
    return "Game phase changed to: " + upperPhase;
}

std::string GameController::handleMoveRobot(std::string team,
                                            int robotId,
                                            double x,
                                            double y,
                                            double theta) {
    // Validate position bounds
    if (!checkFieldBounds(x, y)) {
        return "Invalid position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    std::shared_ptr<Robot> targetRobot = nullptr;
    for (TeamInGame& t : teamsInGame_) {
        if (toLower(t.getTeam()->name) == team) {
            for (const std::shared_ptr<Robot>& robot : t.getTeam()->robots) {
                if (robot->number == robotId) {
                    targetRobot = robot;
                    break;
                }
            }
            break;
        }
    }

    if (!targetRobot) {
        return "Robot " + std::to_string(robotId) + " not found in team '" + team + "'.";
    }

    std::string trunkBodyName = targetRobot->name + "_Trunk";
    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, trunkBodyName.c_str());
    if (bodyId < 0) {
        return "Error: Could not find body '" + trunkBodyName + "'";
    }

    // Body must have exactly one joint
    int jntadr = mujContext_->model->body_jntadr[bodyId];
    int jntnum = mujContext_->model->body_jntnum[bodyId];

    if (jntnum != 1) {
        return "Error: Robot trunk must have exactly one joint";
    }

    // Joint must be free
    if (mujContext_->model->jnt_type[jntadr] != mjJNT_FREE) {
        return "Error: Robot trunk joint is not a free joint";
    }

    int qposadr = mujContext_->model->jnt_qposadr[jntadr];
    int qveladr = mujContext_->model->jnt_dofadr[jntadr];

    // Safety bounds
    if (qposadr < 0 || qposadr + 6 >= mujContext_->model->nq) {
        return "Error: Invalid qpos address";
    }

    // Position (keep height)
    mujContext_->data->qpos[qposadr + 0] = x;
    mujContext_->data->qpos[qposadr + 1] = y;
    // qposadr + 2 = z → unchanged

    // Orientation (yaw only)
    double thetaRad = theta * M_PI / 180.0;
    double halfTheta = 0.5 * thetaRad;

    double qw = std::cos(halfTheta);
    double qz = std::sin(halfTheta);
    double norm = std::sqrt(qw * qw + qz * qz);

    mujContext_->data->qpos[qposadr + 3] = qw / norm;
    mujContext_->data->qpos[qposadr + 4] = 0.0;
    mujContext_->data->qpos[qposadr + 5] = 0.0;
    mujContext_->data->qpos[qposadr + 6] = qz / norm;

    // Zero linear + angular velocity (VERY IMPORTANT)
    for (int i = 0; i < 6; ++i) {
        mujContext_->data->qvel[qveladr + i] = 0.0;
    }

    // Defer mj_forward to simulation loop
    request_mjforward = true;

    return "Robot " + team + "-" + std::to_string(robotId) +
           " moved to (" + std::to_string(x) + ", " +
           std::to_string(y) + ", " +
           std::to_string(theta) + ")";
}


std::string GameController::handleMoveBall(double x, double y) {
    if (!checkFieldBounds(x, y)) {
        return "Invalid ball position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, "ball");
    if (bodyId < 0) {
        return "Error: Could not find ball body in the simulation.";
    }

    int jntadr = mujContext_->model->body_jntadr[bodyId];

    // Safety checks (important)
    if (mujContext_->model->jnt_type[jntadr] != mjJNT_FREE) {
        return "Error: ball joint is not a free joint";
    }

    int qposadr = mujContext_->model->jnt_qposadr[jntadr];
    int qveladr = mujContext_->model->jnt_dofadr[jntadr];

    // Position (keep height)
    mujContext_->data->qpos[qposadr + 0] = x;
    mujContext_->data->qpos[qposadr + 1] = y;

    // Zero linear + angular velocity
    for (int i = 0; i < 6; i++) {
        mujContext_->data->qvel[qveladr + i] = 0.0;
    }

    // Defer forward call to sim loop
    request_mjforward = true;

    return "Ball moved to (" + std::to_string(x) + ", " + std::to_string(y) + ")";
}


std::string GameController::handlePenalizeRobot(std::string team, int robotId, Penalty penalty) {
    
    double redTeamPenalization_y = 5.f; // y position for Red team penalization area
    double redTeamInitialPenalization_x = -4.9; // x position for Red team penalization area
    
    double blueTeamPenalization_y = -5.f; // y position for Blue team penalization area
    double blueTeamInitialPenalization_x = 4.9; // x position for Blue team penalization area

    double penalizationOffset = 0.5f; // Offset between robots in penalization area. Red goes +x direction, Blue goes -x direction

    for (TeamInGame& t : teamsInGame_) {
        if (toLower(t.getTeam()->name) == team) {
            

            // If robot is already penalized with the same penalty, do nothing
            for (const RobotInGame& rig : t.getRobotsInGame()) {
                if (rig.getRobot()->number == robotId) {
                    if(rig.getPenalty() == penalty){
                        return "Robot " + team + "-" + std::to_string(robotId) + " is already set to penalty " + penaltyToString(penalty);
                    }
                    break;
                }
            }

            if(penalty == NONE_PENALTY){
                // Red robots go in: redTeamInitialPenalization_x, redTeamPenalization_y - 0.5, -90
                // Blue robots go in: blueTeamInitialPenalization_x, blueTeamPenalization_y + 0.5, 90

                if(team == "red"){
                    handleMoveRobot(team, robotId, redTeamInitialPenalization_x, redTeamPenalization_y - 0.5, -90);
                }
                else if(team == "blue"){
                    handleMoveRobot(team, robotId, blueTeamInitialPenalization_x, blueTeamPenalization_y + 0.5, 90);
                }
            }
            else{
                // Red robots go in: redTeamInitialPenalization_x + n*offset, redTeamPenalization_y, 90
                // Blue robots go in: blueTeamInitialPenalization_x - n*offset, blueTeamPenalization_y, -90

                int penalizedCount = 0;
                for (const RobotInGame& other_rig : t.getRobotsInGame()) {
                    if (other_rig.getRobot()->number == robotId) continue; // Skip the robot being penalized

                    if (other_rig.getPenalty() != NONE_PENALTY) {
                        penalizedCount++;
                    }
                }

                if(team == "red"){
                    double penalization_x = redTeamInitialPenalization_x + penalizedCount * penalizationOffset;
                    handleMoveRobot(team, robotId, penalization_x, redTeamPenalization_y, 90);
                }
                else if(team == "blue"){
                    double penalization_x = blueTeamInitialPenalization_x - penalizedCount * penalizationOffset;
                    handleMoveRobot(team, robotId, penalization_x, blueTeamPenalization_y, -90);
                }
            }

            RobotInGame* rig = t.getRobotInGame(robotId);
            if (!rig) {
                return "Robot " + std::to_string(robotId) + " not found in team '" + team + "'";
            }
            rig->setPenalized(penalty, gameTime_);

            return "Robot " + team + "-" + std::to_string(robotId) + " penalization set to " + penaltyToString(penalty);
        }
    }

    return "Team '" + team + "' not found.";
}

std::string GameController::handleGoal(){
    double ball_x = mujContext_->data->qpos[mujContext_->model->jnt_qposadr[mj_name2id(mujContext_->model, mjOBJ_JOINT, "ball_joint")] + 0];
    double ball_y = mujContext_->data->qpos[mujContext_->model->jnt_qposadr[mj_name2id(mujContext_->model, mjOBJ_JOINT, "ball_joint")] + 1];
    
    // Ball Radius: 11 cm, Goal Width: 2.6 m, Field Length: 14 m, Line Width = Goal Post Widt: 8 cm
    double field_length_half = 7.0;
    double ball_radius = 0.11;
    double goal_width_half = 1.3;
    double line_width = 0.08; // Line Width = Goal Post Width   

    double effective_goal_width_half = goal_width_half - line_width/2 - ball_radius;
    double effective_field_length_half = field_length_half + ball_radius;

    if(ball_y >= effective_goal_width_half || ball_y <= -effective_goal_width_half) 
        return "None";

    if(ball_x >= effective_field_length_half) 
        return "Red";
    else if(ball_x <= -effective_field_length_half) 
        return "Blue";
    
    return "None";
}


void GameController::updateSimTime() {
    if (mujContext_ && mujContext_->data) {
        simTime_ = mujContext_->data->time;
    }
}

void GameController::updateGameTime(double time) {
    gameTime_ = time;
}

void GameController::updateScore(int redTeamScore, int blueTeamScore) {
    for (TeamInGame& t : teamsInGame_) {
        std::string teamNameLower = toLower(t.getTeam()->name);
        if (teamNameLower == "red") {
            t.setScore(redTeamScore);
        } else if (teamNameLower == "blue") {
            t.setScore(blueTeamScore);
        }
    }
}

void GameController::update(){
    if(request_mjforward){
        if (mujContext_ && mujContext_->model && mujContext_->data) {
            mj_forward(mujContext_->model, mujContext_->data);
        }
        request_mjforward = false;
    }

    // Update sim time
    if (mujContext_ && mujContext_->data) {
        simTime_ = mujContext_->data->time;
    }

    if (currentPhase_ != INITIAL && currentPhase_ != FINISH) {
        // Update game time
        if (simTime_ - lastUpdateGameTime_ >= 1.0) {
            gameTime_ += 1.0;
            lastUpdateGameTime_ = simTime_;
        }
    }

    // Update current phase elapsed time
    if (simTime_ - lastUpdatecurrentPhaseElapsedTime_ >= 1.0) {
        currentPhaseElapsedTime_ += 1.0;
        lastUpdatecurrentPhaseElapsedTime_ = simTime_;
    }


    if(currentPhase_ == INITIAL){
        // Transition to READY phase after initialPhaseDuration_
        if (initialPhaseDuration_ > 0 && currentPhaseElapsedTime_ >= initialPhaseDuration_){
            currentPhase_ = READY;
            currentPhaseElapsedTime_ = 0;
        }
    }
    else if(currentPhase_ == READY){
        // Transition to SET phase after readyPhaseDuration_
        if(readyPhaseDuration_ > 0 && currentPhaseElapsedTime_ >= readyPhaseDuration_){
            currentPhase_ = SET;
            currentPhaseElapsedTime_ = 0;
        }
    }
    else if(currentPhase_ == SET){
        // Transition to PLAYING phase after setPhaseDuration_
        if(setPhaseDuration_ > 0 && currentPhaseElapsedTime_ >= setPhaseDuration_){
            currentPhase_ = PLAYING;
            currentPhaseElapsedTime_ = 0;
        }
    }
    else if(currentPhase_ == PLAYING){
        if(playingPhaseDuration_ > 0 && currentPhaseElapsedTime_ >= playingPhaseDuration_){
            currentPhase_ = FINISH;
            currentPhaseElapsedTime_ = 0;
        }
        
        // Handle Goal
        std::string scoringTeam = handleGoal();
        if(scoringTeam != "None" && (simTime_ - lastUpdateScore_ >= 1.0)){
            if(scoringTeam == "Red"){
                int redScore, blueScore;
                std::tie(redScore, blueScore) = getScore();
                redScore += 1;
                updateScore(redScore, blueScore);
            }
            else if(scoringTeam == "Blue"){
                int redScore, blueScore;
                std::tie(redScore, blueScore) = getScore();
                blueScore += 1;
                updateScore(redScore, blueScore);
            }

            // Reset ball position to center
            handleMoveBall(0.0, 0.0);
            currentPhase_ = SET; // Reset game phase to SET after a goal
            lastUpdateScore_ = simTime_;
        }

    }
}

bool GameController::checkFieldBounds(double x, double y) {
    return (x > minX && x < maxX && y > minY && y < maxY);
}

}  // namespace spqr
