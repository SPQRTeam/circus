#include "CommandConsole.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "GameControllerActions.h"
#include "SceneParser.h"
#include "Team.h"

using namespace RoboCup;

namespace spqr {

namespace {

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Finds the side of the official GameController that a team name refers to. Only the first team
// (ours) is shown, the second one is invisible to the referee.
bool sideOf(const std::string& teamName, Side& side) {
    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    if (!teams.empty() && toLower(teams[0]->name) == teamName) {
        side = Side::home;
        return true;
    }
    if (teams.size() > 1 && toLower(teams[1]->name) == teamName) {
        side = Side::away;
        return true;
    }
    return false;
}

bool penaltyCallOf(const std::string& name, PenaltyCall& call) {
    static const std::map<std::string, PenaltyCall> calls = {
        {"leaving_the_field", PenaltyCall::leavingTheField},
        {"pushing", PenaltyCall::pushing},
        {"illegal_position", PenaltyCall::illegalPosition},
        {"incapable_robot", PenaltyCall::incapableRobot},
        {"motion_in_set", PenaltyCall::motionInSet},
        {"motion_in_stop", PenaltyCall::motionInStop},
        {"ball_holding", PenaltyCall::ballHolding},
        {"local_game_stuck", PenaltyCall::localGameStuck},
        {"pick_up", PenaltyCall::requestForPickUp},
        {"arms_hands", PenaltyCall::playingWithArmsHands},
        {"yellow", PenaltyCall::caution},
        {"red", PenaltyCall::sendOff},
    };
    auto found = calls.find(name);
    if (found == calls.end()) {
        return false;
    }
    call = found->second;
    return true;
}

}  // namespace

void CommandConsole::configure(const SimulationConfig& config) {
    fieldWidth_ = config.field.width;
    fieldHeight_ = config.field.height;
}

void CommandConsole::bindMujoco(MujocoContext* mujContext) {
    mujContext_ = mujContext;
}

std::map<std::string, std::string> CommandConsole::availableCommands() const {
    return {
        {"ready", "Start the kick-off: ready"},
        {"set", "Wait for the set play: set"},
        {"playing", "Free the set play: playing"},
        {"finish", "Finish the half: finish"},
        {"kickin", "Throw-in for a team: kickin <team>"},
        {"cornerkick", "Corner kick for a team: cornerkick <team>"},
        {"goalkick", "Goal kick for a team: goalkick <team>"},
        {"penaltykick", "Penalty kick for a team: penaltykick <team>"},
        {"pushingfreekick", "Direct free kick for a team: pushingfreekick <team>"},
        {"mvr", "Move robot command: mvr <team> <robot_id> <x> <y> <theta> [m, m, deg]"},
        {"mvb", "Move ball command: mvb <x> <y> [m, m]"},
        {"shootb", "Shoot ball command: shootb <x> <y> <speed> <direction> [m, m, m/s, deg]"},
        {"penalize", "Penalize robot command: penalize <team> <robot_id> <penalty_type>. Penalty types: leaving_the_field, pushing, "
                     "illegal_position, incapable_robot, motion_in_set, motion_in_stop, ball_holding, local_game_stuck, pick_up, "
                     "arms_hands, yellow, red"},
        {"unpenalize", "Unpenalize robot command: unpenalize <team> <robot_id>"},
    };
}

bool CommandConsole::isCommandValid(const std::string& command) const {
    for (const auto& [cmd, desc] : availableCommands()) {
        if (command.rfind(cmd, 0) == 0) {
            return true;
        }
    }
    return false;
}

std::string CommandConsole::handleCommand(std::string command) {
    if (!isCommandValid(command)) {
        return "Unknown command: " + command;
    }

    if (command.rfind("mvr", 0) == 0) {
        std::istringstream iss(command);
        std::string cmd, team;
        int robotId;
        double x, y, theta;
        if (!(iss >> cmd >> team >> robotId >> x >> y >> theta)) {
            return "Invalid mvr command format. Usage: mvr <team> <robot_id> <x> <y> <theta>";
        }
        return handleMoveRobot(toLower(team), robotId, x, y, theta);
    }

    if (command.rfind("mvb", 0) == 0) {
        std::istringstream iss(command);
        std::string cmd;
        double x, y;
        iss >> cmd >> x >> y;
        return handleMoveBall(x, y);
    }

    if (command.rfind("shootb", 0) == 0) {
        std::istringstream iss(command);
        std::string cmd;
        double x, y, speed, direction;
        if (!(iss >> cmd >> x >> y >> speed >> direction)) {
            return "Invalid shootb command format. Usage: shootb <x> <y> <speed> <direction>";
        }
        return handleShootBall(x, y, speed, direction);
    }

    return handleReferee(command);
}

std::string CommandConsole::handleReferee(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, team, arg;
    iss >> cmd;

    if (cmd == "ready" || cmd == "set" || cmd == "playing" || cmd == "finish") {
        RoboCupGameControlData data{};
        GameControllerBridge::instance().readControlData(data, true);
        GameControllerAction action = cmd == "ready"   ? gcaction::kickOff(data) :
                                      cmd == "set"     ? gcaction::waitForSetPlay() :
                                      cmd == "playing" ? gcaction::freeSetPlay() :
                                                         gcaction::finishHalf();
        return action.apply() ? "" : "Action not legal: " + cmd;
    }

    if (cmd == "kickin" || cmd == "cornerkick" || cmd == "goalkick" || cmd == "penaltykick" || cmd == "pushingfreekick") {
        iss >> team;
        Side side;
        if (!sideOf(toLower(team), side)) {
            return "Unknown team: " + team;
        }
        static const std::map<std::string, SetPlay> setPlays = {
            {"kickin", SetPlay::throwIn},          {"cornerkick", SetPlay::cornerKick},          {"goalkick", SetPlay::goalKick},
            {"penaltykick", SetPlay::penaltyKick}, {"pushingfreekick", SetPlay::directFreeKick},
        };
        return gcaction::startSetPlay(side, setPlays.at(cmd)).apply() ? "" : "Action not legal: " + cmd;
    }

    if (cmd == "penalize" || cmd == "unpenalize") {
        int robotId = 0;
        if (!(iss >> team >> robotId)) {
            return "Invalid " + cmd + " command format.";
        }
        Side side;
        if (!sideOf(toLower(team), side)) {
            return "Unknown team: " + team;
        }
        if (cmd == "unpenalize") {
            return gcaction::unpenalize(side, robotId, false).apply() ? "" : "Action not legal: unpenalize";
        }
        PenaltyCall call;
        if (!(iss >> arg) || !penaltyCallOf(toLower(arg), call)) {
            return "Invalid penalty type: " + arg;
        }
        const auto& calls = gcaction::penaltyCalls();
        size_t index = 0;
        while (index < calls.size() && calls[index].second != call) {
            ++index;
        }
        return gcaction::penalize(side, robotId, index).apply() ? "" : "Action not legal: penalize";
    }

    return "Unknown command: " + command;
}

bool CommandConsole::checkFieldBounds(double x, double y) const {
    return x >= -fieldWidth_ / 2 && x <= fieldWidth_ / 2 && y >= -fieldHeight_ / 2 && y <= fieldHeight_ / 2;
}

std::string CommandConsole::handleMoveRobot(const std::string& team, int robotId, double x, double y, double theta) {
    if (!checkFieldBounds(x, y)) {
        return "Invalid position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }

    std::shared_ptr<Robot> targetRobot = nullptr;
    for (const std::shared_ptr<Team>& candidate : TeamManager::instance().getTeams()) {
        if (toLower(candidate->name) != team) {
            continue;
        }
        for (const std::shared_ptr<Robot>& robot : candidate->robots) {
            if (robot->number == robotId) {
                targetRobot = robot;
                break;
            }
        }
        break;
    }
    if (!targetRobot) {
        return "Robot " + std::to_string(robotId) + " not found in team '" + team + "'.";
    }

    std::string trunkBodyName = targetRobot->name + "_Trunk";
    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, trunkBodyName.c_str());
    if (bodyId < 0) {
        return "Error: Could not find body '" + trunkBodyName + "'";
    }
    if (mujContext_->model->body_jntnum[bodyId] != 1) {
        return "Error: Robot trunk must have exactly one joint";
    }
    int jntadr = mujContext_->model->body_jntadr[bodyId];
    if (mujContext_->model->jnt_type[jntadr] != mjJNT_FREE) {
        return "Error: Robot trunk joint is not a free joint";
    }
    int qposadr = mujContext_->model->jnt_qposadr[jntadr];
    int qveladr = mujContext_->model->jnt_dofadr[jntadr];
    if (qposadr < 0 || qposadr + 6 >= mujContext_->model->nq) {
        return "Error: Invalid qpos address";
    }

    // Position (keep height), yaw only, zero velocities.
    mjData* data = mujContext_->data;
    data->qpos[qposadr + 0] = x;
    data->qpos[qposadr + 1] = y;
    double halfTheta = 0.5 * theta * M_PI / 180.0;
    double qw = std::cos(halfTheta);
    double qz = std::sin(halfTheta);
    double norm = std::sqrt(qw * qw + qz * qz);
    data->qpos[qposadr + 3] = qw / norm;
    data->qpos[qposadr + 4] = 0.0;
    data->qpos[qposadr + 5] = 0.0;
    data->qpos[qposadr + 6] = qz / norm;
    for (int i = 0; i < 6; ++i) {
        data->qvel[qveladr + i] = 0.0;
    }
    mj_forward(mujContext_->model, data);

    return "Robot " + team + "-" + std::to_string(robotId) + " moved to (" + std::to_string(x) + ", " + std::to_string(y) + ", "
           + std::to_string(theta) + ")";
}

std::string CommandConsole::handleMoveBall(double x, double y) {
    if (!checkFieldBounds(x, y)) {
        return "Invalid ball position (" + std::to_string(x) + ", " + std::to_string(y) + "). Must be within field bounds.";
    }
    int bodyId = mj_name2id(mujContext_->model, mjOBJ_BODY, "ball");
    if (bodyId < 0) {
        return "Error: Could not find ball body in the simulation.";
    }
    int jntadr = mujContext_->model->body_jntadr[bodyId];
    if (mujContext_->model->jnt_type[jntadr] != mjJNT_FREE) {
        return "Error: ball joint is not a free joint";
    }
    int qposadr = mujContext_->model->jnt_qposadr[jntadr];
    int qveladr = mujContext_->model->jnt_dofadr[jntadr];

    mjData* data = mujContext_->data;
    data->qpos[qposadr + 0] = x;
    data->qpos[qposadr + 1] = y;
    for (int i = 0; i < 6; ++i) {
        data->qvel[qveladr + i] = 0.0;
    }
    mj_forward(mujContext_->model, data);

    return "Ball moved to (" + std::to_string(x) + ", " + std::to_string(y) + ")";
}

std::string CommandConsole::handleShootBall(double x, double y, double speed, double direction) {
    // Place the ball first: reuses the bounds and free joint checks, and zeroes its velocity
    std::string result = handleMoveBall(x, y);
    if (result.rfind("Ball moved", 0) != 0) {
        return result;
    }

    const mjModel* model = mujContext_->model;
    mjData* data = mujContext_->data;
    int bodyId = mj_name2id(model, mjOBJ_BODY, "ball");
    int jntadr = model->body_jntadr[bodyId];
    int qposadr = model->jnt_qposadr[jntadr];
    int qveladr = model->jnt_dofadr[jntadr];

    double directionRad = direction * M_PI / 180.0;
    double vx = speed * std::cos(directionRad);
    double vy = speed * std::sin(directionRad);
    data->qvel[qveladr + 0] = vx;
    data->qvel[qveladr + 1] = vy;

    // Angular velocity for rolling without slipping (w = z x v / r), expressed in the body frame.
    double radius = model->geom_rbound[model->body_geomadr[bodyId]];
    mjtNum omegaWorld[3] = {-vy / radius, vx / radius, 0.0};
    mjtNum quatInv[4];
    mju_negQuat(quatInv, data->qpos + qposadr + 3);
    mju_rotVecQuat(data->qvel + qveladr + 3, omegaWorld, quatInv);

    return "Ball shot from (" + std::to_string(x) + ", " + std::to_string(y) + ") at " + std::to_string(speed) + " m/s, direction "
           + std::to_string(direction) + " deg";
}

}  // namespace spqr
