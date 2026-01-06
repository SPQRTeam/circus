#pragma once

#include <qobject.h>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Tool.h"

namespace spqr {

class ImageDisplay : public QWidget {
    Q_OBJECT

public:
    ImageDisplay(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget { "
                        "  background-color: #1a1a1a; "
                        "  border: none; "
                        "}");

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        imageLabel_ = new QLabel(this);
        imageLabel_->setAlignment(Qt::AlignCenter);
        imageLabel_->setScaledContents(false);
        imageLabel_->setStyleSheet("QLabel { "
                                   "  background-color: #1a1a1a; "
                                   "}");

        layout->addWidget(imageLabel_);
        setLayout(layout);
    }

    void setImage(const QImage& image) {
        if (image.isNull()) {
            return;
        }

        currentImage_ = image;
        updateDisplay();
    }

    void setImage(const unsigned char* data, int width, int height, int channels) {
        if (!data || width <= 0 || height <= 0) {
            return;
        }

        QImage::Format format;
        if (channels == 3) {
            format = QImage::Format_RGB888;
        } else if (channels == 4) {
            format = QImage::Format_RGBA8888;
        } else {
            return; // Unsupported format
        }

        currentImage_ = QImage(data, width, height, width * channels, format).copy();
        updateDisplay();
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        updateDisplay();
    }

private:
    void updateDisplay() {
        if (currentImage_.isNull()) {
            imageLabel_->clear();
            imageLabel_->setText("No image");
            imageLabel_->setStyleSheet("QLabel { "
                                      "  color: #888888; "
                                      "  font-size: 14px; "
                                      "  background-color: #1a1a1a; "
                                      "}");
            return;
        }

        // Scale image to fit widget while maintaining aspect ratio
        QSize labelSize = imageLabel_->size();
        QPixmap pixmap = QPixmap::fromImage(currentImage_);
        QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        imageLabel_->setPixmap(scaledPixmap);
        imageLabel_->setStyleSheet("QLabel { background-color: #1a1a1a; }");
    }

    QLabel* imageLabel_;
    QImage currentImage_;
};

class Image : public Tool {
    Q_OBJECT

public:
    Image(QWidget* parent = nullptr) : Tool(ToolType::IMAGE, parent) {
        // Clear the default "Select a source" label from base Tool class
        QLayout* oldLayout = layout();
        if (oldLayout) {
            QLayoutItem* item;
            while ((item = oldLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    delete item->widget();
                }
                delete item;
            }
            delete oldLayout;
        }

        // Create new layout
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Image display widget
        imageDisplay_ = new ImageDisplay(this);
        layout->addWidget(imageDisplay_, 1);

        setLayout(layout);
    }

    void setImage(const QImage& image) {
        imageDisplay_->setImage(image);
    }

    void setImage(const unsigned char* data, int width, int height, int channels = 3) {
        imageDisplay_->setImage(data, width, height, channels);
    }

    void update() override {
        // Trigger repaint if needed
        imageDisplay_->update();
    }

    ~Image() override = default;

private:
    ImageDisplay* imageDisplay_;
};

}  // namespace spqr
