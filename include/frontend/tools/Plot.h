#pragma once

#include <qdatetime.h>
#include <qobject.h>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <deque>
#include <vector>
#include <random>
#include <cmath>
#include <limits>

#include "Tool.h"

namespace spqr {

struct TimeSeriesData {
    QString name;
    QColor color;
    std::deque<std::pair<double, double>> data; // (timestamp, value)
    double currentValue = 0.0;

    TimeSeriesData(const QString& n, const QColor& c) : name(n), color(c) {}
};

class PlotWidget : public QWidget {
    Q_OBJECT

public:
    PlotWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget { "
                        "  background-color: #1a1a1a; "
                        "  border: none; "
                        "}");
        setMinimumHeight(150);
    }

    void setTimeSeries(const std::vector<TimeSeriesData*>& series) {
        timeSeries_ = series;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw background
        painter.fillRect(rect(), QColor(26, 26, 26));

        if (timeSeries_.empty()) {
            return;
        }

        // Calculate time window (last 10 seconds)
        double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
        double minTime = currentTime - 10.0;
        double maxTime = currentTime;

        // Calculate value range from all series
        double minValue = std::numeric_limits<double>::max();
        double maxValue = std::numeric_limits<double>::lowest();
        bool hasData = false;

        for (const auto* series : timeSeries_) {
            for (const auto& point : series->data) {
                if (point.first >= minTime) {
                    minValue = std::min(minValue, point.second);
                    maxValue = std::max(maxValue, point.second);
                    hasData = true;
                }
            }
        }

        if (!hasData) {
            minValue = -1.0;
            maxValue = 1.0;
        } else {
            // Add 10% padding to the range
            double range = maxValue - minValue;
            if (range < 1e-6) {
                range = 1.0;
            }
            minValue -= range * 0.1;
            maxValue += range * 0.1;
        }

        // Define plot area with margins
        const int leftMargin = 60;
        const int rightMargin = 20;
        const int topMargin = 20;
        const int bottomMargin = 40;

        QRect plotArea(leftMargin, topMargin,
                      width() - leftMargin - rightMargin,
                      height() - topMargin - bottomMargin);

        // Draw grid lines and axes
        painter.setPen(QPen(QColor(60, 60, 60), 1));

        // Horizontal grid lines (5 lines)
        for (int i = 0; i <= 5; i++) {
            int y = plotArea.top() + (plotArea.height() * i) / 5;
            painter.drawLine(plotArea.left(), y, plotArea.right(), y);

            // Y-axis labels
            double value = maxValue - (maxValue - minValue) * i / 5.0;
            painter.setPen(QColor(150, 150, 150));
            painter.drawText(QRect(0, y - 10, leftMargin - 5, 20),
                           Qt::AlignRight | Qt::AlignVCenter,
                           QString::number(value, 'f', 2));
            painter.setPen(QPen(QColor(60, 60, 60), 1));
        }

        // Vertical grid lines (10 lines for 10 seconds)
        for (int i = 0; i <= 10; i++) {
            int x = plotArea.left() + (plotArea.width() * i) / 10;
            painter.drawLine(x, plotArea.top(), x, plotArea.bottom());

            // X-axis labels (time)
            if (i % 2 == 0) {
                painter.setPen(QColor(150, 150, 150));
                QString timeLabel = QString("-%1s").arg(10 - i);
                painter.drawText(QRect(x - 20, plotArea.bottom() + 5, 40, 20),
                               Qt::AlignCenter, timeLabel);
                painter.setPen(QPen(QColor(60, 60, 60), 1));
            }
        }

        // Draw time series
        for (const auto* series : timeSeries_) {
            if (series->data.empty()) {
                continue;
            }

            painter.setPen(QPen(series->color, 2));

            QPointF lastPoint;
            bool firstPoint = true;

            for (const auto& point : series->data) {
                if (point.first < minTime) {
                    continue;
                }

                // Map data point to screen coordinates
                double xNorm = (point.first - minTime) / (maxTime - minTime);
                double yNorm = (maxValue - point.second) / (maxValue - minValue);

                int x = plotArea.left() + static_cast<int>(xNorm * plotArea.width());
                int y = plotArea.top() + static_cast<int>(yNorm * plotArea.height());

                QPointF currentPoint(x, y);

                if (!firstPoint) {
                    painter.drawLine(lastPoint, currentPoint);
                }

                lastPoint = currentPoint;
                firstPoint = false;
            }
        }
    }

private:
    std::vector<TimeSeriesData*> timeSeries_;
};

class Plot : public Tool {
    Q_OBJECT

public:
    Plot(QWidget* parent = nullptr) : Tool(ToolType::PLOT, parent) {
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
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        // Current values section
        valuesContainer_ = new QWidget(this);
        valuesContainer_->setStyleSheet("QWidget { "
                                       "  background-color: #252525; "
                                       "  border: 1px solid #444444; "
                                       "  border-radius: 3px; "
                                       "}");
        valuesLayout_ = new QVBoxLayout(valuesContainer_);
        valuesLayout_->setContentsMargins(8, 6, 8, 6);
        valuesLayout_->setSpacing(4);

        layout->addWidget(valuesContainer_, 0);

        // Plot widget
        plotWidget_ = new PlotWidget(this);
        layout->addWidget(plotWidget_, 1);

        setLayout(layout);

        // Setup update timer (30 FPS)
        updateTimer_ = new QTimer(this);
        connect(updateTimer_, &QTimer::timeout, this, &Plot::updatePlot);
        updateTimer_->start(33);

        // Initialize random number generator for colors
        randomEngine_.seed(std::random_device{}());
    }

    void addTimeSeries(const QString& name, const QColor& color = QColor()) {
        QColor seriesColor = color.isValid() ? color : generateRandomColor();

        TimeSeriesData* series = new TimeSeriesData(name, seriesColor);
        timeSeries_.push_back(series);

        // Add label for current value
        QLabel* valueLabel = new QLabel(this);
        valueLabel->setStyleSheet("QLabel { "
                                 "  color: white; "
                                 "  font-size: 12px; "
                                 "  background-color: transparent; "
                                 "  border: none; "
                                 "}");
        updateValueLabel(valueLabel, name, 0.0, seriesColor);
        valuesLayout_->addWidget(valueLabel);
        valueLabels_.push_back(valueLabel);
    }

    void addDataPoint(const QString& seriesName, double value) {
        double timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

        for (auto* series : timeSeries_) {
            if (series->name == seriesName) {
                series->data.push_back({timestamp, value});
                series->currentValue = value;

                // Remove old data points (older than 10 seconds)
                double cutoffTime = timestamp - 10.0;
                while (!series->data.empty() && series->data.front().first < cutoffTime) {
                    series->data.pop_front();
                }

                break;
            }
        }
    }

    void clearTimeSeries() {
        for (auto* series : timeSeries_) {
            delete series;
        }
        timeSeries_.clear();

        for (auto* label : valueLabels_) {
            delete label;
        }
        valueLabels_.clear();
    }

    ~Plot() override {
        clearTimeSeries();
    }

private slots:
    void updatePlot() {
        // Update value labels
        for (size_t i = 0; i < timeSeries_.size() && i < valueLabels_.size(); i++) {
            updateValueLabel(valueLabels_[i], timeSeries_[i]->name,
                           timeSeries_[i]->currentValue, timeSeries_[i]->color);
        }

        // Update plot
        plotWidget_->setTimeSeries(timeSeries_);
    }

private:
    void updateValueLabel(QLabel* label, const QString& name, double value, const QColor& color) {
        QString colorHex = QString("rgb(%1, %2, %3)")
                              .arg(color.red())
                              .arg(color.green())
                              .arg(color.blue());

        label->setText(QString("<span style='color: %1;'>●</span> <b>%2:</b> %3")
                          .arg(colorHex)
                          .arg(name)
                          .arg(value, 0, 'f', 3));
    }

    QColor generateRandomColor() {
        std::uniform_int_distribution<int> dist(100, 255);

        // Generate colors that are visually distinct
        QColor color;
        bool validColor = false;
        int attempts = 0;

        while (!validColor && attempts < 100) {
            int r = dist(randomEngine_);
            int g = dist(randomEngine_);
            int b = dist(randomEngine_);

            color = QColor(r, g, b);

            // Check if color is different enough from existing colors
            validColor = true;
            for (const auto* series : timeSeries_) {
                if (colorDistance(color, series->color) < 100) {
                    validColor = false;
                    break;
                }
            }

            attempts++;
        }

        return color;
    }

    double colorDistance(const QColor& c1, const QColor& c2) const {
        int dr = c1.red() - c2.red();
        int dg = c1.green() - c2.green();
        int db = c1.blue() - c2.blue();
        return std::sqrt(dr*dr + dg*dg + db*db);
    }

    std::vector<TimeSeriesData*> timeSeries_;
    std::vector<QLabel*> valueLabels_;

    QWidget* valuesContainer_;
    QVBoxLayout* valuesLayout_;
    PlotWidget* plotWidget_;
    QTimer* updateTimer_;

    std::mt19937 randomEngine_;
};

}  // namespace spqr
