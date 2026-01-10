#include "frontend/game_controller_panel/GameControllerPanelHeader.h"
#include "frontend/game_controller_panel/GameControllerPanel.h"

namespace spqr {

void GameControllerPanelHeader::setActiveButton(GameControllerView view) {
    // Reset all buttons to normal style
    consoleButton_->setStyleSheet(getButtonStyle());
    team1Button_->setStyleSheet(getButtonStyle());
    team2Button_->setStyleSheet(getButtonStyle());

    // Set active button style
    switch (view) {
        case GameControllerView::CONSOLE:
            consoleButton_->setStyleSheet(getActiveButtonStyle());
            break;
        case GameControllerView::TEAM1:
            team1Button_->setStyleSheet(getActiveButtonStyle());
            break;
        case GameControllerView::TEAM2:
            team2Button_->setStyleSheet(getActiveButtonStyle());
            break;
        case GameControllerView::NONE:
            // All buttons are already reset
            break;
    }
}

}  // namespace spqr
