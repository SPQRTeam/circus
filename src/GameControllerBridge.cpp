#include "GameControllerBridge.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#include "CircusNetwork.h"
#include "SceneParser.h"
#include "Team.h"

using namespace RoboCup;

namespace spqr {

namespace {

// Competition whose parameters are used for the simulated games.
constexpr const char* kCompetition = "large_advanced";

// Team number of our team in the league (SPQR Team in config/teams.yaml).
constexpr uint8_t kHomeTeamNumber = 19;
// The opponent is not shown: it is registered as the "Invisibles" team.
constexpr uint8_t kAwayTeamNumber = 0;

// Bits of game_controller_autoreferee::Features that the simulator relies on.
constexpr uint32_t kRefereeFeatures = (1u << 0)      // SWITCH_TO_SET
                                      | (1u << 1)    // SWITCH_TO_PLAYING
                                      | (1u << 2)    // SWITCH_TO_FINISHED
                                      | (1u << 3)    // BALL_OUT
                                      | (1u << 4)    // INVALID_GOAL
                                      | (1u << 16);  // PLACE_BALL: kick-off, penalty kick, shoot-out and all other set plays

constexpr float kRobotRadius = 0.25f;

// TEAM_MESSAGE_PORT_BASE and TEAM_MESSAGE_MAX_SIZE in game_controller_msgs.
constexpr uint16_t kTeamMessagePortBase = 10000;
constexpr size_t kTeamMessageMaxSize = 512;

// Distance between robots placed next to each other for a penalty (as the automatic referee).
constexpr float kPenaltyPlacementDistance = 0.5f;

// How much further out than its spawn position a penalized robot waits.
constexpr float kOutsideDistance = 1.0f;

// Keys of the penalties in params.yaml, indexed by the PENALTY_* values of the control message.
const std::map<uint8_t, const char*> kPenaltyKeys = {
    {PENALTY_ILLEGAL_POSITIONING, "illegalPositioning"},
    {PENALTY_MOTION_IN_SET, "motionInSet"},
    {PENALTY_MOTION_IN_STOP, "motionInStop"},
    {PENALTY_LOCAL_GAME_STUCK, "localGameStuck"},
    {PENALTY_INCAPABLE_ROBOT, "incapableRobot"},
    {PENALTY_PICK_UP, "pickedUp"},
    {PENALTY_BALL_HOLDING, "ballHolding"},
    {PENALTY_LEAVING_THE_FIELD, "leavingTheField"},
    {PENALTY_PLAYING_WITH_ARMS_HANDS, "playingWithArmsHands"},
    {PENALTY_PUSHING, "pushing"},
    {PENALTY_CAUTIONED, "cautioned"},
    {PENALTY_SENT_OFF, "sentOff"},
    {PENALTY_SUBSTITUTE, "substitute"},
};

// Reads the game state for the simulator. The control message lists the teams in the order of the
// sides of the field (the team that defends the left goal first), so they swap at half-time; the
// simulator always gets the home team first. The messages sent to the robots keep the original
// order.
bool readGame(RoboCup::GameController* gameController, bool trueData, RoboCupGameControlData& data) {
    if (!gc_read(gameController, 0, trueData, reinterpret_cast<uint8_t*>(&data), sizeof(data))) {
        return false;
    }
    if (data.teams[0].teamNumber != kHomeTeamNumber) {
        std::swap(data.teams[0], data.teams[1]);
    }
    return true;
}

float normalizeAngle(double angle) {
    return static_cast<float>(std::atan2(std::sin(angle), std::cos(angle)));
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Returns the position in qpos and qvel of a free joint attached to the given body.
bool freeJointAddresses(const mjModel* model, const char* bodyName, int& qposadr, int& qveladr) {
    int bodyId = mj_name2id(model, mjOBJ_BODY, bodyName);
    if (bodyId < 0) {
        return false;
    }
    int jntadr = model->body_jntadr[bodyId];
    if (model->body_jntnum[bodyId] != 1 || model->jnt_type[jntadr] != mjJNT_FREE) {
        return false;
    }
    qposadr = model->jnt_qposadr[jntadr];
    qveladr = model->jnt_dofadr[jntadr];
    return true;
}

}  // namespace

void GameControllerBridge::configure(const SimulationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = std::make_unique<SimulationConfig>(config);
}

void GameControllerBridge::bindMujoco(MujocoContext* mujContext) {
    stopSender();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        destroy();
        mujContext_ = mujContext;
        createGameController();
    }
    startSender();
}

void GameControllerBridge::createGameController() {
    if (!config_ || !mujContext_) {
        return;
    }

    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    if (teams.empty()) {
        std::cerr << "GameController: no teams registered, referee disabled" << std::endl;
        return;
    }
    const std::shared_ptr<Team>& homeTeam = teams[0];

    std::string yaml = readFile(std::string(GC_CONFIG_DIR) + "/" + kCompetition + "/params.yaml");
    if (yaml.empty()) {
        std::cerr << "GameController: cannot read params for " << kCompetition << std::endl;
        return;
    }

    TeamParams home{};
    home.number = kHomeTeamNumber;
    home.fieldPlayerColor = Color::red;
    home.goalkeeperColor = Color::red;

    TeamParams away{};
    away.number = kAwayTeamNumber;
    away.fieldPlayerColor = Color::blue;
    away.goalkeeperColor = Color::yellow;

    TestParams test{};
    test.noDelay = false;
    test.penaltyShootout = false;

    Side kickOffSide = config_->game.first_kickoff_team == homeTeam->name ? Side::home : Side::away;

    params_ = gc_params_new(reinterpret_cast<const uint8_t*>(yaml.data()), yaml.size(), &home, &away, kickOffSide, SideMapping::homeDefendsLeftGoal,
                            test);
    if (!params_) {
        std::cerr << "GameController: " << gc_last_error() << std::endl;
        return;
    }

    gameController_ = gc_new(params_);

    try {
        YAML::Node competition = YAML::Load(yaml);
        for (const auto& [penalty, key] : kPenaltyKeys) {
            incrementalPenalties_[penalty] = competition["penalties"][key]["incremental"].as<bool>(false);
        }
        hasExtraTime_ = competition["extraHalfDuration"] && !competition["extraHalfDuration"].IsNull();

        YAML::Node teamList = YAML::LoadFile(std::string(GC_CONFIG_DIR) + "/teams.yaml");
        for (const YAML::Node& team : teamList) {
            int number = team["number"].as<int>();
            if (number == kHomeTeamNumber) {
                teamNames_[0] = team["name"].as<std::string>();
            }
            if (number == kAwayTeamNumber) {
                teamNames_[1] = team["name"].as<std::string>();
            }
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "GameController: cannot read configuration: " << e.what() << std::endl;
    }

    const FieldConfig& field = config_->field;
    AutorefereeFieldDimensions dimensions{};
    dimensions.length = field.width;
    dimensions.width = field.height;
    dimensions.goalWidth = field.goal_width;
    dimensions.goalHeight = field.goal_height;
    dimensions.goalAreaLength = field.goal_area_width;
    dimensions.goalAreaWidth = field.goal_area_height;
    dimensions.penaltyAreaLength = field.penalty_area_width;
    dimensions.penaltyAreaWidth = field.penalty_area_height;
    dimensions.penaltyMarkDistance = field.penalty_mark_distance;
    dimensions.centerCircleRadius = field.center_radius;
    dimensions.borderStripWidth = 0.7f;
    dimensions.markRadius = 0.05f;
    dimensions.lineWidth = field.line_width;
    dimensions.ballRadius = field.ball_radius;

    AutorefereeConfig refereeConfig{};
    refereeConfig.features.bits = kRefereeFeatures;
    refereeConfig.field = dimensions;
    refereeConfig.timeUntilSwitchToSet = config_->game.ready_phase_duration * 1000ull;
    refereeConfig.timeUntilSwitchToPlaying = config_->game.set_phase_duration * 1000ull;
    refereeConfig.penaltyPlacementDistance = kPenaltyPlacementDistance;
    refereeConfig.userptr = this;
    refereeConfig.placeBall = &GameControllerBridge::placeBall;
    refereeConfig.placePlayer = &GameControllerBridge::placePlayer;
    refereeConfig.whistle = &GameControllerBridge::whistle;
    autoreferee_ = gc_autoreferee_new(&refereeConfig);

    lastSimTime_ = mujContext_->data->time;
    pendingMs_ = 0.0;

    loadFormation();

    lastHalf_ = -1;
    refereeSign_ = -1.f;
    RoboCupGameControlData initial{};
    if (readGame(gameController_, true, initial)) {
        for (int side = 0; side < 2; ++side) {
            for (int player = 0; player < MAX_NUM_PLAYERS; ++player) {
                lastPenalties_[side][player] = initial.teams[side].players[player].penalty;
            }
        }
    }

    // Every simulated robot has a fixed address on the Docker network (see Container.cpp).
    for (const std::shared_ptr<Team>& team : teams) {
        for (const std::shared_ptr<Robot>& robot : team->robots) {
            std::string address = UAN_SEVEN_CIU + std::to_string(team->number) + "." + std::to_string(robot->number + 10);
            in_addr parsed{};
            if (inet_pton(AF_INET, address.c_str(), &parsed) == 1) {
                robotAddresses_.push_back(ntohl(parsed.s_addr));
            }
        }
    }
}

void GameControllerBridge::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!gameController_ || !autoreferee_ || !mujContext_) {
        return;
    }

    const mjModel* model = mujContext_->model;
    const mjData* data = mujContext_->data;

    double elapsedMs = (data->time - lastSimTime_) * 1000.0;
    lastSimTime_ = data->time;
    if (elapsedMs < 0.0) {
        // The simulation was reset: restart the clock of the GameController.
        elapsedMs = 0.0;
    }
    pendingMs_ += elapsedMs;
    auto wholeMs = static_cast<uint64_t>(std::floor(pendingMs_));
    if (wholeMs > 0) {
        gc_seek(gameController_, wholeMs);
        pendingMs_ -= static_cast<double>(wholeMs);
    }

    // The robots never change sides in the simulator (red, the home team, always defends the -x
    // goal, which the framework relies on), while for the GameController the sides switch every
    // half. Its automatic referee has the left goal at +x, so the world is given to it rotated
    // by pi while the home team defends the left goal, and as it is otherwise.
    RoboCupGameControlData game{};
    if (readGame(gameController_, true, game)) {
        bool shootout = game.gamePhase == GAME_PHASE_PENALTY_SHOOT_OUT;
        bool homeDefendsLeftGoal = shootout ? shootoutSides_ == SideMapping::homeDefendsLeftGoal : game.firstHalf;
        refereeSign_ = homeDefendsLeftGoal ? -1.f : 1.f;
    }
    const float sign = refereeSign_;
    const double turn = sign < 0 ? M_PI : 0.0;

    AutorefereeWorld world{};

    int ballQpos = 0;
    int ballQvel = 0;
    if (freeJointAddresses(model, "ball", ballQpos, ballQvel)) {
        world.ball[0] = sign * static_cast<float>(data->qpos[ballQpos + 0]);
        world.ball[1] = sign * static_cast<float>(data->qpos[ballQpos + 1]);
        world.ball[2] = static_cast<float>(data->qpos[ballQpos + 2]);
    }

    // Only our team is observed: the opponent is invisible to the referee.
    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    if (!teams.empty()) {
        for (const std::shared_ptr<Robot>& robot : teams[0]->robots) {
            int qposadr = 0;
            int qveladr = 0;
            if (robot->number < PlayerNumber_MIN || robot->number > PlayerNumber_MAX
                || !freeJointAddresses(model, (robot->name + "_Trunk").c_str(), qposadr, qveladr)) {
                continue;
            }
            // Yaw from the quaternion (w, x, y, z) stored in qpos.
            double w = data->qpos[qposadr + 3];
            double x = data->qpos[qposadr + 4];
            double y = data->qpos[qposadr + 5];
            double z = data->qpos[qposadr + 6];
            AutorefereePlayer& player = world.players[0][robot->number - 1];
            player.present = true;
            player.x = sign * static_cast<float>(data->qpos[qposadr + 0]);
            player.y = sign * static_cast<float>(data->qpos[qposadr + 1]);
            player.theta = normalizeAngle(std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z)) + turn);
            player.radius = kRobotRadius;
        }
    }

    gc_autoreferee_update(autoreferee_, gameController_, &world);
    placePenalizedRobots();
}

double GameControllerBridge::simTime() {
    std::lock_guard<std::mutex> lock(mutex_);
    return mujContext_ ? mujContext_->data->time : 0.0;
}

bool GameControllerBridge::apply(VAction* action, ActionSource source, const GameControllerActionInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!gameController_) {
        gc_action_destroy(action);
        return false;
    }
    if (!gc_apply(gameController_, action, source)) {
        return false;
    }
    if (source != ActionSource::user) {
        return true;
    }
    requestSend();

    using Kind = GameControllerActionInfo::Kind;
    switch (info.kind) {
        case Kind::StopPlay:
            // The core does not record Stop / Resume Play, since its effect persists undo.
            break;
        case Kind::Undo:
            for (uint32_t i = 0; i < info.undoStates && !history_.empty(); ++i) {
                const HistoryEntry& entry = history_.back();
                if (entry.penalizedSide >= 0) {
                    --penaltyCounters_[entry.penalizedSide];
                }
                shootoutSides_ = entry.previousShootoutSides;
                history_.pop_back();
            }
            break;
        default: {
            HistoryEntry entry{info.name, -1, shootoutSides_};
            if (info.kind == Kind::Penalize) {
                // The counter only grows for incremental penalties, which is decided by the penalty
                // that the call resulted in (e.g. a second yellow card becomes a red one).
                RoboCupGameControlData data{};
                if (readGame(gameController_, true, data) && info.player >= 1 && info.player <= MAX_NUM_PLAYERS) {
                    int side = info.side == Side::home ? 0 : 1;
                    uint8_t penalty = data.teams[side].players[info.player - 1].penalty;
                    if (incrementalPenalties_[penalty]) {
                        ++penaltyCounters_[side];
                        entry.penalizedSide = side;
                    }
                }
            } else if (info.kind == Kind::PenaltyShootout) {
                shootoutSides_ = info.sides;
            }
            history_.push_back(entry);
            break;
        }
    }
    return true;
}

std::vector<std::string> GameControllerBridge::undoNames(size_t n) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (auto it = history_.rbegin(); it != history_.rend() && names.size() < n; ++it) {
        names.push_back(it->name);
    }
    return names;
}

std::array<uint32_t, 2> GameControllerBridge::penaltyCounters() {
    std::lock_guard<std::mutex> lock(mutex_);
    return penaltyCounters_;
}

std::string GameControllerBridge::teamName(Side side) {
    std::lock_guard<std::mutex> lock(mutex_);
    return teamNames_[side == Side::home ? 0 : 1];
}

bool GameControllerBridge::hasExtraTime() {
    std::lock_guard<std::mutex> lock(mutex_);
    return hasExtraTime_;
}

SideMapping GameControllerBridge::shootoutSides() {
    std::lock_guard<std::mutex> lock(mutex_);
    return shootoutSides_;
}

bool GameControllerBridge::isLegal(const VAction* action) {
    std::lock_guard<std::mutex> lock(mutex_);
    return gameController_ && gc_is_legal(gameController_, action);
}

bool GameControllerBridge::readControlData(RoboCupGameControlData& data, bool trueData) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!gameController_) {
        return false;
    }
    return readGame(gameController_, trueData, data);
}

// The callbacks receive positions in the frame of the automatic referee (see update).
void GameControllerBridge::placeBall(void* userptr, float x, float y, float z) {
    auto* bridge = static_cast<GameControllerBridge*>(userptr);
    const float sign = bridge->refereeSign_;
    bridge->placeBallInWorld(sign * x, sign * y, z);
}

void GameControllerBridge::placePlayer(void* userptr, Side side, uint8_t player, float x, float y, float theta) {
    auto* bridge = static_cast<GameControllerBridge*>(userptr);
    if (side != Side::home) {
        return;
    }
    // Robot names are resolved by number through the registered team.
    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    if (teams.empty()) {
        return;
    }
    for (const std::shared_ptr<Robot>& robot : teams[0]->robots) {
        if (robot->number == player) {
            const float sign = bridge->refereeSign_;
            bridge->placeRobotInWorld(*robot, sign * x, sign * y, normalizeAngle(theta + (sign < 0 ? M_PI : 0.0)));
            return;
        }
    }
}

void GameControllerBridge::whistle(void* /*userptr*/) {
    // The simulator has no audio whistle: the referee decisions are visible in the GUI only.
}

void GameControllerBridge::placeBallInWorld(float x, float y, float z) {
    if (!mujContext_) {
        return;
    }
    int qposadr = 0;
    int qveladr = 0;
    if (!freeJointAddresses(mujContext_->model, "ball", qposadr, qveladr)) {
        return;
    }
    mjData* data = mujContext_->data;
    data->qpos[qposadr + 0] = x;
    data->qpos[qposadr + 1] = y;
    data->qpos[qposadr + 2] = z;
    for (int i = 0; i < 6; ++i) {
        data->qvel[qveladr + i] = 0.0;
    }
    mj_forward(mujContext_->model, data);
}

// A penalty that removes the robot from the field (all but Motion in Set), and a player that is
// substituted out, send the robot to wait one metre further out than where it was spawned at the
// side of the field, facing the field: every robot has its own spot, so penalized robots do not
// run into each other. Only when the penalty is removed in the GameController (robots may come
// back only then, not when their penalty time is up) is the robot put back on the touchline at
// ReadyFormation::penalizedPose, where the localization of the framework is reset when its
// penalty is over.
void GameControllerBridge::placePenalizedRobots() {
    RoboCupGameControlData data{};
    if (!readGame(gameController_, true, data)) {
        return;
    }
    // In a penalty shoot-out the players are benched and selected by the GameController.
    bool shootout = data.gamePhase == GAME_PHASE_PENALTY_SHOOT_OUT;

    // A new half (or extra half) has started. Every penalty except sent off and substitute has
    // just been removed, but the robots do not come back from the touchline: they all start the
    // half from where they were spawned (see placeForNewHalf).
    if (data.gamePhase == GAME_PHASE_NORMAL || data.gamePhase == GAME_PHASE_EXTRA_TIME) {
        int half = (data.gamePhase == GAME_PHASE_EXTRA_TIME ? 2 : 0) + (data.firstHalf ? 0 : 1);
        if (half != lastHalf_) {
            bool newHalf = lastHalf_ >= 0;
            lastHalf_ = half;
            if (newHalf) {
                placeForNewHalf(data);
                return;
            }
        }
    }

    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    std::vector<std::pair<float, float>> placed;
    for (int side = 0; side < 2; ++side) {
        for (int player = 0; player < MAX_NUM_PLAYERS; ++player) {
            uint8_t penalty = data.teams[side].players[player].penalty;
            uint8_t last = lastPenalties_[side][player];
            lastPenalties_[side][player] = penalty;
            if (shootout || penalty == last || side >= static_cast<int>(teams.size())) {
                continue;
            }

            bool removed = (last == PENALTY_NONE && penalty != PENALTY_MOTION_IN_SET) || (last == PENALTY_SUBSTITUTE);
            bool returning = penalty == PENALTY_NONE && last != PENALTY_MOTION_IN_SET;
            for (const std::shared_ptr<Robot>& robot : teams[side]->robots) {
                if (robot->number != player + 1) {
                    continue;
                }
                if (returning) {
                    Pose pose = penalizedPose_.value_or(Pose{-(config_->field.width * 0.5f - config_->field.penalty_mark_distance),
                                                             -config_->field.height * 0.5f, static_cast<float>(M_PI_2)});
                    placeOnTouchline(*robot, pose.x, placed);
                } else if (removed) {
                    placeOutside(*robot);
                }
            }
        }
    }
}

void GameControllerBridge::placeOutside(const Robot& robot) {
    // The spawn position of the scene is in the world frame. Robots are spawned outside the
    // touchline; a robot spawned inside the field waits one metre beyond the touchline of its
    // team instead (at -y of the team frame: -y for red, +y for blue).
    const float halfWidth = config_->field.height * 0.5f;
    const float spawnX = static_cast<float>(robot.initPosition.x());
    const float spawnY = static_cast<float>(robot.initPosition.y());
    const float outwards = spawnY != 0.f ? std::copysign(1.f, spawnY) : (robot.colorName == "blue" ? 1.f : -1.f);
    const float y = std::abs(spawnY) > halfWidth ? spawnY + outwards * kOutsideDistance : outwards * (halfWidth + kOutsideDistance);
    const float theta = outwards < 0 ? static_cast<float>(M_PI_2) : -static_cast<float>(M_PI_2);
    placeRobotInWorld(robot, spawnX, y, theta);
}

// The sides of the field switch at half-time, but turning the robots around would break the
// framework, whose field frame in simulation is fixed by the team color (and which, in fast
// simulation, is given the world pose of the robot). The field is symmetric, so the robots are
// put back where they started the game instead: the framework resets its pose to that same
// initial pose when the new half goes to Ready.
void GameControllerBridge::placeForNewHalf(const RoboCupGameControlData& data) {
    std::vector<std::shared_ptr<Team>> teams = TeamManager::instance().getTeams();
    for (int side = 0; side < 2; ++side) {
        for (int player = 0; player < MAX_NUM_PLAYERS; ++player) {
            lastPenalties_[side][player] = data.teams[side].players[player].penalty;
        }
        if (side >= static_cast<int>(teams.size())) {
            continue;
        }
        for (const std::shared_ptr<Robot>& robot : teams[side]->robots) {
            if (robot->number < 1 || robot->number > MAX_NUM_PLAYERS) {
                continue;
            }
            uint8_t penalty = data.teams[side].players[robot->number - 1].penalty;
            if (penalty == PENALTY_SUBSTITUTE || penalty == PENALTY_SENT_OFF) {
                continue;
            }
            placeRobotInWorld(*robot, static_cast<float>(robot->initPosition.x()), static_cast<float>(robot->initPosition.y()),
                              static_cast<float>(robot->initOrientation.z()));
        }
    }
    placeBallInWorld(0.f, 0.f, config_->field.ball_radius);
}

void GameControllerBridge::placeInTeamFrame(const Robot& robot, float x, float y, float theta) {
    // In simulation the red team plays towards +x (its frame is the world), the blue one towards
    // -x (its frame is the world rotated by pi), as in ReadyFormation of the framework.
    if (robot.colorName == "blue") {
        placeRobotInWorld(robot, -x, -y, std::atan2(std::sin(theta + M_PI), std::cos(theta + M_PI)));
    } else {
        placeRobotInWorld(robot, x, y, theta);
    }
}

void GameControllerBridge::placeOnTouchline(const Robot& robot, float teamX, std::vector<std::pair<float, float>>& occupied) {
    const FieldConfig& field = config_->field;
    const float halfLength = field.width * 0.5f;
    // The touchline at -y of the team frame, facing the field (ReadyFormation::penalizedPose).
    const float teamY = penalizedPose_ ? penalizedPose_->y : -field.height * 0.5f;
    const float teamTheta = penalizedPose_ ? penalizedPose_->theta : static_cast<float>(M_PI_2);
    const float sign = robot.colorName == "blue" ? -1.f : 1.f;

    // Spots taken by the other robots and by those just placed (world frame).
    const mjModel* model = mujContext_->model;
    const mjData* data = mujContext_->data;
    for (const std::shared_ptr<Team>& team : TeamManager::instance().getTeams()) {
        for (const std::shared_ptr<Robot>& other : team->robots) {
            int qposadr = 0;
            int qveladr = 0;
            if (other.get() != &robot && freeJointAddresses(model, (other->name + "_Trunk").c_str(), qposadr, qveladr)) {
                occupied.emplace_back(static_cast<float>(data->qpos[qposadr]), static_cast<float>(data->qpos[qposadr + 1]));
            }
        }
    }
    auto clearance = [&](float x) {
        float nearest = std::numeric_limits<float>::infinity();
        for (const auto& [ox, oy] : occupied) {
            nearest = std::min(nearest, std::hypot(ox - sign * x, oy - sign * teamY));
        }
        return nearest;
    };

    // Candidates as in rules::penalty_placement_candidates: from teamX, alternately towards the
    // own goal line and the halfway line, within the own half.
    const float min = -halfLength + kRobotRadius;
    const float max = -kRobotRadius;
    std::vector<float> candidates{std::clamp(teamX, min, max)};
    for (int k = 1;; ++k) {
        float towardsGoalLine = candidates[0] - k * kPenaltyPlacementDistance;
        float towardsHalfwayLine = candidates[0] + k * kPenaltyPlacementDistance;
        if (towardsGoalLine < min && towardsHalfwayLine > max) {
            break;
        }
        if (towardsGoalLine >= min) {
            candidates.push_back(towardsGoalLine);
        }
        if (towardsHalfwayLine <= max) {
            candidates.push_back(towardsHalfwayLine);
        }
    }
    // If every spot is taken, the one with the most room is used.
    float best = candidates[0];
    float bestClearance = -1.f;
    for (float x : candidates) {
        float room = clearance(x);
        if (room >= kPenaltyPlacementDistance) {
            best = x;
            break;
        }
        if (room > bestClearance) {
            bestClearance = room;
            best = x;
        }
    }

    placeInTeamFrame(robot, best, teamY, teamTheta);
    occupied.emplace_back(sign * best, sign * teamY);
}

// Same file as ReadyFormation: fieldIO -> formationIO.yaml in the config of the framework.
void GameControllerBridge::loadFormation() {
    penalizedPose_.reset();

    const char* root = std::getenv("FRAMEWORK_PATH") ? std::getenv("FRAMEWORK_PATH") : std::getenv("PIXI_PROJECT_ROOT");
    if (!root) {
        std::cerr << "GameController: framework not found, returning robots are placed from the field geometry" << std::endl;
        return;
    }
    const std::string& field = config_->game.field;
    const std::string suffix = field.rfind("field", 0) == 0 ? field.substr(5) : field;
    const std::string path = std::string(root) + "/src/app/config/formations/formation" + suffix + ".yaml";
    try {
        YAML::Node formation = YAML::LoadFile(path);
        const YAML::Node pose = formation["penalizedPose"];
        penalizedPose_ = Pose{pose[0].as<float>(), pose[1].as<float>(), pose[2].as<float>()};
    } catch (const YAML::Exception& e) {
        std::cerr << "GameController: cannot read " << path << " (" << e.what() << "), returning robots are placed from the field geometry"
                  << std::endl;
    }
}

void GameControllerBridge::placeRobotInWorld(const Robot& robot, float x, float y, float theta) {
    if (!mujContext_) {
        return;
    }
    int qposadr = 0;
    int qveladr = 0;
    if (!freeJointAddresses(mujContext_->model, (robot.name + "_Trunk").c_str(), qposadr, qveladr)) {
        return;
    }
    mjData* data = mujContext_->data;
    // Upright at the height it was spawned at (it may have fallen), facing theta, standing still.
    data->qpos[qposadr + 0] = x;
    data->qpos[qposadr + 1] = y;
    data->qpos[qposadr + 2] = robot.initPosition.z();
    double halfTheta = 0.5 * static_cast<double>(theta);
    data->qpos[qposadr + 3] = std::cos(halfTheta);
    data->qpos[qposadr + 4] = 0.0;
    data->qpos[qposadr + 5] = 0.0;
    data->qpos[qposadr + 6] = std::sin(halfTheta);
    for (int i = 0; i < 6; ++i) {
        data->qvel[qveladr + i] = 0.0;
    }
    mj_forward(mujContext_->model, data);
}

void GameControllerBridge::destroy() {
    if (autoreferee_) {
        gc_autoreferee_destroy(autoreferee_);
        autoreferee_ = nullptr;
    }
    if (gameController_) {
        gc_destroy(gameController_);
        gameController_ = nullptr;
    }
    if (params_) {
        gc_params_destroy(params_);
        params_ = nullptr;
    }
    lastSimTime_ = 0.0;
    pendingMs_ = 0.0;
    history_.clear();
    robotAddresses_.clear();
    packetNumber_ = 0;
    penaltyCounters_ = {0, 0};
    shootoutSides_ = SideMapping::homeDefendsLeftGoal;
}

GameControllerBridge::~GameControllerBridge() {
    stopSender();
    std::lock_guard<std::mutex> lock(mutex_);
    destroy();
}

void GameControllerBridge::startSender() {
    {
        std::lock_guard<std::mutex> lock(aliveMutex_);
        lastAlive_ = {};
    }
    if (statusSocket_ < 0) {
        statusSocket_ = openReceiver(GAMECONTROLLER_RETURN_PORT, "status messages");
    }
    // Team messages are broadcast on the port of each team (TEAM_MESSAGE_PORT_BASE + team number).
    const std::array<uint8_t, 2> teamNumbers{kHomeTeamNumber, kAwayTeamNumber};
    for (int side = 0; side < 2; ++side) {
        if (teamSockets_[side] < 0) {
            teamSockets_[side] = openReceiver(kTeamMessagePortBase + teamNumbers[side], "team messages");
        }
    }
    if (statusSocket_ >= 0 || teamSockets_[0] >= 0 || teamSockets_[1] >= 0) {
        receiverRunning_ = true;
        receiver_ = std::thread(&GameControllerBridge::receiverLoop, this);
    }

    if (socket_ < 0) {
        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_ < 0) {
            std::cerr << "GameController: cannot create the socket for the control messages" << std::endl;
            return;
        }
    }
    std::lock_guard<std::mutex> lock(senderMutex_);
    senderRunning_ = true;
    sendNow_ = false;
    sender_ = std::thread(&GameControllerBridge::senderLoop, this);
}

void GameControllerBridge::stopSender() {
    receiverRunning_ = false;
    if (receiver_.joinable()) {
        receiver_.join();
    }
    {
        std::lock_guard<std::mutex> lock(senderMutex_);
        senderRunning_ = false;
    }
    senderWakeup_.notify_all();
    if (sender_.joinable()) {
        sender_.join();
    }
}

void GameControllerBridge::requestSend() {
    {
        std::lock_guard<std::mutex> lock(senderMutex_);
        sendNow_ = true;
    }
    senderWakeup_.notify_all();
}

void GameControllerBridge::senderLoop() {
    while (true) {
        bool stopped = sendControlMessage();
        // As the official GameController: 2 Hz, raised to 5 Hz while play is stopped.
        std::unique_lock<std::mutex> lock(senderMutex_);
        senderWakeup_.wait_for(lock, std::chrono::milliseconds(stopped ? 200 : 500), [this]() { return !senderRunning_ || sendNow_; });
        if (!senderRunning_) {
            return;
        }
        sendNow_ = false;
    }
}

int GameControllerBridge::openReceiver(uint16_t port, const char* what) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0) {
        std::cerr << "GameController: cannot receive " << what << ", port " << port << " is in use (is another GameController running?)" << std::endl;
        close(fd);
        return -1;
    }
    return fd;
}

void GameControllerBridge::receiverLoop() {
    // One more byte than a team message may have, to recognize the ones that are too long.
    std::vector<uint8_t> buffer(kTeamMessageMaxSize + 1);
    while (receiverRunning_) {
        std::array<pollfd, 3> fds{};
        nfds_t count = 0;
        for (int fd : {statusSocket_, teamSockets_[0], teamSockets_[1]}) {
            if (fd >= 0) {
                fds[count++] = pollfd{fd, POLLIN, 0};
            }
        }
        // A short timeout lets the receiver notice when it has to stop.
        if (poll(fds.data(), count, 200) <= 0) {
            continue;
        }
        for (nfds_t i = 0; i < count; ++i) {
            if (!(fds[i].revents & POLLIN)) {
                continue;
            }
            ssize_t length = recv(fds[i].fd, buffer.data(), buffer.size(), 0);
            if (length < 0) {
                continue;
            }
            if (fds[i].fd == statusSocket_) {
                receiveStatusMessage(buffer.data(), static_cast<size_t>(length));
            } else {
                receiveTeamMessage(fds[i].fd == teamSockets_[0] ? Side::home : Side::away, static_cast<size_t>(length));
            }
        }
    }
}

// As the runtime of the GameController: every team message is applied as a network action, which
// uses up the message budget of the team during Ready, Set and Playing, and a message that is too
// long is illegal communication.
void GameControllerBridge::receiveTeamMessage(Side side, size_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gameController_) {
        gc_apply(gameController_, gc_action_team_message(side, length > kTeamMessageMaxSize), ActionSource::network);
    }
}

// Accepts the status messages that the official GameController accepts (StatusMessage::try_from
// in game_controller_msgs) and records when each player was last heard of.
void GameControllerBridge::receiveStatusMessage(const uint8_t* buffer, size_t length) {
    if (length != sizeof(RoboCupGameControlReturnData)) {
        return;
    }
    RoboCupGameControlReturnData message;
    std::memcpy(&message, buffer, sizeof(message));
    if (std::memcmp(message.header, GAMECONTROLLER_RETURN_STRUCT_HEADER, 4) != 0 || message.version != GAMECONTROLLER_RETURN_STRUCT_VERSION
        || message.playerNum < 1 || message.playerNum > MAX_NUM_PLAYERS || message.fallen > 1) {
        return;
    }
    bool finite = !std::isnan(message.ballAge);
    for (float value : {message.pose[0], message.pose[1], message.pose[2], message.ball[0], message.ball[1]}) {
        finite = finite && !std::isnan(value);
    }
    if (!finite) {
        return;
    }
    // The side is found from the team number, the home team first (Params::get_side).
    int side = message.teamNum == kHomeTeamNumber ? 0 : message.teamNum == kAwayTeamNumber ? 1 : -1;
    if (side < 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(aliveMutex_);
    lastAlive_[side][message.playerNum - 1] = std::chrono::steady_clock::now();
}

std::array<std::array<int, MAX_NUM_PLAYERS>, 2> GameControllerBridge::connectionStatus() {
    // CONNECTION_STATUS_TIMEOUT_GOOD / _BAD in game_controller_runtime.
    constexpr auto good = std::chrono::seconds(2);
    constexpr auto bad = std::chrono::seconds(4);
    std::array<std::array<int, MAX_NUM_PLAYERS>, 2> status{};
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(aliveMutex_);
    for (int side = 0; side < 2; ++side) {
        for (int player = 0; player < MAX_NUM_PLAYERS; ++player) {
            const auto& alive = lastAlive_[side][player];
            if (alive == std::chrono::steady_clock::time_point{}) {
                continue;
            }
            auto age = now - alive;
            status[side][player] = age <= good ? 2 : age <= bad ? 1 : 0;
        }
    }
    return status;
}

bool GameControllerBridge::sendControlMessage() {
    RoboCupGameControlData data{};
    std::vector<uint32_t> addresses;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!gameController_ || robotAddresses_.empty()) {
            return false;
        }
        // The players receive the possibly delayed state, not the true one.
        if (!gc_read(gameController_, packetNumber_++, false, reinterpret_cast<uint8_t*>(&data), sizeof(data))) {
            return false;
        }
        addresses = robotAddresses_;
    }

    for (uint32_t address : addresses) {
        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_port = htons(GAMECONTROLLER_DATA_PORT);
        target.sin_addr.s_addr = htonl(address);
        sendto(socket_, &data, sizeof(data), 0, reinterpret_cast<sockaddr*>(&target), sizeof(target));
    }
    return data.stopped;
}

}  // namespace spqr
