#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QWidget>

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
            layout->setContentsMargins(10, 5, 10, 5);
            layout->setSpacing(20);

            // Game Time label
            QLabel* gameTimeTitle = new QLabel("Game Time:", this);
            gameTimeTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(gameTimeTitle);

            gameTimeLabel_ = new QLabel("00:00", this);
            gameTimeLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gameTimeLabel_);

            // Add spacing
            layout->addSpacing(30);

            // Score label
            QLabel* scoreTitle = new QLabel("Score:", this);
            scoreTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(scoreTitle);

            // Red team score
            QLabel* redLabel = new QLabel("Red", this);
            redLabel->setStyleSheet(getTeamLabelStyle("#cc4444"));
            layout->addWidget(redLabel);

            redScoreLabel_ = new QLabel("0", this);
            redScoreLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(redScoreLabel_);

            // Separator
            QLabel* separator = new QLabel("-", this);
            separator->setStyleSheet(getLabelValueStyle());
            layout->addWidget(separator);

            // Blue team score
            blueScoreLabel_ = new QLabel("0", this);
            blueScoreLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(blueScoreLabel_);

            QLabel* blueLabel = new QLabel("Blue", this);
            blueLabel->setStyleSheet(getTeamLabelStyle("#4444cc"));
            layout->addWidget(blueLabel);

            // Add spacing
            layout->addSpacing(30);

            // Game Phase label
            QLabel* phaseTitle = new QLabel("Phase:", this);
            phaseTitle->setStyleSheet(getLabelTitleStyle());
            layout->addWidget(phaseTitle);

            gamePhaseLabel_ = new QLabel("INITIAL", this);
            gamePhaseLabel_->setStyleSheet(getLabelValueStyle());
            layout->addWidget(gamePhaseLabel_);

            // Add stretch to push everything to the left
            layout->addStretch();

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

            // Update game time
            double gameTime = gc.getGameTime();
            int minutes = static_cast<int>(gameTime) / 60;
            int seconds = static_cast<int>(gameTime) % 60;
            gameTimeLabel_->setText(QString("%1:%2")
                                        .arg(minutes, 2, 10, QChar('0'))
                                        .arg(seconds, 2, 10, QChar('0')));

            // Update score
            auto [redScore, blueScore] = gc.getScore();
            redScoreLabel_->setText(QString::number(redScore));
            blueScoreLabel_->setText(QString::number(blueScore));

            // Update game phase
            GamePhase phase = gc.getCurrentPhase();
            gamePhaseLabel_->setText(QString::fromStdString(gamePhaseToString(phase)));
        }

    private:
        QString getLabelTitleStyle() {
            return "QLabel { "
                   "  color: #888888; "
                   "  font-size: 12px; "
                   "  background-color: transparent; "
                   "  border: none; "
                   "}";
        }

        QString getLabelValueStyle() {
            return "QLabel { "
                   "  color: #00a0b0; "
                   "  font-size: 14px; "
                   "  font-weight: bold; "
                   "  background-color: transparent; "
                   "  border: none; "
                   "}";
        }

        QString getTeamLabelStyle(const QString& color) {
            return QString("QLabel { "
                           "  color: %1; "
                           "  font-size: 12px; "
                           "  font-weight: bold; "
                           "  background-color: transparent; "
                           "  border: none; "
                           "}")
                .arg(color);
        }

        QLabel* gameTimeLabel_;
        QLabel* redScoreLabel_;
        QLabel* blueScoreLabel_;
        QLabel* gamePhaseLabel_;
        QTimer* updateTimer_;
};

}  // namespace spqr
