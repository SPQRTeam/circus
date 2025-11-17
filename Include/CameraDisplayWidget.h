#pragma once

#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

#include "Camera.h"

namespace spqr {

class CameraDisplayWidget : public QWidget {
    Q_OBJECT

   public:
    CameraDisplayWidget(Camera* camera, const QString& label, QWidget* parent = nullptr)
        : QWidget(parent), camera_(camera) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);

        // Label showing camera name
        labelText_ = new QLabel(label, this);
        labelText_->setStyleSheet("QLabel { color: white; font-weight: bold; }");
        labelText_->setAlignment(Qt::AlignCenter);
        layout->addWidget(labelText_);

        // Image display
        imageLabel_ = new QLabel(this);
        imageLabel_->setMinimumSize(320, 240);  // Half resolution for display
        imageLabel_->setMaximumSize(320, 240);
        imageLabel_->setScaledContents(true);
        imageLabel_->setStyleSheet("QLabel { border: 2px solid white; background-color: black; }");
        layout->addWidget(imageLabel_);

        setLayout(layout);
    }

    void updateImage() {
        int width = camera_->getWidth();
        int height = camera_->getHeight();

        // Get a copy of the image data (thread-safe)
        std::vector<uint8_t> imageData;
        camera_->copyImageTo(imageData);

        if (imageData.empty())
            return;

        // Convert to QImage (RGB888 format)
        QImage image(imageData.data(), width, height, width * 3, QImage::Format_RGB888);

        // Create a deep copy since imageData will go out of scope
        QImage imageCopy = image.copy();

        // Update the label
        imageLabel_->setPixmap(QPixmap::fromImage(imageCopy));
    }

   private:
    Camera* camera_;
    QLabel* labelText_;
    QLabel* imageLabel_;
};

}  // namespace spqr
