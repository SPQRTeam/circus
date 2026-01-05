#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "frontend/ToolsPanelHeader.h"
#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

namespace spqr {

class ToolsPanel : public QWidget {
        Q_OBJECT

    public:
        ToolsPanel(QWidget* parent) : QWidget(parent) {
            // setStyleSheet("QWidget { background-color: rgba(30, 30, 30, 230); }");

            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            header_ = new ToolsPanelHeader(this);
            mainLayout->addWidget(header_);

            container_ = new QWidget(this);
            container_->setStyleSheet("QWidget { background-color: #232323; }");
            QHBoxLayout* containerLayout = new QHBoxLayout(container_);
            containerLayout->setSpacing(10);
            containerLayout->setContentsMargins(10, 10, 10, 10);
            container_->setLayout(containerLayout);
            mainLayout->addWidget(container_);

            setLayout(mainLayout);

            // Connect header signals
            connect(header_, &ToolsPanelHeader::collapseToggled, this, &ToolsPanel::onCollapseToggled);
            connect(header_, &ToolsPanelHeader::openClicked, this, &ToolsPanel::onOpenClicked);
            connect(header_, &ToolsPanelHeader::playClicked, this, &ToolsPanel::onPlayClicked);
            connect(header_, &ToolsPanelHeader::pauseClicked, this, &ToolsPanel::onPauseClicked);

            // Start collapsed
            setFixedHeight(header_->height());
            container_->hide();

            // Initialize button states (no simulation loaded yet)
            header_->setSimulationPlaying(false);
        }

        void setSimulationPlaying(bool playing) {
            header_->setSimulationPlaying(playing);
        }

    signals:
        void openRequested();
        void playRequested();
        void pauseRequested();

    private slots:
        void onCollapseToggled(bool collapsed) {
            if (collapsed) {
                container_->hide();
                setFixedHeight(header_->height());
            } else {
                container_->show();
                setFixedHeight(500);  // Expanded height
            }
        }

        void onOpenClicked() {
            emit openRequested();
        }

        void onPlayClicked() {
            emit playRequested();
        }

        void onPauseClicked() {
            emit pauseRequested();
        }

    private:
        ToolsPanelHeader* header_;
        QWidget* container_;
};

}  // namespace spqr
