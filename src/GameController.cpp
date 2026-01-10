#include "GameController.h"

namespace spqr {

std::map<std::string, std::string> GameController::availableCommands() const {
    return {
        {"initial", "Set game phase to INITIAL"},
        {"ready", "Set game phase to READY"},
        {"set", "Set game phase to SET"},
        {"play", "Set game phase to PLAY"},
        {"mvr", "Move robot command: mvr <team> <robot_id> <x> <y> <theta>"},
        {"mvb", "Move ball command: mvb <x> <y>"}
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
    if(!isCommandValid(command)) {
        return "Unknown command: " + command;
    }

    if ((command.rfind("initial", 0) == 0) || 
        (command.rfind("ready", 0) == 0) || 
        (command.rfind("set", 0) == 0) || 
        (command.rfind("play", 0) == 0)) {
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

    return "Unknown command: " + command;
}

std::string GameController::handleGamePhase(std::string phase) {
    if (phase == "initial") {
        currentPhase_ = INITIAL;
    }
    else if (phase == "ready") {
        currentPhase_ = READY;
    }
    else if (phase == "set") {
        currentPhase_ = SET;
    }
    else if (phase == "play") {
        currentPhase_ = PLAY;
    }
    else {
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

    // Validate team exists
    bool teamFound = false;
    bool robotFound = false;
    std::shared_ptr<Team> targetTeam = nullptr;

    for (std::shared_ptr<Team> t : TeamManager::instance().getTeams()) {
        if (t->name == team) {
            teamFound = true;
            targetTeam = t;
            break;
        }
    }

    if (!teamFound) {
        return "Team '" + team + "' not found. Available teams: " + getAvailableTeams();
    }

    // Validate robot exists in the team and move it
    std::shared_ptr<Robot> targetRobot = nullptr;
    for (std::shared_ptr<Robot> robot : targetTeam->robots) {
        if (robot->number == robotId) {
            robotFound = true;
            targetRobot = robot;
            break;
        }
    }

    if (!robotFound) {
        return "Robot " + std::to_string(robotId) + " not found in team '" + team + "'. Available robots: " + getAvailableRobots(targetTeam);
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
    mujContext_->data->qpos[qposadr + 4] = 0;                      // x
    mujContext_->data->qpos[qposadr + 5] = 0;                      // y
    mujContext_->data->qpos[qposadr + 6] = std::sin(halfTheta);  // z   
    mj_forward(mujContext_->model, mujContext_->data);

    return "Robot " + team + "-" + std::to_string(robotId) + " moved to (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(theta) + ")";
}

std::string GameController::handleMoveBall(double x, double y) {
    if (!checkFieldBounds(x, y)) {
        return "Invalid ball position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    // Implementation for moving ball can be added here
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

void GameController::updateSimTime() {
    if (mujContext_ && mujContext_->data) {
        simTime_ = mujContext_->data->time;
    }
}

void GameController::updateGameTime(double time) {
    gameTime_ = time;
}

void GameController::updateScore(int redTeamScore, int blueTeamScore) {
    scoreRedTeam_ = redTeamScore;
    scoreBlueTeam_ = blueTeamScore;
}

std::string GameController::getAvailableTeams() {
    std::string teams = "";
    for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
        if (!teams.empty()) teams += ", ";
        teams += team->name;
    }
    return teams.empty() ? "none" : teams;
}

std::string GameController::getAvailableRobots(std::shared_ptr<Team> team) {
    std::string robots = "";
    for (std::shared_ptr<Robot> robot : team->robots) {
        if (!robots.empty()) robots += ", ";
        robots += std::to_string(robot->number);
    }
    return robots.empty() ? "none" : robots;
}

bool GameController::checkFieldBounds(double x, double y) {
    return (x > minX && x < maxX && y > minY && y < maxY);
}

}