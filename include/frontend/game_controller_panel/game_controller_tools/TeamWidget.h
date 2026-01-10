#pragma once

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "GameController.h"
#include "Team.h"

namespace spqr {

class TeamWidget : public QWidget {
        Q_OBJECT

    public:
        TeamWidget(std::string teamName, GameController* gameController, QWidget* parent = nullptr)
            : teamName_(teamName), gameController_(gameController), QWidget(parent) {
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                          "  background-color: #0a0a0a; "
                          "  border: 1px solid #444444; "
                          "}");

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            // Header label
            QLabel* header = new QLabel(QString("Team %1 - Robots").arg(teamName), this);
            header->setStyleSheet("QLabel { "
                                  "  color: #00a0b0; "
                                  "  font-size: 14px; "
                                  "  font-weight: bold; "
                                  "  background-color: transparent; "
                                  "  border: none; "
                                  "}");
            layout->addWidget(header);

            // Robot list
            robotList_ = new QListWidget(this);
            robotList_->setStyleSheet("QListWidget { "
                                      "  background-color: #1a1a1a; "
                                      "  color: #cccccc; "
                                      "  border: 1px solid #444444; "
                                      "  font-size: 12px; "
                                      "} "
                                      "QListWidget::item { "
                                      "  padding: 5px; "
                                      "  border-bottom: 1px solid #333333; "
                                      "} "
                                      "QListWidget::item:selected { "
                                      "  background-color: #006778; "
                                      "  color: white; "
                                      "} "
                                      "QListWidget::item:hover { "
                                      "  background-color: #2a2a2a; "
                                      "}");
            layout->addWidget(robotList_);

            // Placeholder info label
            QLabel* infoLabel = new QLabel("Robot list will be populated from TeamManager", this);
            infoLabel->setAlignment(Qt::AlignCenter);
            infoLabel->setStyleSheet("QLabel { "
                                     "  color: #666666; "
                                     "  font-size: 10px; "
                                     "  font-style: italic; "
                                     "  background-color: transparent; "
                                     "  border: none; "
                                     "}");
            layout->addWidget(infoLabel);

            setLayout(layout);

            loadRobots();
        }

        virtual ~TeamWidget() = default;

    private:
        void loadRobots() {
            for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
                if (team->name == teamName_) {
                    for (std::shared_ptr<Robot> robot : team->robots) {
                        robotList_->addItem(QString::fromStdString(robot->name));
                    }
                }
            }
        }

        std::string teamName_;
        GameController* gameController_;
        QListWidget* robotList_;
};

}  // namespace spqr
