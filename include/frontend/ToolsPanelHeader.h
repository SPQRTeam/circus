#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <vector>
#include <cmath>

#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

namespace spqr {

class ToolsPanelHeader : public QWidget {
        Q_OBJECT

    public:
        ToolsPanelHeader(QWidget* parent) : QWidget(parent) {
            // Create background widget
            background_ = new QWidget(this);
            background_->setStyleSheet("QWidget { "
                                      "  background-color: #333333; "
                                      "  border: 1px solid #555555; "
                                      "  border-radius: 3px; "
                                      "}");
            background_->lower(); // Send to back

            QHBoxLayout* layout = new QHBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            // Open button
            openButton_ = new QPushButton("Open", this);
            openButton_->setStyleSheet("QPushButton { "
                                       "  background-color: #444444; "
                                       "  color: white; "
                                       "  border: 1px solid #666666; "
                                       "  border-radius: 3px; "
                                       "  padding: 2px 2px 2px 2px; "
                                       "  width: 80px; "
                                       "  height: 25px; "
                                       "  font-size: 12px; "
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
                                       "  width: 80px; "
                                       "  height: 25px; "
                                       "  font-size: 12px; "
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
                                        "  width: 80px; "
                                        "  height: 25px; "
                                        "  font-size: 12px; "
                                        "  font-weight: bold; "
                                        "} "
                                        "QPushButton:hover { "
                                        "  background-color: #595959; "
                                        "  border: 1px solid #1e667e; "
                                        "} ");
            connect(pauseButton_, &QPushButton::clicked, this, &ToolsPanelHeader::pauseClicked);
            layout->addWidget(pauseButton_);
            
            layout->addStretch();
            setLayout(layout);
            setFixedHeight(35);
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
                                                  "  width: 80px; "
                                                  "  height: 25px; "
                                                  "  font-size: 12px; "
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
                                                  "  width: 80px; "
                                                  "  height: 25px; "
                                                  "  font-size: 12px; "
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
                                                  "  width: 80px; "
                                                  "  height: 25px; "
                                                  "  font-size: 12px; "
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
                                                  "  width: 80px; "
                                                  "  height: 25px; "
                                                  "  font-size: 12px; "
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
        void collapseToggled();
        void resizeDragStarted();
        void resizeDragEnded();
        void resizeRequested(int deltaY);

    protected:
        void mousePressEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton) {
                isDragging_ = false;
                dragStartY_ = event->globalPosition().y();
                event->accept();
            }
        }

        void mouseMoveEvent(QMouseEvent* event) override {
            if (event->buttons() & Qt::LeftButton) {
                int deltaY = dragStartY_ - event->globalPosition().y();

                // If moved more than 3 pixels, consider it a drag
                if (std::abs(deltaY) > 3) {
                    if (!isDragging_) {
                        isDragging_ = true;
                        emit resizeDragStarted();
                    }
                    setCursor(Qt::SizeVerCursor);
                    emit resizeRequested(deltaY);
                    dragStartY_ = event->globalPosition().y();
                }
                event->accept();
            }
        }

        void mouseReleaseEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton) {
                setCursor(Qt::ArrowCursor);

                if (isDragging_) {
                    emit resizeDragEnded();
                } else {
                    // If it wasn't a drag, treat as a click to toggle collapse
                    emit collapseToggled();
                }

                isDragging_ = false;
                event->accept();
            }
        }

        void resizeEvent(QResizeEvent* event) override {
            QWidget::resizeEvent(event);
            if (background_) {
                background_->setGeometry(0, 0, width(), height());
            }
        }

    private:
        QWidget* background_;
        QPushButton* openButton_;
        QPushButton* playButton_;
        QPushButton* pauseButton_;
        bool isDragging_ = false;
        qreal dragStartY_ = 0;
};

}  // namespace spqr
