#pragma once

#include <GameController.h>
#include <RoboCupGameControlData.h>
#include <qlabel.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <iostream>
#include <map>
#include <optional>

#include "CommandConsole.h"
#include "GameControllerActions.h"

namespace spqr {

class GameControllerPanelHeader : public QWidget {
        Q_OBJECT

    public:
        GameControllerPanelHeader(QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                          "  background-color: #333333; "
                          "  border: 1px solid #555555; "
                          "  border-radius: 3px; "
                          "}");

            // Set fixed height for the header
            setFixedHeight(40);

            // Main horizontal layout
            QHBoxLayout* layout = new QHBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            // Sim Time label
            QLabel* simTimeTitle = new QLabel("Sim Time:", this);
            simTimeTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(simTimeTitle);

            simTimeLabel_ = new QLabel("00:00", this);
            simTimeLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(simTimeLabel_);

            // Add spacing
            layout->addSpacing(10);

            // Game Time label
            QLabel* gameTimeTitle = new QLabel("Game Time:", this);
            gameTimeTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(gameTimeTitle);

            gameTimeLabel_ = new QLabel("00:00", this);
            gameTimeLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gameTimeLabel_);

            // Add spacing
            layout->addSpacing(10);

            // Game Phase label
            QLabel* phaseTitle = new QLabel("Game Phase:", this);
            phaseTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(phaseTitle);

            gamePhaseLabel_ = new QLabel("Initial", this);
            gamePhaseLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gamePhaseLabel_);

            // Spacer before Phase Time (can be hidden)
            phaseTimeSpacer_ = new QWidget(this);
            phaseTimeSpacer_->setFixedWidth(5);
            phaseTimeSpacer_->setStyleSheet("QWidget { background-color: transparent; border: none; }");
            layout->addWidget(phaseTimeSpacer_);

            // Phase Time label
            phaseTimeTitleLabel_ = new QLabel("Phase Time:", this);
            phaseTimeTitleLabel_->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(phaseTimeTitleLabel_);

            gamePhaseTimeLabel_ = new QLabel("00:00", this);
            gamePhaseTimeLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gamePhaseTimeLabel_);

            layout->addSpacing(10);

            // SubPhase label (hidden by default)
            subPhaseTitleLabel_ = new QLabel("Sub Phase:", this);
            subPhaseTitleLabel_->setStyleSheet(getLabelTitleStyle());
            subPhaseTitleLabel_->setVisible(false);
            layout->addWidget(subPhaseTitleLabel_);

            subPhaseLabel_ = new QLabel("None", this);
            subPhaseLabel_->setStyleSheet(getLabelValueStyle());
            subPhaseLabel_->setVisible(false);
            layout->addWidget(subPhaseLabel_);

            layout->addSpacing(10);

            // SubPhase Time label (hidden by default)
            subPhaseTimeTitleLabel_ = new QLabel("Sub Phase Time:", this);
            subPhaseTimeTitleLabel_->setStyleSheet(getLabelTitleStyle());
            subPhaseTimeTitleLabel_->setVisible(false);
            layout->addWidget(subPhaseTimeTitleLabel_);

            subPhaseTimeLabel_ = new QLabel("00:00", this);
            subPhaseTimeLabel_->setStyleSheet(getLabelValueStyle());
            subPhaseTimeLabel_->setVisible(false);
            layout->addWidget(subPhaseTimeLabel_);

            // Add stretch to push score to the right
            layout->addStretch();

            phaseButton_ = new QPushButton("Ready", this);
            phaseButton_->setToolTip("Advance to next game phase");
            phaseButton_->setStyleSheet(getButtonStyle());
            connect(phaseButton_, &QPushButton::clicked, this, &GameControllerPanelHeader::onPhaseButtonClicked);
            layout->addWidget(phaseButton_);

            // Score container with background
            QWidget* scoreContainer = new QWidget(this);
            scoreContainer->setAttribute(Qt::WA_StyledBackground, true);
            scoreContainer->setStyleSheet("QWidget { "
                                          "  background-color: #444444; "
                                          "  border: 1px solid #666666; "
                                          "  border-radius: 3px; "
                                          "}");

            QHBoxLayout* scoreLayout = new QHBoxLayout(scoreContainer);
            scoreLayout->setContentsMargins(8, 2, 8, 2);
            scoreLayout->setSpacing(8);

            // Red team score
            QLabel* redLabel = new QLabel("Red", scoreContainer);
            redLabel->setStyleSheet(getScoreLabelStyle("#993546"));
            scoreLayout->addWidget(redLabel);

            redScoreLabel_ = new QLabel("0", scoreContainer);
            redScoreLabel_->setStyleSheet(getScoreLabelStyle("#ffffff"));
            scoreLayout->addWidget(redScoreLabel_);

            // Separator
            QLabel* separator = new QLabel("-", scoreContainer);
            separator->setStyleSheet(getScoreLabelStyle("#ffffff"));
            scoreLayout->addWidget(separator);

            // Blue team score
            blueScoreLabel_ = new QLabel("0", scoreContainer);
            blueScoreLabel_->setStyleSheet(getScoreLabelStyle("#ffffff"));
            scoreLayout->addWidget(blueScoreLabel_);

            QLabel* blueLabel = new QLabel("Blue", scoreContainer);
            blueLabel->setStyleSheet(getScoreLabelStyle("#108296"));
            scoreLayout->addWidget(blueLabel);

            scoreContainer->setLayout(scoreLayout);
            layout->addWidget(scoreContainer);

            setLayout(layout);

            // Setup timer for periodic updates
            updateTimer_ = new QTimer(this);
            connect(updateTimer_, &QTimer::timeout, this, &GameControllerPanelHeader::updateDisplay);
            updateTimer_->start(100);  // Update every 100ms
        }

        virtual ~GameControllerPanelHeader() = default;

    private slots:
        void updateDisplay() {
            simTimeLabel_->setText(formatTime(static_cast<int>(GameControllerBridge::instance().simTime())));

            RoboCupGameControlData data{};
            if (!GameControllerBridge::instance().readControlData(data, true)) {
                gameTimeLabel_->setText("--:--");
                gamePhaseLabel_->setText("NOT RUNNING");
                updatePhaseButton(nullptr);
                return;
            }

            gameTimeLabel_->setText(formatTime(data.secsRemaining));
            gamePhaseTimeLabel_->setText(formatTime(data.secondaryTime));

            bool shootout = data.gamePhase == GAME_PHASE_PENALTY_SHOOT_OUT;
            gamePhaseLabel_->setText(QString::fromStdString(stateName(data.state)) + (shootout ? " (SHOOTOUT)" : ""));

            subPhaseLabel_->setText(QString::fromStdString(setPlayName(data.setPlay)));
            subPhaseTimeLabel_->setText(kickingTeamName(data));

            redScoreLabel_->setText(QString::number(data.teams[0].score));
            blueScoreLabel_->setText(QString::number(data.teams[1].score));

            updatePhaseButton(&data);
        }

        void onPhaseButtonClicked() {
            RoboCupGameControlData data{};
            if (!GameControllerBridge::instance().readControlData(data, true)) {
                return;
            }
            std::optional<GameControllerAction> action = phaseAction(data);
            if (action) {
                action->apply();
            }
        }

    private:
        static QString formatTime(int seconds) {
            if (seconds < 0) {
                seconds = 0;
            }
            return QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
        }

        static std::string stateName(uint8_t state) {
            switch (state) {
                case STATE_INITIAL:
                    return "INITIAL";
                case STATE_READY:
                    return "READY";
                case STATE_SET:
                    return "SET";
                case STATE_PLAYING:
                    return "PLAYING";
                case STATE_FINISHED:
                    return "FINISHED";
                default:
                    return "UNKNOWN";
            }
        }

        static std::string setPlayName(uint8_t setPlay) {
            switch (setPlay) {
                case SET_PLAY_DIRECT_FREE_KICK:
                    return "Direct free kick";
                case SET_PLAY_INDIRECT_FREE_KICK:
                    return "Indirect free kick";
                case SET_PLAY_PENALTY_KICK:
                    return "Penalty kick";
                case SET_PLAY_THROW_IN:
                    return "Throw-in";
                case SET_PLAY_GOAL_KICK:
                    return "Goal kick";
                case SET_PLAY_CORNER_KICK:
                    return "Corner kick";
                default:
                    return "-";
            }
        }

        static QString kickingTeamName(const RoboCupGameControlData& data) {
            if (data.kickingTeam == KICKING_TEAM_NONE) {
                return "-";
            }
            return data.teams[0].teamNumber == data.kickingTeam ? "SPQR" : "Opponent";
        }

        // The action that moves the game on from the given state, if any.
        static std::optional<GameControllerAction> phaseAction(const RoboCupGameControlData& data) {
            bool shootout = data.gamePhase == GAME_PHASE_PENALTY_SHOOT_OUT;
            switch (data.state) {
                case STATE_INITIAL:
                    return gcaction::kickOff(data);
                case STATE_READY:
                    return shootout ? gcaction::waitForPenaltyShot() : gcaction::waitForSetPlay();
                case STATE_SET:
                    return shootout ? gcaction::freePenaltyShot() : gcaction::freeSetPlay();
                case STATE_PLAYING:
                    return shootout ? gcaction::finishPenaltyShot() : gcaction::finishHalf();
                case STATE_FINISHED:
                    // After the first half (or the first extra half) the second one can start right
                    // away, without waiting for the end of the half-time break.
                    if (!shootout && data.firstHalf) {
                        return gcaction::switchHalf();
                    }
                    return std::nullopt;
                default:
                    return std::nullopt;
            }
        }

        // The button shows the next step of the game, and is enabled only if that action is legal.
        void updatePhaseButton(const RoboCupGameControlData* data) {
            std::optional<GameControllerAction> action = data ? phaseAction(*data) : std::nullopt;
            if (!action) {
                phaseButton_->setText("Finished");
                phaseButton_->setEnabled(false);
                phaseButton_->setStyleSheet(getButtonStyle(false));
                return;
            }

            bool legal = action->isLegal();
            phaseButton_->setText(QString::fromStdString(action->info.name));
            phaseButton_->setEnabled(legal);
            phaseButton_->setStyleSheet(getButtonStyle(legal));
        }

        QString getLabelTitleStyle() {
            return "QLabel { "
                   "  color: #888888; "
                   "  font-size: 14px; "
                   "  background-color: transparent; "
                   "  border: none; "
                   "}";
        }

        QString getLabelValueStyle(const QString& color = QString("#ffffff")) {
            return QString("QLabel { "
                           "  color: %1; "
                           "  font-size: 14px; "
                           "  font-weight: bold; "
                           "  background-color: transparent; "
                           "  border: none; "
                           "}")
                .arg(color);
        }

        QString getScoreLabelStyle(const QString& color) {
            return QString("QLabel { "
                           "  color: %1; "
                           "  font-size: 16px; "
                           "  font-weight: bold; "
                           "  background-color: transparent; "
                           "  border: none; "
                           "}")
                .arg(color);
        }

        QString getButtonStyle(bool enabled = true) {
            if (enabled)
                return "QPushButton { "
                       "  background-color: #444444; "
                       "  color: white; "
                       "  border: 1px solid #666666; "
                       "  border-radius: 3px; "
                       "  padding: 2px 2px 2px 2px; "
                       "  width: 110px; "
                       "  height: 25px; "
                       "  font-size: 12px; "
                       "  font-weight: bold; "
                       "} "
                       "QPushButton:hover { "
                       "  background-color: #595959; "
                       "  border: 1px solid #006778; "
                       "} ";
            else
                return "QPushButton { "
                       "  background-color: #666666; "
                       "  color: #909090; "
                       "  border: 1px solid #666666; "
                       "  border-radius: 3px; "
                       "  padding: 2px 2px 2px 2px; "
                       "  width: 110px; "
                       "  height: 25px; "
                       "  font-size: 12px; "
                       "  font-weight: bold; "
                       "} "
                       "QPushButton:hover { "
                       "  background-color: #595959; "
                       "  border: 1px solid #006778; "
                       "} ";
        }

        QLabel* simTimeLabel_;
        QLabel* gameTimeLabel_;
        QLabel* gamePhaseLabel_;
        QWidget* phaseTimeSpacer_;
        QLabel* phaseTimeTitleLabel_;
        QLabel* gamePhaseTimeLabel_;
        QLabel* subPhaseTitleLabel_;
        QLabel* subPhaseLabel_;
        QLabel* subPhaseTimeTitleLabel_;
        QLabel* subPhaseTimeLabel_;
        QPushButton* phaseButton_;
        QLabel* redScoreLabel_;
        QLabel* blueScoreLabel_;
        QTimer* updateTimer_;
};

}  // namespace spqr
