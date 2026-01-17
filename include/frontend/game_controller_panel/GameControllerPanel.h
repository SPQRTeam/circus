#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

#include "frontend/game_controller_panel/GameControllerPanelColumn.h"
#include "frontend/game_controller_panel/game_controller_tools/ConsoleWidget.h"
#include "frontend/game_controller_panel/game_controller_tools/TeamWidget.h"

namespace spqr {

class GameControllerPanel : public QWidget {
        Q_OBJECT

    public:
        GameControllerPanel(QWidget* parent = nullptr) : QWidget(parent) {
            // Main horizontal layout
            QHBoxLayout* mainLayout = new QHBoxLayout(this);
            mainLayout->setContentsMargins(5, 0, 5, 0);
            mainLayout->setSpacing(0);

            // Create the header (left column with buttons)
            column_ = new GameControllerPanelColumn(this);
            mainLayout->addWidget(column_, 0, Qt::AlignLeft);

            // Create the content container
            contentContainer_ = new QWidget(this);
            contentContainer_->setStyleSheet("QWidget { background-color: #1a1a1a; }");
            contentContainer_->setMinimumWidth(minExpandedWidth_);
            contentContainer_->setMaximumWidth(maxExpandedWidth_);
            contentContainer_->hide();  // Start collapsed

            QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer_);
            contentLayout->setContentsMargins(0, 0, 0, 0);
            contentLayout->setSpacing(0);

            contentContainer_->setLayout(contentLayout);
            mainLayout->addWidget(contentContainer_);

            setLayout(mainLayout);

            // Set size policy to prevent expanding
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            setFixedWidth(column_->width() + 5);

            // Connect header signals
            connect(column_, &GameControllerPanelColumn::consoleButtonClicked, this, &GameControllerPanel::onConsoleButtonClicked);
            connect(column_, &GameControllerPanelColumn::team1ButtonClicked, this, &GameControllerPanel::onTeam1ButtonClicked);
            connect(column_, &GameControllerPanelColumn::team2ButtonClicked, this, &GameControllerPanel::onTeam2ButtonClicked);

            // Start collapsed
            isExpanded_ = false;
            currentView_ = GameControllerView::NONE;

            for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
                teamNames_.push_back(team->name);
            }
        }

        bool isExpanded() const {
            return isExpanded_;
        }
        int getExpandedWidth() const {
            return isExpanded_ ? (column_->width() + contentContainer_->width()) : column_->width() + 5;
        }

    signals:
        void expansionChanged(bool expanded);
        void resizeDragStarted();
        void resizeDragEnded();

    protected:
        void mousePressEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton && isExpanded_) {
                int mouseX = event->pos().x();
                // Check if mouse is near the right edge (within 15 pixels for easier grabbing)
                if (std::abs(mouseX - width()) <= 30) {
                    isDragging_ = true;
                    dragStartX_ = event->globalPosition().x();
                    initialWidth_ = width();
                    emit resizeDragStarted();
                    event->accept();
                    return;
                }
            }
            QWidget::mousePressEvent(event);
        }

        void mouseMoveEvent(QMouseEvent* event) override {
            if (isExpanded_) {
                int mouseX = event->pos().x();
                // Change cursor when hovering near right edge (15 pixels for easier targeting)
                if (std::abs(mouseX - width()) <= 30 || isDragging_) {
                    setCursor(Qt::SizeHorCursor);
                } else {
                    setCursor(Qt::ArrowCursor);
                }

                if (isDragging_ && (event->buttons() & Qt::LeftButton)) {
                    int deltaX = event->globalPosition().x() - dragStartX_;
                    int newWidth = initialWidth_ + deltaX;

                    // Clamp width between min and max (fixed values, not dynamic)
                    int minWidth = column_->width() + 250 + 5;  // 250 is base minExpandedWidth
                    int maxWidth = column_->width() + 600 + 5;  // 600 is max allowed width
                    newWidth = std::max(minWidth, std::min(maxWidth, newWidth));

                    // Update content container width
                    int contentWidth = newWidth - column_->width() - 5;
                    contentContainer_->setMinimumWidth(contentWidth);
                    contentContainer_->setMaximumWidth(contentWidth);
                    setFixedWidth(newWidth);

                    event->accept();
                    return;
                }
            }
            QWidget::mouseMoveEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton && isDragging_) {
                isDragging_ = false;
                setCursor(Qt::ArrowCursor);
                emit resizeDragEnded();

                // Store the current width as the preferred width for this view
                currentExpandedWidth_ = contentContainer_->width();

                event->accept();
                return;
            }
            QWidget::mouseReleaseEvent(event);
        }

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
            column_->setActiveButton(GameControllerView::NONE);
            setFixedWidth(column_->width() + 5);
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

            // Reload team names in case they were empty at construction time
            if (teamNames_.empty()) {
                for (std::shared_ptr<Team> team : TeamManager::instance().getTeams()) {
                    teamNames_.push_back(team->name);
                }
            }

            // Create and show the requested view
            QWidget* viewWidget = nullptr;
            switch (view) {
                case GameControllerView::CONSOLE:
                    viewWidget = createConsoleWidget();
                    break;
                case GameControllerView::TEAM1:
                    if (teamNames_.size() > 0) {
                        viewWidget = createTeamWidget(teamNames_[0]);
                    }
                    break;
                case GameControllerView::TEAM2:
                    if (teamNames_.size() > 1) {
                        viewWidget = createTeamWidget(teamNames_[1]);
                    }
                    break;
                case GameControllerView::NONE:
                    break;
            }

            if (viewWidget) {
                layout->addWidget(viewWidget);
                contentContainer_->show();
                isExpanded_ = true;
                currentView_ = view;
                column_->setActiveButton(view);
                setFixedWidth(column_->width() + contentContainer_->minimumWidth() + 5);
                emit expansionChanged(true);
            }
        }

        QWidget* createConsoleWidget() {
            return new ConsoleWidget(contentContainer_);
        }

        QWidget* createTeamWidget(std::string teamName) {
            return new TeamWidget(teamName, contentContainer_);
        }

        GameControllerPanelColumn* column_;
        QWidget* contentContainer_;

        bool isExpanded_ = false;
        GameControllerView currentView_ = GameControllerView::NONE;

        // Size constraints
        int minExpandedWidth_ = 250;
        int maxExpandedWidth_ = 400;
        int currentExpandedWidth_ = 250;

        std::vector<std::string> teamNames_;

        // Resize drag state
        bool isDragging_ = false;
        qreal dragStartX_ = 0;
        int initialWidth_ = 0;
};

}  // namespace spqr
