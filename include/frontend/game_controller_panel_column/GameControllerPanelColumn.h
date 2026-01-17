#pragma once

#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

namespace spqr {

enum class GameControllerView { NONE, CONSOLE, TEAM1, TEAM2 };

enum class GameControllerView;

class GameControllerPanelColumn : public QWidget {
        Q_OBJECT

    public:
        GameControllerPanelColumn(QWidget* parent = nullptr) : QWidget(parent) {
            // Create background widget
            background_ = new QWidget(this);
            background_->setStyleSheet("QWidget { "
                                       "  background-color: #333333; "
                                       "  border: 1px solid #555555; "
                                       "  border-radius: 3px; "
                                       "}");
            background_->lower();  // Send to back

            // Set fixed width for the button column
            setFixedWidth(50);

            // Vertical layout for the buttons
            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            // Console button (at the top)
            consoleButton_ = new QPushButton("C", this);
            consoleButton_->setToolTip("Console");
            consoleButton_->setStyleSheet(getButtonStyle());
            consoleButton_->setFixedSize(40, 40);
            connect(consoleButton_, &QPushButton::clicked, this, &GameControllerPanelColumn::consoleButtonClicked);
            layout->addWidget(consoleButton_);

            // Team 1 button
            team1Button_ = new QPushButton("T1", this);
            team1Button_->setToolTip("Team 1");
            team1Button_->setStyleSheet(getButtonStyle());
            team1Button_->setFixedSize(40, 40);
            connect(team1Button_, &QPushButton::clicked, this, &GameControllerPanelColumn::team1ButtonClicked);
            layout->addWidget(team1Button_);

            // Team 2 button
            team2Button_ = new QPushButton("T2", this);
            team2Button_->setToolTip("Team 2");
            team2Button_->setStyleSheet(getButtonStyle());
            team2Button_->setFixedSize(40, 40);
            connect(team2Button_, &QPushButton::clicked, this, &GameControllerPanelColumn::team2ButtonClicked);
            layout->addWidget(team2Button_);

            // Add stretch to push buttons to the top
            layout->addStretch();

            setLayout(layout);
        }

        // void setActiveButton(GameControllerView view);
        void setActiveButton(GameControllerView view) {
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

    signals:
        void consoleButtonClicked();
        void team1ButtonClicked();
        void team2ButtonClicked();

    protected:
        void resizeEvent(QResizeEvent* event) override {
            QWidget::resizeEvent(event);
            if (background_) {
                background_->setGeometry(0, 0, width(), height());
            }
        }

    private:
        QString getButtonStyle() {
            return "QPushButton { "
                   "  background-color: #444444; "
                   "  color: white; "
                   "  border: 1px solid #666666; "
                   "  border-radius: 3px; "
                   "  font-size: 11px; "
                   "  font-weight: bold; "
                   "} "
                   "QPushButton:hover { "
                   "  background-color: #595959; "
                   "  border: 1px solid #006778; "
                   "} ";
        }

        QString getActiveButtonStyle() {
            return "QPushButton { "
                   "  background-color: #006778; "
                   "  color: white; "
                   "  border: 1px solid #00a0b0; "
                   "  border-radius: 3px; "
                   "  font-size: 11px; "
                   "  font-weight: bold; "
                   "} "
                   "QPushButton:hover { "
                   "  background-color: #007888; "
                   "  border: 1px solid #00c0d0; "
                   "} ";
        }

        QWidget* background_;
        QPushButton* consoleButton_;
        QPushButton* team1Button_;
        QPushButton* team2Button_;
};

}  // namespace spqr
