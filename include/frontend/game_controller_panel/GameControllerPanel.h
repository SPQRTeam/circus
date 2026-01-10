#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

#include "GameController.h"
#include "frontend/game_controller_panel/GameControllerPanelHeader.h"
#include "frontend/game_controller_panel/game_controller_tools/ConsoleWidget.h"
#include "frontend/game_controller_panel/game_controller_tools/TeamWidget.h"

namespace spqr {

enum class GameControllerView {
    NONE,
    CONSOLE,
    TEAM1,
    TEAM2
};

class GameControllerPanel : public QWidget {
        Q_OBJECT

    public:
        GameControllerPanel(GameController* gameController, QWidget* parent = nullptr)
            : gameController_(gameController), QWidget(parent) {

            // Main horizontal layout
            QHBoxLayout* mainLayout = new QHBoxLayout(this);
            mainLayout->setContentsMargins(5, 0, 5, 0);
            mainLayout->setSpacing(0);

            // Create the header (left column with buttons)
            header_ = new GameControllerPanelHeader(this);
            mainLayout->addWidget(header_, 0, Qt::AlignLeft);

            // Create the content container
            contentContainer_ = new QWidget(this);
            contentContainer_->setStyleSheet("QWidget { background-color: #1a1a1a; }");
            contentContainer_->setMinimumWidth(minExpandedWidth_);
            contentContainer_->setMaximumWidth(maxExpandedWidth_);
            contentContainer_->hide(); // Start collapsed

            QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer_);
            contentLayout->setContentsMargins(0, 0, 0, 0);
            contentLayout->setSpacing(0);

            contentContainer_->setLayout(contentLayout);
            mainLayout->addWidget(contentContainer_);

            setLayout(mainLayout);

            // Set size policy to prevent expanding
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            setFixedWidth(header_->width() + 5);

            // Connect header signals
            connect(header_, &GameControllerPanelHeader::consoleButtonClicked, this, &GameControllerPanel::onConsoleButtonClicked);
            connect(header_, &GameControllerPanelHeader::team1ButtonClicked, this, &GameControllerPanel::onTeam1ButtonClicked);
            connect(header_, &GameControllerPanelHeader::team2ButtonClicked, this, &GameControllerPanel::onTeam2ButtonClicked);

            // Start collapsed
            isExpanded_ = false;
            currentView_ = GameControllerView::NONE;

            for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
                teamNames_.push_back(team->name);
            }
        }

        bool isExpanded() const { return isExpanded_; }
        int getExpandedWidth() const { return isExpanded_ ? (header_->width() + contentContainer_->width()) : header_->width() + 5; }

    signals:
        void expansionChanged(bool expanded);

    private slots:
        void onConsoleButtonClicked() {
            if (isExpanded_ && currentView_ == GameControllerView::CONSOLE) {
                // Collapse if already showing console
                collapsePanel();
            } else {
                // Expand and show console
                showView(GameControllerView::CONSOLE);
            }
        }

        void onTeam1ButtonClicked() {
            if (isExpanded_ && currentView_ == GameControllerView::TEAM1) {
                // Collapse if already showing team1
                collapsePanel();
            } else {
                // Expand and show team1
                showView(GameControllerView::TEAM1);
            }
        }

        void onTeam2ButtonClicked() {
            if (isExpanded_ && currentView_ == GameControllerView::TEAM2) {
                // Collapse if already showing team2
                collapsePanel();
            } else {
                // Expand and show team2
                showView(GameControllerView::TEAM2);
            }
        }

    private:
        void collapsePanel() {
            contentContainer_->hide();
            isExpanded_ = false;
            currentView_ = GameControllerView::NONE;
            header_->setActiveButton(GameControllerView::NONE);
            setFixedWidth(header_->width());
            emit expansionChanged(false);
        }

        void showView(GameControllerView view) {
            // Clear current content
            QLayout* layout = contentContainer_->layout();
            if (layout) {
                QLayoutItem* item;
                while ((item = layout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        item->widget()->hide();
                        item->widget()->setParent(nullptr);
                    }
                    delete item;
                }
            }

            // Create and show the requested view
            QWidget* viewWidget = nullptr;
            switch (view) {
                case GameControllerView::CONSOLE:
                    viewWidget = createConsoleWidget();
                    break;
                case GameControllerView::TEAM1:
                    viewWidget = createTeamWidget(teamNames_[0]);
                    break;
                case GameControllerView::TEAM2:
                    viewWidget = createTeamWidget(teamNames_[1]);
                    break;
                case GameControllerView::NONE:
                    break;
            }

            if (viewWidget) {
                layout->addWidget(viewWidget);
                contentContainer_->show();
                isExpanded_ = true;
                currentView_ = view;
                header_->setActiveButton(view);
                setFixedWidth(header_->width() + contentContainer_->minimumWidth());
                emit expansionChanged(true);
            }
        }

        QWidget* createConsoleWidget() {
            return new ConsoleWidget(gameController_, contentContainer_);
        }

        QWidget* createTeamWidget(std::string teamName) {
            return new TeamWidget(teamName, gameController_, contentContainer_);
        }

        GameController* gameController_;
        GameControllerPanelHeader* header_;
        QWidget* contentContainer_;

        bool isExpanded_ = false;
        GameControllerView currentView_ = GameControllerView::NONE;

        // Size constraints
        int minExpandedWidth_ = 250;
        int maxExpandedWidth_ = 400;

        std::vector<std::string> teamNames_;
};

}  // namespace spqr
