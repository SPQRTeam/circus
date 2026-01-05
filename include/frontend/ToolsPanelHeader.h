#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

namespace spqr {

class ToolsPanelHeader : public QWidget {
        Q_OBJECT

    public:
        ToolsPanelHeader(QWidget* parent) : QWidget(parent) {
            QHBoxLayout* layout = new QHBoxLayout(this);
            layout->setContentsMargins(5, 0, 5, 5);
            layout->setSpacing(5);

            // Open button
            openButton_ = new QPushButton("Open", this);
            openButton_->setStyleSheet("QPushButton { "
                                       "  background-color: #444444; "
                                       "  color: white; "
                                       "  border: 1px solid #666666; "
                                       "  border-radius: 3px; "
                                       "  padding: 2px 2px 2px 2px; "
                                       "  width: 100px; "
                                       "  height: 25px; "
                                       "  font-size: 14px; "
                                       "  font-weight: bold; "
                                       "} "
                                       "QPushButton:hover { "
                                       "  background-color: #595959; "
                                       "  border: 1px solid #1e667e; "
                                       "} ");
            connect(openButton_, &QPushButton::clicked, this, &ToolsPanelHeader::openClicked);
            layout->addWidget(openButton_);

            // Play button
            playButton_ = new QPushButton("Play", this);
            playButton_->setStyleSheet("QPushButton { "
                                       "  background-color: #444444; "
                                       "  color: white; "
                                       "  border: 1px solid #666666; "
                                       "  border-radius: 3px; "
                                       "  padding: 2px 2px 2px 2px; "
                                       "  width: 100px; "
                                       "  height: 25px; "
                                       "  font-size: 14px; "
                                       "  font-weight: bold; "
                                       "} "
                                       "QPushButton:hover { "
                                       "  background-color: #595959; "
                                       "  border: 1px solid #1e667e; "
                                       "} ");
            connect(playButton_, &QPushButton::clicked, this, &ToolsPanelHeader::playClicked);
            layout->addWidget(playButton_);

            // Pause button
            pauseButton_ = new QPushButton("Pause", this);
            pauseButton_->setStyleSheet("QPushButton { "
                                        "  background-color: #444444; "
                                        "  color: white; "
                                        "  border: 1px solid #666666; "
                                        "  border-radius: 3px; "
                                        "  padding: 2px 2px 2px 2px; "
                                        "  width: 100px; "
                                        "  height: 25px; "
                                        "  font-size: 14px; "
                                        "  font-weight: bold; "
                                        "} "
                                        "QPushButton:hover { "
                                        "  background-color: #595959; "
                                        "  border: 1px solid #1e667e; "
                                        "} ");
            connect(pauseButton_, &QPushButton::clicked, this, &ToolsPanelHeader::pauseClicked);
            layout->addWidget(pauseButton_);

            // Spacer to push collapse button to the right
            layout->addStretch();

            // Collapse/Expand button on the right
            collapseButton_ = new QPushButton("▲", this);
            collapseButton_->setStyleSheet("QPushButton { "
                                           "  background-color: #444444; "
                                           "  color: white; "
                                           "  border: 1px solid #666666; "
                                           "  border-radius: 3px; "
                                           "  padding: 2px 2px 2px 2px; "
                                           "  width: 40px; "
                                           "  height: 25px; "
                                           "  font-size: 14px; "
                                           "  font-weight: bold; "
                                           "} "
                                           "QPushButton:hover { "
                                           "  background-color: #595959; "
                                           "  border: 1px solid #1e667e; "
                                           "} ");
            connect(collapseButton_, &QPushButton::clicked, this, &ToolsPanelHeader::toggleCollapse);
            layout->addWidget(collapseButton_);

            setLayout(layout);
            setFixedHeight(40);

            isCollapsed_ = true;
        }

        void setCollapsed(bool collapsed) {
            isCollapsed_ = collapsed;
            collapseButton_->setText(collapsed ? "▲" : "▼");
        }

        void setSimulationPlaying(bool playing) {
            playButton_->setEnabled(!playing);
            pauseButton_->setEnabled(playing);

            pauseButton_->setStyleSheet(playing ? "QPushButton { "
                                                  "  background-color: #444444; "
                                                  "  color: white; "
                                                  "  border: 1px solid #7e1e1e; "
                                                  "  border-radius: 3px; "
                                                  "  padding: 2px 2px 2px 2px; "
                                                  "  width: 100px; "
                                                  "  height: 25px; "
                                                  "  font-size: 14px; "
                                                  "  font-weight: bold; "
                                                  "} "
                                                  "QPushButton:hover { "
                                                  "  background-color: #595959; "
                                                  "  border: 1px solid #7e1e1e; "
                                                  "} " :
                                                  "QPushButton { "
                                                  "  background-color: #666666; "
                                                  "  color: #909090; "
                                                  "  border: 1px solid #666666; "
                                                  "  border-radius: 3px; "
                                                  "  padding: 2px 2px 2px 2px; "
                                                  "  width: 100px; "
                                                  "  height: 25px; "
                                                  "  font-size: 14px; "
                                                  "  font-weight: bold; "
                                                  "} "
                                                  "QPushButton:hover { "
                                                  "  background-color: #595959; "
                                                  "  border: 1px solid #1e667e; "
                                                  "} ");

            playButton_->setStyleSheet(!playing ? "QPushButton { "
                                                  "  background-color: #444444; "
                                                  "  color: white; "
                                                  "  border: 1px solid #1e7e34; "
                                                  "  border-radius: 3px; "
                                                  "  padding: 2px 2px 2px 2px; "
                                                  "  width: 100px; "
                                                  "  height: 25px; "
                                                  "  font-size: 14px; "
                                                  "  font-weight: bold; "
                                                  "} "
                                                  "QPushButton:hover { "
                                                  "  background-color: #595959; "
                                                  "  border: 1px solid #1e7e34; "
                                                  "} " :
                                                  "QPushButton { "
                                                  "  background-color: #666666; "
                                                  "  color: #909090; "
                                                  "  border: 1px solid #666666; "
                                                  "  border-radius: 3px; "
                                                  "  padding: 2px 2px 2px 2px; "
                                                  "  width: 100px; "
                                                  "  height: 25px; "
                                                  "  font-size: 14px; "
                                                  "  font-weight: bold; "
                                                  "} "
                                                  "QPushButton:hover { "
                                                  "  background-color: #595959; "
                                                  "  border: 1px solid #1e667e; "
                                                  "} ");
        }

    signals:
        void openClicked();
        void playClicked();
        void pauseClicked();
        void collapseToggled(bool collapsed);

    private slots:
        void toggleCollapse() {
            isCollapsed_ = !isCollapsed_;
            collapseButton_->setText(isCollapsed_ ? "▲" : "▼");
            emit collapseToggled(isCollapsed_);
        }

    private:
        QPushButton* openButton_;
        QPushButton* playButton_;
        QPushButton* pauseButton_;
        QPushButton* collapseButton_;
        bool isCollapsed_;
};

}  // namespace spqr
