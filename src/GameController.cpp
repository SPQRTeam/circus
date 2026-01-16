#include "GameController.h"

namespace spqr {

GameController::GameController(MujocoContext* mujContext) : mujContext_(mujContext) {
    for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
        TeamInGame teamInGame(team);
        teamsInGame_.emplace_back(teamInGame);
        for (const std::shared_ptr<Robot>& robot : team->robots) {
            teamInGame.addRobotInGame(robot);
        }
    }
} 

std::map<std::string, std::string> GameController::availableCommands() const {
    return 
        {
        {"initial", "Set game phase to INITIAL"},
        {"ready", "Set game phase to READY"},
        {"set", "Set game phase to SET"},
        {"play", "Set game phase to PLAY"},
        {"mvr", "Move robot command: mvr <team> <robot_id> <x> <y> <theta>"},
        {"mvb", "Move ball command: mvb <x> <y>"},
        {"penalize", "Penalize robot command: penalize <team> <robot_id> <penalty_type>. Penalty types: LEAVING_THE_FIELD, PUSHING, FOUL, ILLEGAL_DEFENSE"},
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

    if ((command.rfind("initial", 0) == 0) || (command.rfind("ready", 0) == 0) || (command.rfind("set", 0) == 0) || (command.rfind("play", 0) == 0)) {
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

        Penalty penalty;
        if (penaltyStr == "LEAVING_THE_FIELD") {
            penalty = LEAVING_THE_FIELD;
        } else if (penaltyStr == "PUSHING") {
            penalty = PUSHING;
        } else if (penaltyStr == "FOUL") {
            penalty = FOUL;
        } else if (penaltyStr == "ILLEGAL_DEFENSE") {
            penalty = ILLEGAL_DEFENSE;
        } else {
            return "Invalid penalty type: " + penaltyStr;
        }

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

        return handlePenalizeRobot(team, robotId, NONE_PENALTY);
    }

    return "Unknown command: " + command;
}

std::string GameController::handleGamePhase(std::string phase) {
    if (phase == "initial") {
        currentPhase_ = INITIAL;
    } else if (phase == "ready") {
        currentPhase_ = READY;
    } else if (phase == "set") {
        currentPhase_ = SET;
    } else if (phase == "play") {
        currentPhase_ = PLAY;
    } else {
        return "Invalid game phase: " + phase;
    }

    // Convert phase to uppercase for display
    std::string upperPhase = phase;
    std::transform(upperPhase.begin(), upperPhase.end(), upperPhase.begin(), ::toupper);
    return "Game phase changed to: " + upperPhase;
}

std::string GameController::handleMoveRobot(std::string team, int robotId, double x, double y, double theta) {
    // Validate position bounds
    if (!checkFieldBounds(x, y)) {
        return "Invalid position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    bool teamFound = false;
    bool robotFound = false;
    std::shared_ptr<Robot> targetRobot = nullptr;
    for (TeamInGame& t : teamsInGame_) {
        if (t.getTeam()->name == team) {
            teamFound = true;
            for (std::shared_ptr<Robot> robot : t.getTeam()->robots) {
                if (robot->number == robotId) {
                    robotFound = true;
                    targetRobot = robot;
                    break;
                }
            }
            break;
        }
    }

    if (!teamFound) {
        return "Team '" + team + "' not found.";
    }

    if (!robotFound) {
        return "Robot " + std::to_string(robotId) + " not found in team '" + team + "'.";
    }

    std::string trunkBodyName = targetRobot->name + "_Trunk";
    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, trunkBodyName.c_str());
    if (bodyId < 0) {
        return "Error: Could not find body for robot " + team + "-" + std::to_string(robotId);
    }

    int jntadr = mujContext_->model->body_jntadr[bodyId];
    int qposadr = mujContext_->model->jnt_qposadr[jntadr];

    mujContext_->data->qpos[qposadr + 0] = x;
    mujContext_->data->qpos[qposadr + 1] = y;

    double halfTheta = theta * 0.5;
    mujContext_->data->qpos[qposadr + 3] = std::cos(halfTheta);  // w
    mujContext_->data->qpos[qposadr + 4] = 0;                    // x
    mujContext_->data->qpos[qposadr + 5] = 0;                    // y
    mujContext_->data->qpos[qposadr + 6] = std::sin(halfTheta);  // z
    mj_forward(mujContext_->model, mujContext_->data);

    return "Robot " + team + "-" + std::to_string(robotId) + " moved to (" + std::to_string(x) + ", " + std::to_string(y) + ", "
           + std::to_string(theta) + ")";
}

std::string GameController::handleMoveBall(double x, double y) {
    if (!checkFieldBounds(x, y)) {
        return "Invalid ball position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    std::string bodyName = "ball";
    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, bodyName.c_str());
    if (bodyId < 0) {
        return "Error: Could not find ball body in the simulation.";
    }

    int jntadr = mujContext_->model->body_jntadr[bodyId];
    int qposadr = mujContext_->model->jnt_qposadr[jntadr];

    mujContext_->data->qpos[qposadr + 0] = x;
    mujContext_->data->qpos[qposadr + 1] = y;
    mj_forward(mujContext_->model, mujContext_->data);

    return "Ball moved to (" + std::to_string(x) + ", " + std::to_string(y) + ")";
}

std::string GameController::handlePenalizeRobot(std::string team, int robotId, Penalty penalty) {
    
    for (TeamInGame& t : teamsInGame_) {
        if (t.getTeam()->name == team) {
            RobotInGame rig =  t.getRobotInGame(robotId);
            rig.setPenalized(penalty, gameTime_);
            break;
        }
    }

    return "Penalize command received for robot " + std::to_string(robotId) + " in team '" + team + "'";
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
        if (t.getTeam()->name == "Red") {
            t.setScore(redTeamScore);
        } else if (t.getTeam()->name == "Blue") {
            t.setScore(blueTeamScore);
        }
    }
}

void GameController::update(){

    // Update sim time
    if (mujContext_ && mujContext_->data) {
        simTime_ = mujContext_->data->time;
    }

    if(currentPhase_ == PLAY){
        // Update game time - increment by 1 second when a full second has elapsed
        if (simTime_ - lastGameTimeUpdateSimTime_ >= 1.0) {
            gameTime_ += 1.0;
            lastGameTimeUpdateSimTime_ = simTime_;
        }
        
        // Handle Goal
        std::string scoringTeam = handleGoal();
        if(scoringTeam != "None"){
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
        }

    }

}

bool GameController::checkFieldBounds(double x, double y) {
    return (x > minX && x < maxX && y > minY && y < maxY);
}

}  // namespace spqr
