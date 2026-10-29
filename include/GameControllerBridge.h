#pragma once

#include <GameController.h>
#include <RoboCupGameControlData.h>

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "MujocoContext.h"
#include "SceneParser.h"
#include "robots/Robot.h"

namespace spqr {

// Describes an action applied by the user. The C API does not expose the undo history nor the
// penalty counters, so the bridge keeps them the same way the core does: every user action
// except Stop/Resume Play is recorded, and undoing n states drops the last n entries.
struct GameControllerActionInfo {
        enum class Kind { Other, StopPlay, Undo, Penalize, PenaltyShootout };

        // Label shown on the undo button for this action (as in the React frontend).
        std::string name;
        Kind kind = Kind::Other;
        uint32_t undoStates = 0;
        RoboCup::Side side = RoboCup::Side::home;
        uint8_t player = 0;
        RoboCup::SideMapping sides = RoboCup::SideMapping::homeDefendsLeftGoal;
};

// Runs the official GameController (external/GameController, C API) inside the simulator.
// The simulated world is fed to the automatic referee, which applies its decisions to the
// GameController and moves the robots and the ball through the MuJoCo context.
class GameControllerBridge {
    public:
        static GameControllerBridge& instance() {
            static GameControllerBridge bridge;
            return bridge;
        }

        // Stores the field and timing configuration used when the GameController is created.
        void configure(const SimulationConfig& config);

        // Creates the GameController for the teams currently registered in TeamManager.
        void bindMujoco(MujocoContext* mujContext);

        // Advances the GameController and the automatic referee by one simulation step.
        void update();

        // Simulation time in seconds, or 0 if no simulation is bound.
        double simTime();

        // Applies an action (takes ownership of it). Returns whether it was applied. User actions
        // are recorded for the undo buttons as described by info.
        bool apply(RoboCup::VAction* action, RoboCup::ActionSource source, const GameControllerActionInfo& info = {});

        // Names of the last n user actions that can be undone, most recent first.
        std::vector<std::string> undoNames(size_t n);

        // Number of incremental penalties of each team (index 0 = home, 1 = away).
        std::array<uint32_t, 2> penaltyCounters();

        // Name of a team as listed in config/teams.yaml of the GameController.
        std::string teamName(RoboCup::Side side);

        // Whether the competition has extra time.
        bool hasExtraTime();

        // Side mapping chosen for the penalty shoot-out.
        RoboCup::SideMapping shootoutSides();

        // Connection status of each player as shown by the GameController, from the status
        // messages the robots send back: 2 = good, 1 = bad, 0 = offline. Indexed by side
        // (0 = home, 1 = away) and player number - 1.
        std::array<std::array<int, MAX_NUM_PLAYERS>, 2> connectionStatus();

        // Checks an action without applying it (does not take ownership of it).
        bool isLegal(const RoboCup::VAction* action);

        // Copies the control message describing the game state, with the home team always in
        // teams[0] (the message itself orders them by side of the field). If trueData is set, the
        // true state is returned, otherwise the (possibly delayed) state sent to the players.
        bool readControlData(RoboCupGameControlData& data, bool trueData);

    private:
        GameControllerBridge() = default;
        ~GameControllerBridge();
        GameControllerBridge(const GameControllerBridge&) = delete;
        GameControllerBridge& operator=(const GameControllerBridge&) = delete;

        static void placeBall(void* userptr, float x, float y, float z);
        static void placePlayer(void* userptr, RoboCup::Side side, uint8_t player, float x, float y, float theta);
        static void whistle(void* userptr);

        void destroy();
        void createGameController();

        // Sends the control message to the robots like the official GameController: periodically,
        // and immediately after the user applied an action.
        void startSender();
        void stopSender();
        void senderLoop();
        bool sendControlMessage();
        void requestSend();

        // Receives the status messages of the robots on GAMECONTROLLER_RETURN_PORT.
        void receiverLoop();

        void placeBallInWorld(float x, float y, float z);
        void placeRobotInWorld(const Robot& robot, float x, float y, float theta);

        // Moves the robots whose penalty just started or ended (see placePenalizedRobots).
        void placePenalizedRobots();
        // Puts every robot back at its spawn pose and the ball on the center mark.
        void placeForNewHalf(const RoboCupGameControlData& data);
        // Places a penalized robot outside the field, next to where it was spawned.
        void placeOutside(const Robot& robot);
        // Pose in the frame of the robot's team (own goal at -x) -> pose in the world.
        void placeInTeamFrame(const Robot& robot, float x, float y, float theta);
        // Places the robot on the touchline at the first free spot from teamX (team frame).
        void placeOnTouchline(const Robot& robot, float teamX, std::vector<std::pair<float, float>>& occupied);
        void loadFormation();

        // penalizedPose [x, y, theta] in the team frame, read from the formation of the framework
        // for the field in use (src/app/config/formations), as ReadyFormation does.
        struct Pose {
                float x, y, theta;
        };
        std::optional<Pose> penalizedPose_;

        std::mutex mutex_;
        MujocoContext* mujContext_ = nullptr;
        std::unique_ptr<SimulationConfig> config_;

        RoboCup::Params* params_ = nullptr;
        RoboCup::GameController* gameController_ = nullptr;
        RoboCup::Autoreferee* autoreferee_ = nullptr;

        // World of the simulator -> frame of the automatic referee: -1 = rotated by pi, 1 = same.
        float refereeSign_ = -1.f;
        // Half being played (0/1 = first/second half, 2/3 = extra halves), -1 if not known yet.
        int lastHalf_ = -1;

        // Penalty of every player in the last step (index: side, player number - 1).
        std::array<std::array<uint8_t, MAX_NUM_PLAYERS>, 2> lastPenalties_{};

        double lastSimTime_ = 0.0;
        double pendingMs_ = 0.0;

        struct HistoryEntry {
                std::string name;
                int penalizedSide = -1;
                RoboCup::SideMapping previousShootoutSides = RoboCup::SideMapping::homeDefendsLeftGoal;
        };
        std::vector<HistoryEntry> history_;
        std::array<uint32_t, 2> penaltyCounters_{0, 0};
        RoboCup::SideMapping shootoutSides_ = RoboCup::SideMapping::homeDefendsLeftGoal;

        // Loaded from the competition params and the team list of the GameController.
        std::map<uint8_t, bool> incrementalPenalties_;
        bool hasExtraTime_ = false;
        std::array<std::string, 2> teamNames_;

        // Addresses (host byte order) of the simulated robots on the Docker network.
        std::vector<uint32_t> robotAddresses_;
        uint8_t packetNumber_ = 0;
        int socket_ = -1;
        std::thread sender_;
        std::mutex senderMutex_;
        std::condition_variable senderWakeup_;
        bool senderRunning_ = false;
        bool sendNow_ = false;

        int statusSocket_ = -1;
        std::thread receiver_;
        std::atomic<bool> receiverRunning_{false};
        std::mutex aliveMutex_;
        // Time of the last valid status message of each player (default value: never).
        std::array<std::array<std::chrono::steady_clock::time_point, MAX_NUM_PLAYERS>, 2> lastAlive_{};
};

}  // namespace spqr
