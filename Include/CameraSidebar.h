#pragma once

#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "CameraDisplayWidget.h"
#include "Robot.h"

namespace spqr {

class CameraSidebar : public QWidget {
    Q_OBJECT

   public:
    CameraSidebar(QWidget* parent = nullptr) : QWidget(parent) {
        setStyleSheet("QWidget { background-color: rgba(30, 30, 30, 230); }");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Toggle button
        toggleButton_ = new QPushButton("▲ Show Cameras ▲", this);
        toggleButton_->setStyleSheet("QPushButton { "
                                     "  background-color: #2d5016; "
                                     "  color: white; "
                                     "  border: 2px solid white; "
                                     "  padding: 5px; "
                                     "  font-weight: bold; "
                                     "} "
                                     "QPushButton:hover { background-color: #3d6026; }");
        toggleButton_->setFixedHeight(30);
        connect(toggleButton_, &QPushButton::clicked, this, &CameraSidebar::toggle);
        mainLayout->addWidget(toggleButton_);

        // Camera container
        cameraContainer_ = new QWidget(this);
        cameraContainer_->setStyleSheet("QWidget { background-color: rgba(30, 30, 30, 230); }");
        QHBoxLayout* cameraLayout = new QHBoxLayout(cameraContainer_);
        cameraLayout->setSpacing(10);
        cameraLayout->setContentsMargins(10, 10, 10, 10);
        cameraContainer_->setLayout(cameraLayout);
        mainLayout->addWidget(cameraContainer_);

        setLayout(mainLayout);

        // Start collapsed
        collapsed_ = true;
        cameraContainer_->hide();
        setFixedHeight(30);  // Just the button height

        // Timer to update camera feeds
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, &CameraSidebar::updateCameraFeeds);
        updateTimer_->start(33);  // ~30 FPS
    }

    void addRobotCameras(Robot* robot, const QString& robotName) {
        QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(cameraContainer_->layout());

        if (auto* t1 = dynamic_cast<T1*>(robot)) {
            auto* leftCam = new CameraDisplayWidget(t1->cameras[0], robotName + " - Left", this);
            auto* rightCam = new CameraDisplayWidget(t1->cameras[1], robotName + " - Right", this);

            layout->addWidget(leftCam);
            layout->addWidget(rightCam);

            cameraWidgets_.push_back(leftCam);
            cameraWidgets_.push_back(rightCam);
        } else if (auto* k1 = dynamic_cast<K1*>(robot)) {
            auto* leftCam = new CameraDisplayWidget(k1->cameras[0], robotName + " - Left", this);
            auto* rightCam = new CameraDisplayWidget(k1->cameras[1], robotName + " - Right", this);

            layout->addWidget(leftCam);
            layout->addWidget(rightCam);

            cameraWidgets_.push_back(leftCam);
            cameraWidgets_.push_back(rightCam);
        }
    }

   signals:
    void toggled(bool expanded);

   private slots:
    void toggle() {
        collapsed_ = !collapsed_;

        if (collapsed_) {
            toggleButton_->setText("▲ Show Cameras ▲");
            cameraContainer_->hide();
            setFixedHeight(30);  // Just button
        } else {
            toggleButton_->setText("▼ Hide Cameras ▼");
            cameraContainer_->show();
            setFixedHeight(300);  // Button + camera feeds (240px + margins)
        }

        emit toggled(!collapsed_);
    }

    void updateCameraFeeds() {
        if (!collapsed_) {
            for (auto* widget : cameraWidgets_) {
                widget->updateImage();
            }
        }
    }

   private:
    QPushButton* toggleButton_;
    QWidget* cameraContainer_;
    QTimer* updateTimer_;
    bool collapsed_;
    std::vector<CameraDisplayWidget*> cameraWidgets_;
};

}  // namespace spqr
