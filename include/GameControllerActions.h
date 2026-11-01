#pragma once

#include <GameController.h>

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "GameControllerBridge.h"

namespace spqr {

// An action of the official GameController together with what the bridge records about it.
// The action itself is created on demand, so the same GameControllerAction can be checked for
// legality and applied any number of times.
struct GameControllerAction {
        std::function<RoboCup::VAction*()> make;
        GameControllerActionInfo info;

        bool isLegal() const {
            RoboCup::VAction* action = make();
            bool legal = GameControllerBridge::instance().isLegal(action);
            RoboCup::gc_action_destroy(action);
            return legal;
        }

        bool apply() const {
            return GameControllerBridge::instance().apply(make(), RoboCup::ActionSource::user, info);
        }
};

// The actions of the GameController, named as in the undo panel of the React frontend.
namespace gcaction {

using Kind = GameControllerActionInfo::Kind;

inline GameControllerAction make(std::function<RoboCup::VAction*()> make, std::string name, Kind kind = Kind::Other) {
    GameControllerAction action{std::move(make), {}};
    action.info.name = std::move(name);
    action.info.kind = kind;
    return action;
}

// Penalty calls in the order of the penalty panel, with their labels.
inline const std::vector<std::pair<std::string, RoboCup::PenaltyCall>>& penaltyCalls() {
    static const std::vector<std::pair<std::string, RoboCup::PenaltyCall>> calls = {
        {"Pushing", RoboCup::PenaltyCall::pushing},
        {"Incapable Robot", RoboCup::PenaltyCall::incapableRobot},
        {"Leaving the Field", RoboCup::PenaltyCall::leavingTheField},
        {"Illegal Position", RoboCup::PenaltyCall::illegalPosition},
        {"Motion in Set", RoboCup::PenaltyCall::motionInSet},
        {"Motion in Stop", RoboCup::PenaltyCall::motionInStop},
        {"Ball Holding", RoboCup::PenaltyCall::ballHolding},
        {"Local Game Stuck", RoboCup::PenaltyCall::localGameStuck},
        {"Pick-up", RoboCup::PenaltyCall::requestForPickUp},
        {"Arms / Hands", RoboCup::PenaltyCall::playingWithArmsHands},
        {"Yellow Card", RoboCup::PenaltyCall::caution},
        {"Red Card", RoboCup::PenaltyCall::sendOff},
    };
    return calls;
}

inline std::string setPlayName(RoboCup::SetPlay setPlay) {
    switch (setPlay) {
        case RoboCup::SetPlay::kickOff:
            return "Ready";
        case RoboCup::SetPlay::directFreeKick:
            return "Direct Free Kick";
        case RoboCup::SetPlay::indirectFreeKick:
            return "Indirect Free Kick";
        case RoboCup::SetPlay::penaltyKick:
            return "Penalty Kick";
        case RoboCup::SetPlay::throwIn:
            return "Throw-in";
        case RoboCup::SetPlay::goalKick:
            return "Goal Kick";
        case RoboCup::SetPlay::cornerKick:
            return "Corner Kick";
        default:
            return "startSetPlay";
    }
}

inline GameControllerAction stopPlay(bool resume) {
    return make([resume]() { return RoboCup::gc_action_stop_play(resume); }, resume ? "Resume Play" : "Stop Play", Kind::StopPlay);
}

inline GameControllerAction undo(uint32_t states) {
    GameControllerAction action = make([states]() { return RoboCup::gc_action_undo(states); }, "Undo", Kind::Undo);
    action.info.undoStates = states;
    return action;
}

inline GameControllerAction startSetPlay(std::optional<RoboCup::Side> side, RoboCup::SetPlay setPlay) {
    return make(
        [side, setPlay]() {
            RoboCup::Side value = side.value_or(RoboCup::Side::home);
            return RoboCup::gc_action_start_set_play(side ? &value : nullptr, setPlay);
        },
        setPlayName(setPlay));
}

// The team that takes the next kick-off, as reported by the control message.
inline std::optional<RoboCup::Side> kickingSide(const RoboCupGameControlData& data) {
    if (data.kickingTeam == KICKING_TEAM_NONE) {
        return std::nullopt;
    }
    return data.teams[0].teamNumber == data.kickingTeam ? RoboCup::Side::home : RoboCup::Side::away;
}

// The kick-off is only legal for the team that takes it (in the second half the other one), so
// the team must always be given: a kick-off without a team is never legal.
inline GameControllerAction kickOff(const RoboCupGameControlData& data) {
    return startSetPlay(kickingSide(data), RoboCup::SetPlay::kickOff);
}

inline GameControllerAction timeout(std::optional<RoboCup::Side> side) {
    return make(
        [side]() {
            RoboCup::Side value = side.value_or(RoboCup::Side::home);
            return RoboCup::gc_action_timeout(side ? &value : nullptr);
        },
        side ? "Timeout" : "Referee Timeout");
}

inline GameControllerAction goal(RoboCup::Side side) {
    return make([side]() { return RoboCup::gc_action_goal(side); }, "Goal");
}

inline GameControllerAction penalize(RoboCup::Side side, uint8_t player, size_t callIndex) {
    const auto& call = penaltyCalls()[callIndex];
    RoboCup::PenaltyCall penaltyCall = call.second;
    GameControllerAction action
        = make([side, player, penaltyCall]() { return RoboCup::gc_action_penalize(side, player, penaltyCall); }, call.first, Kind::Penalize);
    action.info.side = side;
    action.info.player = player;
    return action;
}

inline GameControllerAction unpenalize(RoboCup::Side side, uint8_t player, bool force) {
    return make([side, player, force]() { return RoboCup::gc_action_unpenalize(side, player, force); }, "Unpenalize");
}

inline GameControllerAction selectGoalkeeper(RoboCup::Side side, uint8_t player) {
    return make([side, player]() { return RoboCup::gc_action_select_goalkeeper(side, player); }, "Select");
}

inline GameControllerAction selectPenaltyShotPlayer(RoboCup::Side side, uint8_t player, bool goalkeeper) {
    return make([side, player, goalkeeper]() { return RoboCup::gc_action_select_penalty_shot_player(side, player, goalkeeper); }, "Select");
}

inline GameControllerAction substitute(RoboCup::Side side, uint8_t playerOut, uint8_t playerIn) {
    return make([side, playerOut, playerIn]() { return RoboCup::gc_action_substitute(side, playerOut, playerIn); }, "Substitute");
}

inline GameControllerAction switchHalf() {
    return make([]() { return RoboCup::gc_action_switch_half(); }, "Second Half");
}

inline GameControllerAction startExtraTime() {
    return make([]() { return RoboCup::gc_action_start_extra_time(); }, "Extra Time");
}

inline GameControllerAction startPenaltyShootout(RoboCup::SideMapping sides) {
    GameControllerAction action
        = make([sides]() { return RoboCup::gc_action_start_penalty_shootout(sides); }, "Penalty Shoot-out", Kind::PenaltyShootout);
    action.info.sides = sides;
    return action;
}

inline GameControllerAction waitForSetPlay() {
    return make([]() { return RoboCup::gc_action_wait_for_set_play(); }, "Set");
}

inline GameControllerAction waitForPenaltyShot() {
    return make([]() { return RoboCup::gc_action_wait_for_penalty_shot(); }, "Set");
}

inline GameControllerAction freeSetPlay() {
    return make([]() { return RoboCup::gc_action_free_set_play(); }, "Playing");
}

inline GameControllerAction freePenaltyShot() {
    return make([]() { return RoboCup::gc_action_free_penalty_shot(); }, "Playing");
}

inline GameControllerAction finishSetPlay() {
    return make([]() { return RoboCup::gc_action_finish_set_play(); }, "Set Play Complete");
}

inline GameControllerAction finishHalf() {
    return make([]() { return RoboCup::gc_action_finish_half(); }, "Finish");
}

inline GameControllerAction finishPenaltyShot() {
    return make([]() { return RoboCup::gc_action_finish_penalty_shot(); }, "Finish");
}

inline GameControllerAction globalGameStuck() {
    return make([]() { return RoboCup::gc_action_global_game_stuck(); }, "Global Game Stuck");
}

inline GameControllerAction addAdditionalTime() {
    return make([]() { return RoboCup::gc_action_add_additional_time(); }, "Add Additional Time");
}

}  // namespace gcaction

}  // namespace spqr
