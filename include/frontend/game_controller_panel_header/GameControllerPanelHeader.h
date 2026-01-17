#pragma once

#include <qlabel.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QWidget>
#include <QPushButton>

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

            // Add spacing
            layout->addSpacing(10);

            QLabel* phaseTimeTitle = new QLabel("Phase Time:", this);
            phaseTimeTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(phaseTimeTitle);

            gamePhaseTimeLabel_ = new QLabel("00:00", this);
            gamePhaseTimeLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gamePhaseTimeLabel_);
            
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
            simTimeLabel_->setText(QString("%1:%2")
                                       .arg(simTimeMinutes, 2, 10, QChar('0'))
                                       .arg(simTimeSeconds, 2, 10, QChar('0')));

            // Update game time
            double gameTime = gc.getGameTime();
            int gameTimeMinutes = static_cast<int>(gameTime) / 60;
            int gameTimeSeconds = static_cast<int>(gameTime) % 60;
            gameTimeLabel_->setText(QString("%1:%2")
                                        .arg(gameTimeMinutes, 2, 10, QChar('0'))
                                        .arg(gameTimeSeconds, 2, 10, QChar('0')));

            // Update score
            auto [redScore, blueScore] = gc.getScore();
            redScoreLabel_->setText(QString::number(redScore));
            blueScoreLabel_->setText(QString::number(blueScore));

            // Update game phase and button label (phase can be changed from other modules)
            GamePhase phase = gc.getCurrentPhase();
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
                    // Already at final phase, do nothing
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
                    phaseButton_->setText("Playing");
                    phaseButton_->setEnabled(false);
                    phaseButton_->setStyleSheet(getButtonStyle(false));
                    break;
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

        QString getLabelValueStyle() {
            return "QLabel { "
                   "  color: #ffffffff; "
                   "  font-size: 14px; "
                   "  font-weight: bold; "
                   "  background-color: transparent; "
                   "  border: none; "
                   "}";
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
        
        QString getButtonStyle(bool enabled=true) {
            if(enabled) 
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
        QLabel* gamePhaseTimeLabel_;
        QPushButton* phaseButton_;
        QLabel* redScoreLabel_;
        QLabel* blueScoreLabel_;
        QTimer* updateTimer_;
};

}  // namespace spqr
