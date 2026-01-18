#pragma once

#include <qlabel.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <iostream>

#include "GameController.h"

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
            GameController& gc = GameController::instance();

            // Update sim time
            double simTime = gc.getSimTime();
            int simTimeMinutes = static_cast<int>(simTime) / 60;
            int simTimeSeconds = static_cast<int>(simTime) % 60;
            simTimeLabel_->setText(QString("%1:%2").arg(simTimeMinutes, 2, 10, QChar('0')).arg(simTimeSeconds, 2, 10, QChar('0')));

            // Update game time (countdown from playing duration to 0)
            double gameTime = gc.getGameElapsedTime();
            double gameRemainingTime = gc.getGameDuration() - gameTime;
            if (gameRemainingTime < 0)
                gameRemainingTime = 0.0;
            int gameTimeMinutes = static_cast<int>(gameRemainingTime) / 60;
            int gameTimeSeconds = static_cast<int>(gameRemainingTime) % 60;
            gameTimeLabel_->setText(QString("%1:%2").arg(gameTimeMinutes, 2, 10, QChar('0')).arg(gameTimeSeconds, 2, 10, QChar('0')));

            // Update current phase time (remaining time counting down)
            GamePhase phase = gc.getCurrentPhase();
            double phaseElapsedTime = gc.getCurrentPhaseElapsedTime();
            double phaseRemainingTime = 0.0;

            // Hide phase time during PLAYING phase
            bool showPhaseTime = (phase != PLAYING);
            phaseTimeSpacer_->setVisible(showPhaseTime);
            phaseTimeTitleLabel_->setVisible(showPhaseTime);
            gamePhaseTimeLabel_->setVisible(showPhaseTime);

            if (showPhaseTime) {
                switch (phase) {
                    case INITIAL:
                        phaseRemainingTime = gc.getInitialPhaseDuration() - phaseElapsedTime;
                        break;
                    case READY:
                        phaseRemainingTime = gc.getReadyPhaseDuration() - phaseElapsedTime;
                        break;
                    case SET:
                        phaseRemainingTime = gc.getSetPhaseDuration() - phaseElapsedTime;
                        break;
                    case PLAYING:
                    case FINISH:
                        phaseRemainingTime = 0.0;
                        break;
                }

                // Ensure remaining time is not negative
                if (phaseRemainingTime < 0)
                    phaseRemainingTime = 0.0;

                int phaseMinutes = static_cast<int>(phaseRemainingTime) / 60;
                int phaseSeconds = static_cast<int>(phaseRemainingTime) % 60;
                gamePhaseTimeLabel_->setText(QString("%1:%2").arg(phaseMinutes, 2, 10, QChar('0')).arg(phaseSeconds, 2, 10, QChar('0')));
            }

            // Update sub-phase display (only visible when not BALLFREE)
            GameSubPhase subPhase = gc.getCurrentSubPhase();
            bool showSubPhase = (subPhase != BALLFREE);

            subPhaseTitleLabel_->setVisible(showSubPhase);
            subPhaseLabel_->setVisible(showSubPhase);
            subPhaseTimeTitleLabel_->setVisible(showSubPhase);
            subPhaseTimeLabel_->setVisible(showSubPhase);

            if (showSubPhase) {
                subPhaseLabel_->setText(QString::fromStdString(gameSubPhaseToString(subPhase)));

                // Set color based on team
                std::string subPhaseTeam = gc.getCurrentSubPhaseTeam();
                QString color = "#ffffff";  // White by default
                if (subPhaseTeam == "red") {
                    color = "#993546";
                } else if (subPhaseTeam == "blue") {
                    color = "#108296";
                }
                subPhaseLabel_->setStyleSheet(getLabelValueStyle(color));

                // Calculate sub-phase remaining time (KICKOFF has different duration)
                double subPhaseElapsedTime = gc.getCurrentSubPhaseElapsedTime();
                double subPhaseDuration = (subPhase == KICKOFF) ? gc.getKickOffSubPhaseDuration() : gc.getSubPhaseDuration();
                double subPhaseRemainingTime = subPhaseDuration - subPhaseElapsedTime;
                if (subPhaseRemainingTime < 0)
                    subPhaseRemainingTime = 0.0;

                int subPhaseMinutes = static_cast<int>(subPhaseRemainingTime) / 60;
                int subPhaseSeconds = static_cast<int>(subPhaseRemainingTime) % 60;
                subPhaseTimeLabel_->setText(QString("%1:%2").arg(subPhaseMinutes, 2, 10, QChar('0')).arg(subPhaseSeconds, 2, 10, QChar('0')));
            }

            // Update score
            auto [redScore, blueScore] = gc.getScore();
            redScoreLabel_->setText(QString::number(redScore));
            blueScoreLabel_->setText(QString::number(blueScore));

            // Update game phase label and button (phase already fetched above)
            gamePhaseLabel_->setText(QString::fromStdString(gamePhaseToString(phase)));
            updatePhaseButton(phase);
        }

        void onPhaseButtonClicked() {
            GameController& gc = GameController::instance();
            GamePhase currentPhase = gc.getCurrentPhase();

            // Advance to next phase: Initial -> Ready -> Set -> Playing
            switch (currentPhase) {
                case INITIAL:
                    gc.handleCommand("ready");
                    break;
                case READY:
                    gc.handleCommand("set");
                    break;
                case SET:
                    gc.handleCommand("playing");
                    break;
                case PLAYING:
                    gc.handleCommand("finish");
                    break;
                case FINISH:
                    // Do nothing
                    break;
            }
        }

    private:
        void updatePhaseButton(GamePhase phase) {
            // Update button label based on current phase
            // Button shows the NEXT phase to transition to
            switch (phase) {
                case INITIAL:
                    phaseButton_->setText("Ready");
                    phaseButton_->setEnabled(true);
                    phaseButton_->setStyleSheet(getButtonStyle(true));
                    break;
                case READY:
                    phaseButton_->setText("Set");
                    phaseButton_->setEnabled(true);
                    phaseButton_->setStyleSheet(getButtonStyle(true));
                    break;
                case SET:
                    phaseButton_->setText("Playing");
                    phaseButton_->setEnabled(true);
                    phaseButton_->setStyleSheet(getButtonStyle(true));
                    break;
                case PLAYING:
                    phaseButton_->setText("Finish");
                    phaseButton_->setEnabled(true);
                    phaseButton_->setStyleSheet(getButtonStyle(true));
                    break;
                case FINISH:
                    phaseButton_->setText("Finished");
                    phaseButton_->setEnabled(false);
                    phaseButton_->setStyleSheet(getButtonStyle(false));
            }
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
