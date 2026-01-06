#pragma once

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMoveEvent>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include <vector>
#include <algorithm>

#include "Constants.h"
#include "frontend/ToolsPanelHeader.h"
#include "frontend/ToolsPanelGrid.h"
#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

namespace spqr {

class ToolsPanel : public QWidget {
        Q_OBJECT

    public:
        ToolsPanel(QWidget* parent) : QWidget(parent) {
            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(5, 0, 5, 0);
            mainLayout->setSpacing(0);

            header_ = new ToolsPanelHeader(this);
            mainLayout->addWidget(header_);

            container_ = new QWidget(this);
            container_->setStyleSheet("QWidget { background-color: #232323; }");
            QVBoxLayout* containerLayout = new QVBoxLayout(container_);
            containerLayout->setSpacing(0);
            containerLayout->setContentsMargins(10, 10, 10, 10);

            // Add the grid to the container
            grid_ = new ToolsPanelGrid(container_);
            containerLayout->addWidget(grid_);

            container_->setLayout(containerLayout);
            mainLayout->addWidget(container_);

            setLayout(mainLayout);

            // Initialize size constraints
            minExpandedHeight_ = 500;
            updateMaxHeight();
            currentExpandedHeight_ = minExpandedHeight_;

            // Connect header signals
            connect(header_, &ToolsPanelHeader::openClicked, this, &ToolsPanel::onOpenClicked);
            connect(header_, &ToolsPanelHeader::playClicked, this, &ToolsPanel::onPlayClicked);
            connect(header_, &ToolsPanelHeader::pauseClicked, this, &ToolsPanel::onPauseClicked);
            connect(header_, &ToolsPanelHeader::collapseToggled, this, &ToolsPanel::onCollapseToggled);
            connect(header_, &ToolsPanelHeader::resizeDragStarted, this, &ToolsPanel::onResizeDragStarted);
            connect(header_, &ToolsPanelHeader::resizeRequested, this, &ToolsPanel::onResizeRequested);
            connect(header_, &ToolsPanelHeader::resizeDragEnded, this, &ToolsPanel::onResizeDragEnded);

            // Connect grid control buttons from header to grid slots
            connect(header_, &ToolsPanelHeader::addRowClicked, grid_, &ToolsPanelGrid::addRow);
            connect(header_, &ToolsPanelHeader::removeRowClicked, grid_, &ToolsPanelGrid::removeRow);
            connect(header_, &ToolsPanelHeader::addColumnClicked, grid_, &ToolsPanelGrid::addColumn);
            connect(header_, &ToolsPanelHeader::removeColumnClicked, grid_, &ToolsPanelGrid::removeColumn);

            // Connect grid size changes to update header button states
            connect(grid_, &ToolsPanelGrid::gridSizeChanged, header_, &ToolsPanelHeader::updateGridButtonStates);

            // Start collapsed
            isCollapsed_ = true;
            container_->hide();
            setFixedHeight(header_->height());

            // Initialize button states (no simulation loaded yet)
            header_->setSimulationPlaying(false);
        }

        void setSimulationPlaying(bool playing) {
            header_->setSimulationPlaying(playing);
        }

    protected:
        void moveEvent(QMoveEvent* event) override {
            QWidget::moveEvent(event);
            updateMaxHeight();
        }

        void changeEvent(QEvent* event) override {
            QWidget::changeEvent(event);
            if (event->type() == QEvent::ParentChange) {
                updateMaxHeight();
            } else if (event->type() == QEvent::WindowStateChange) {
                QWidget* topLevel = window();
                if (topLevel) {
                    // Check if window was un-maximized
                    if (!(topLevel->windowState() & Qt::WindowMaximized) && wasMaximized_) {
                        // Resize to initial window size
                        topLevel->resize(initialWindowWidth, initialWindowHeight);
                    }
                    wasMaximized_ = (topLevel->windowState() & Qt::WindowMaximized);
                }
                updateMaxHeight();
            }
        }

    signals:
        void openRequested();
        void playRequested();
        void pauseRequested();
        void resizeDragStarted();
        void resizeDragEnded();

    private slots:
        void onCollapseToggled() {
            isCollapsed_ = !isCollapsed_;
            if (isCollapsed_) {
                container_->hide();
                setFixedHeight(header_->height());
            } else {
                container_->show();
                setFixedHeight(currentExpandedHeight_);
            }
        }

        void onResizeDragStarted() {
            // Disable window resizing when drag starts
            QWidget* topLevel = window();
            if (topLevel) {
                topLevel->setFixedSize(topLevel->size());
            }
            emit resizeDragStarted();
        }

        void onResizeRequested(int deltaY) {
            if (isCollapsed_) {
                return;
            }

            int newHeight = height() + deltaY;
            newHeight = std::max(minExpandedHeight_, std::min(maxExpandedHeight_, newHeight));

            currentExpandedHeight_ = newHeight;
            setFixedHeight(newHeight);
        }

        void onResizeDragEnded() {
            // Re-enable window resizing when drag ends
            QWidget* topLevel = window();
            if (topLevel) {
                topLevel->setMinimumSize(0, 0);
                topLevel->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            }
            emit resizeDragEnded();
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
        void updateMaxHeight() {
            QWidget* topLevel = window();

            int currentWindowHeight = topLevel ? topLevel->height() : 0;

            // Calculate minimum simulation height (at least 1/6 of screen)
            int minSimulationHeight = currentWindowHeight / 6;

            // Calculate max height for tools panel
            // Reserve space for: min simulation height + header + margins
            maxExpandedHeight_ = currentWindowHeight - minSimulationHeight - header_->height() - 50;

            // Set fixed minimum window size
            if (topLevel) {
                topLevel->setMinimumSize(initialWindowWidth, initialWindowHeight);
            }

            // Clamp current height if it exceeds new max
            if (currentExpandedHeight_ > maxExpandedHeight_) {
                currentExpandedHeight_ = maxExpandedHeight_;
                if (container_->isVisible()) {
                    setFixedHeight(currentExpandedHeight_);
                }
            }
        }

        ToolsPanelHeader* header_;
        QWidget* container_;
        ToolsPanelGrid* grid_;

        int minExpandedHeight_;
        int maxExpandedHeight_;
        int currentExpandedHeight_;
        bool wasMaximized_ = false;
        bool isCollapsed_ = true;
};

}  // namespace spqr
