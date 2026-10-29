#pragma once

#include <map>
#include <string>

#include "MujocoContext.h"

namespace spqr {

struct SimulationConfig;

// Text commands of the sidebar console. Simulator commands (mvr, mvb, shootb) act on MuJoCo
// directly; referee commands are translated into actions of the official GameController.
class CommandConsole {
    public:
        static CommandConsole& instance() {
            static CommandConsole console;
            return console;
        }

        void configure(const SimulationConfig& config);
        void bindMujoco(MujocoContext* mujContext);

        std::map<std::string, std::string> availableCommands() const;
        bool isCommandValid(const std::string& command) const;
        std::string handleCommand(std::string command);

    private:
        CommandConsole() = default;
        CommandConsole(const CommandConsole&) = delete;
        CommandConsole& operator=(const CommandConsole&) = delete;

        bool checkFieldBounds(double x, double y) const;
        std::string handleMoveRobot(const std::string& team, int robotId, double x, double y, double theta);
        std::string handleMoveBall(double x, double y);
        std::string handleShootBall(double x, double y, double speed, double direction);
        std::string handleReferee(const std::string& command);

        MujocoContext* mujContext_ = nullptr;
        double fieldWidth_ = 0.0;
        double fieldHeight_ = 0.0;
};

}  // namespace spqr
