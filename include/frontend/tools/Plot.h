#pragma once

#include <qdatetime.h>
#include <qobject.h>

#include <QCheckBox>
#include <QColor>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <deque>
#include <limits>
#include <random>
#include <vector>

#include "Tool.h"

namespace spqr {

struct TimeSeriesData {
        QString name;
        QColor color;
        std::deque<std::pair<double, double>> data;  // (timestamp, value)
        double currentValue = 0.0;
        bool visible = true;

        TimeSeriesData(const QString& n, const QColor& c) : name(n), color(c) {}
};

class PlotWidget : public QWidget {
        Q_OBJECT

    public:
        PlotWidget(QWidget* parent = nullptr)
            : QWidget(parent), timeWindow_(10.0), autoYBounds_(true), fixedMinY_(-10.0), fixedMaxY_(10.0) {
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

        void setTimeWindow(double seconds) {
            timeWindow_ = seconds;
            update();
        }

        void setYBoundsAuto(bool autoMode) {
            autoYBounds_ = autoMode;
            update();
        }

        void setFixedYBounds(double minY, double maxY) {
            fixedMinY_ = minY;
            fixedMaxY_ = maxY;
            update();
        }

        double getTimeWindow() const { return timeWindow_; }
        bool isYBoundsAuto() const { return autoYBounds_; }
        double getFixedMinY() const { return fixedMinY_; }
        double getFixedMaxY() const { return fixedMaxY_; }

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

            // Calculate time window using settings
            double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
            double minTime = currentTime - timeWindow_;
            double maxTime = currentTime;

            // Calculate value range
            double minValue, maxValue;

            if (autoYBounds_) {
                // Calculate value range from visible series only
                minValue = std::numeric_limits<double>::max();
                maxValue = std::numeric_limits<double>::lowest();
                bool hasData = false;

                for (const auto* series : timeSeries_) {
                    // Only consider visible series for Y-axis bounds
                    if (!series->visible) {
                        continue;
                    }

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
            } else {
                // Use fixed bounds
                minValue = fixedMinY_;
                maxValue = fixedMaxY_;
            }

            // Define plot area with margins
            const int leftMargin = 60;
            const int rightMargin = 20;
            const int topMargin = 20;
            const int bottomMargin = 40;

            QRect plotArea(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

            // Draw grid lines and axes
            painter.setPen(QPen(QColor(60, 60, 60), 1));

            // Horizontal grid lines (5 lines)
            for (int i = 0; i <= 5; i++) {
                int y = plotArea.top() + (plotArea.height() * i) / 5;
                painter.drawLine(plotArea.left(), y, plotArea.right(), y);

                // Y-axis labels
                double value = maxValue - (maxValue - minValue) * i / 5.0;
                painter.setPen(QColor(150, 150, 150));
                painter.drawText(QRect(0, y - 10, leftMargin - 5, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(value, 'f', 2));
                painter.setPen(QPen(QColor(60, 60, 60), 1));
            }

            // Vertical grid lines (10 lines)
            for (int i = 0; i <= 10; i++) {
                int x = plotArea.left() + (plotArea.width() * i) / 10;
                painter.drawLine(x, plotArea.top(), x, plotArea.bottom());

                // X-axis labels (time)
                if (i % 2 == 0) {
                    painter.setPen(QColor(150, 150, 150));
                    double timeValue = timeWindow_ * (10 - i) / 10.0;
                    QString timeLabel = QString("-%1s").arg(QString::number(timeValue, 'f', timeValue < 10 ? 1 : 0));
                    painter.drawText(QRect(x - 20, plotArea.bottom() + 5, 40, 20), Qt::AlignCenter, timeLabel);
                    painter.setPen(QPen(QColor(60, 60, 60), 1));
                }
            }

            // Draw time series
            for (const auto* series : timeSeries_) {
                if (series->data.empty() || !series->visible) {
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
        double timeWindow_;
        bool autoYBounds_;
        double fixedMinY_;
        double fixedMaxY_;
};

class Plot : public Tool {
        Q_OBJECT

    public:
        Plot(QWidget* parent = nullptr) : Tool(ToolType::PLOT, parent) {
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

            // Create horizontal layout for settings button and values panel
            QHBoxLayout* topLayout = new QHBoxLayout();
            topLayout->setSpacing(8);
            topLayout->setContentsMargins(0, 0, 0, 0);

            // Settings button (square)
            settingsButton_ = new QPushButton(this);
            settingsButton_->setText("⚙");
            settingsButton_->setFixedSize(40, 40);
            settingsButton_->setStyleSheet("QPushButton { "
                                           "  background-color: #252525; "
                                           "  border: 1px solid #444444; "
                                           "  border-radius: 3px; "
                                           "  color: white; "
                                           "  font-size: 16px; "
                                           "} "
                                           "QPushButton:hover { "
                                           "  background-color: #2a2a2a; "
                                           "  border-color: #555555; "
                                           "} "
                                           "QPushButton:pressed { "
                                           "  background-color: #202020; "
                                           "}");

            topLayout->addWidget(settingsButton_);

            // Current values section with horizontal scroll
            QScrollArea* valuesScrollArea = new QScrollArea(this);
            valuesScrollArea->setStyleSheet("QScrollArea { "
                                            "  background-color: #252525; "
                                            "  border: 1px solid #444444; "
                                            "  border-radius: 3px; "
                                            "} "
                                            "QScrollBar:horizontal { "
                                            "  height: 8px; "
                                            "  background-color: #1a1a1a; "
                                            "} "
                                            "QScrollBar::handle:horizontal { "
                                            "  background-color: #555555; "
                                            "  border-radius: 4px; "
                                            "} "
                                            "QScrollBar::handle:horizontal:hover { "
                                            "  background-color: #666666; "
                                            "}");
            valuesScrollArea->setFixedHeight(40);
            valuesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            valuesScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            valuesScrollArea->setWidgetResizable(false);

            valuesContainer_ = new QWidget();
            valuesContainer_->setStyleSheet("QWidget { "
                                            "  background-color: transparent; "
                                            "  border: none; "
                                            "}");

            valuesLayout_ = new QHBoxLayout(valuesContainer_);
            valuesLayout_->setContentsMargins(8, 6, 8, 6);
            valuesLayout_->setSpacing(20);
            valuesLayout_->setAlignment(Qt::AlignLeft);
            valuesLayout_->setSizeConstraint(QLayout::SetMinAndMaxSize);

            valuesScrollArea->setWidget(valuesContainer_);
            topLayout->addWidget(valuesScrollArea, 1);

            layout->addLayout(topLayout, 0);

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

            // Create settings panel (initially hidden)
            createSettingsPanel();

            // Connect settings button
            connect(settingsButton_, &QPushButton::clicked, this, &Plot::toggleSettingsPanel);
        }

        void addTimeSeries(const QString& name, const QColor& color = QColor()) {
            QColor seriesColor = color.isValid() ? color : generateRandomColor();

            TimeSeriesData* series = new TimeSeriesData(name, seriesColor);
            timeSeries_.push_back(series);

            // Create container for checkbox and label
            QWidget* seriesWidget = new QWidget(this);
            seriesWidget->setStyleSheet("QWidget { background-color: transparent; border: none; }");
            QHBoxLayout* seriesLayout = new QHBoxLayout(seriesWidget);
            seriesLayout->setContentsMargins(0, 0, 0, 0);
            seriesLayout->setSpacing(6);

            // Add checkbox with colored indicator
            QCheckBox* checkbox = new QCheckBox(this);
            checkbox->setChecked(true);
            QString colorStyle = QString(
                                     "QCheckBox { "
                                     "  background-color: transparent; "
                                     "  border: none; "
                                     "} "
                                     "QCheckBox::indicator { "
                                     "  width: 14px; "
                                     "  height: 14px; "
                                     "  border: 2px solid rgb(%1, %2, %3); "
                                     "  border-radius: 3px; "
                                     "  background-color: #252525; "
                                     "} "
                                     "QCheckBox::indicator:checked { "
                                     "  background-color: rgb(%1, %2, %3); "
                                     "}")
                                     .arg(seriesColor.red())
                                     .arg(seriesColor.green())
                                     .arg(seriesColor.blue());
            checkbox->setStyleSheet(colorStyle);

            // Connect checkbox to visibility toggle
            connect(checkbox, &QCheckBox::toggled, this, [this, series](bool checked) {
                series->visible = checked;
                updatePlot();
            });

            // Add label for current value
            QLabel* valueLabel = new QLabel(this);
            valueLabel->setStyleSheet("QLabel { "
                                      "  color: white; "
                                      "  font-size: 12px; "
                                      "  background-color: transparent; "
                                      "  border: none; "
                                      "  padding: 0px; "
                                      "  margin: 0px; "
                                      "}");
            valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            updateValueLabel(valueLabel, name, 0.0, seriesColor);

            seriesLayout->addWidget(checkbox);
            seriesLayout->addWidget(valueLabel);

            valuesLayout_->addWidget(seriesWidget);
            valueLabels_.push_back(valueLabel);
            checkboxes_.push_back(checkbox);
        }

        void addDataPoint(const QString& seriesName, double value) {
            double timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

            for (auto* series : timeSeries_) {
                if (series->name == seriesName) {
                    series->data.push_back({timestamp, value});
                    series->currentValue = value;

                    // Remove old data points (keep up to 600 seconds to allow time window changes)
                    double cutoffTime = timestamp - 600.0;
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

            for (auto* checkbox : checkboxes_) {
                delete checkbox;
            }
            checkboxes_.clear();
        }

        ~Plot() override {
            clearTimeSeries();
        }

    protected:
        void resizeEvent(QResizeEvent* event) override {
            Tool::resizeEvent(event);
            if (settingsPanel_ && plotWidget_) {
                // Position settings panel to cover the plot widget
                settingsPanel_->setGeometry(plotWidget_->geometry());
            }
        }

    private slots:
        void updatePlot() {
            // Update value labels
            for (size_t i = 0; i < timeSeries_.size() && i < valueLabels_.size(); i++) {
                updateValueLabel(valueLabels_[i], timeSeries_[i]->name, timeSeries_[i]->currentValue, timeSeries_[i]->color);
            }

            // Update plot
            plotWidget_->setTimeSeries(timeSeries_);
        }

        void toggleSettingsPanel() {
            if (settingsPanel_->isVisible()) {
                // Apply settings and close
                applySettings();
                settingsPanel_->hide();
            } else {
                // Show settings panel
                settingsPanel_->show();
                settingsPanel_->raise();
            }
        }

    private:
        void updateValueLabel(QLabel* label, const QString& name, double value, const QColor& color) {
            label->setText(QString("<b>%1:</b> %2").arg(name).arg(value, 0, 'f', 3));
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
            return std::sqrt(dr * dr + dg * dg + db * db);
        }

        void createSettingsPanel() {
            // Create settings panel as overlay
            settingsPanel_ = new QWidget(this);
            settingsPanel_->setStyleSheet("QWidget { "
                                          "  background-color: rgba(26, 26, 26, 240); "
                                          "  border: 1px solid #444444; "
                                          "  border-radius: 5px; "
                                          "}");
            settingsPanel_->hide();

            QVBoxLayout* panelLayout = new QVBoxLayout(settingsPanel_);
            panelLayout->setContentsMargins(20, 20, 20, 20);
            panelLayout->setSpacing(10);

            // Title
            // QLabel* titleLabel = new QLabel("Plot Settings", settingsPanel_);
            // titleLabel->setStyleSheet("QLabel { "
            //                           "  color: white; "
            //                           "  font-size: 15px; "
            //                           "  font-weight: bold; "
            //                           "  background-color: transparent; "
            //                           "  border: none; "
            //                           "}");
            // panelLayout->addWidget(titleLabel);

            // Time window setting
            QGroupBox* timeGroup = new QGroupBox("Timeline", settingsPanel_);
            timeGroup->setStyleSheet("QGroupBox { "
                                     "  color: white; "
                                     "  font-size: 14px; "
                                     "  font-weight: bold; "
                                     "  border: 1px solid #555555; "
                                     "  border-radius: 3px; "
                                     "  margin-top: 10px; "
                                     "  padding-top: 10px; "
                                     "  background-color: transparent; "
                                     "} "
                                     "QGroupBox::title { "
                                     "  subcontrol-origin: margin; "
                                     "  subcontrol-position: top left; "
                                     "  left: 10px; "
                                     "  padding: 0 5px; "
                                     "}");
            QFormLayout* timeLayout = new QFormLayout(timeGroup);
            timeLayout->setLabelAlignment(Qt::AlignLeft);

            // Create white label for time window
            QLabel* timeWindowLabel = new QLabel("Time window:", settingsPanel_);
            timeWindowLabel->setStyleSheet("QLabel { "
                                          "  color: white; "
                                          "  background-color: transparent; "
                                          "  border: none; "
                                          "}");

            timeWindowSpinBox_ = new QSpinBox(settingsPanel_);
            timeWindowSpinBox_->setMinimum(1);
            timeWindowSpinBox_->setMaximum(300);
            timeWindowSpinBox_->setValue(static_cast<int>(plotWidget_->getTimeWindow()));
            timeWindowSpinBox_->setSuffix(" seconds");
            timeWindowSpinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
            timeWindowSpinBox_->setFixedHeight(30);
            timeWindowSpinBox_->setStyleSheet("QSpinBox { "
                                              "  color: white; "
                                              "  background-color: #252525; "
                                              "  border: 1px solid #444444; "
                                              "  border-radius: 3px; "
                                              "  padding: 5px; "
                                              "}"
                                            );
            timeLayout->addRow(timeWindowLabel, timeWindowSpinBox_);

            panelLayout->addWidget(timeGroup);

            // Y-Axis bounds setting
            QGroupBox* yBoundsGroup = new QGroupBox("Bounds", settingsPanel_);
            yBoundsGroup->setStyleSheet("QGroupBox { "
                                        "  color: white; "
                                        "  font-size: 14px; "
                                        "  font-weight: bold; "
                                        "  border: 1px solid #555555; "
                                        "  border-radius: 3px; "
                                        "  margin-top: 10px; "
                                        "  padding-top: 10px; "
                                        "  background-color: transparent; "
                                        "} "
                                        "QGroupBox::title { "
                                        "  subcontrol-origin: margin; "
                                        "  subcontrol-position: top left; "
                                        "  left: 10px; "
                                        "  padding: 0 5px; "
                                        "}");
            QVBoxLayout* yBoundsLayout = new QVBoxLayout(yBoundsGroup);

            autoYBoundsRadio_ = new QRadioButton("Automatic", settingsPanel_);
            autoYBoundsRadio_->setChecked(plotWidget_->isYBoundsAuto());
            autoYBoundsRadio_->setStyleSheet("QRadioButton { "
                                             "  color: white; "
                                             "  background-color: transparent; "
                                             "  border: 1px solid #444444; "
                                             "  padding: 5px; "
                                             "} "
                                             "QRadioButton::indicator { "
                                             "  width: 10px; "
                                             "  height: 10px; "
                                             "  border: 1px solid #444444; "
                                             "  border-radius: 3px; "
                                             "  background-color: transparent; "
                                             "} "
                                             "QRadioButton::indicator:checked { "
                                             "  background-color: #444444; "
                                             "  border: 1px solid #606060; "
                                            "   border-radius: 3px; "
                                             "}");
            yBoundsLayout->addWidget(autoYBoundsRadio_);

            fixedYBoundsRadio_ = new QRadioButton("Fixed", settingsPanel_);
            fixedYBoundsRadio_->setChecked(!plotWidget_->isYBoundsAuto());
            fixedYBoundsRadio_->setStyleSheet("QRadioButton { "
                                              "  color: white; "
                                              "  background-color: transparent; "
                                              "  border: 1px solid #444444; "
                                              "  padding: 5px; "
                                              "} "
                                              "QRadioButton::indicator { "
                                              "  width: 10px; "
                                              "  height: 10px; "
                                              "  border: 1px solid #444444; "
                                              "  border-radius: 3px; "
                                              "  background-color: transparent; "
                                              "} "
                                              "QRadioButton::indicator:checked { "
                                              "  background-color: #444444; "
                                              "  border: 1px solid #606060; "
                                              "  border-radius: 3px; "
                                              "}");
            yBoundsLayout->addWidget(fixedYBoundsRadio_);

            QFormLayout* fixedBoundsLayout = new QFormLayout();
            fixedBoundsLayout->setVerticalSpacing(10);

            // Create white labels
            QLabel* upperBoundLabel = new QLabel("Upper bound:", settingsPanel_);
            upperBoundLabel->setStyleSheet("QLabel { "
                                          "  color: white; "
                                          "  background-color: transparent; "
                                          "  border: none; "
                                          "}");
            QLabel* lowerBoundLabel = new QLabel("Lower bound:", settingsPanel_);
            lowerBoundLabel->setStyleSheet("QLabel { "
                                          "  color: white; "
                                          "  background-color: transparent; "
                                          "  border: none; "
                                          "}");

            maxYSpinBox_ = new QDoubleSpinBox(settingsPanel_);
            maxYSpinBox_->setMinimum(-10000.0);
            maxYSpinBox_->setMaximum(10000.0);
            maxYSpinBox_->setValue(plotWidget_->getFixedMaxY());
            maxYSpinBox_->setDecimals(2);
            maxYSpinBox_->setEnabled(!plotWidget_->isYBoundsAuto());
            maxYSpinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
            maxYSpinBox_->setFixedHeight(30);
            maxYSpinBox_->setStyleSheet("QDoubleSpinBox { "
                                        "  color: white; "
                                        "  background-color: #252525; "
                                        "  border: 1px solid #444444; "
                                        "  border-radius: 3px; "
                                        "  padding: 5px; "
                                        "}");
            fixedBoundsLayout->addRow(upperBoundLabel, maxYSpinBox_);

            fixedBoundsLayout->setVerticalSpacing(10);

            minYSpinBox_ = new QDoubleSpinBox(settingsPanel_);
            minYSpinBox_->setMinimum(-10000.0);
            minYSpinBox_->setMaximum(10000.0);
            minYSpinBox_->setValue(plotWidget_->getFixedMinY());
            minYSpinBox_->setDecimals(2);
            minYSpinBox_->setEnabled(!plotWidget_->isYBoundsAuto());
            minYSpinBox_->setButtonSymbols(QAbstractSpinBox::NoButtons);
            minYSpinBox_->setFixedHeight(30);
            minYSpinBox_->setStyleSheet("QDoubleSpinBox { "
                                        "  color: white; "
                                        "  background-color: #252525; "
                                        "  border: 1px solid #444444; "
                                        "  border-radius: 3px; "
                                        "  padding: 5px; "
                                        "}");
            fixedBoundsLayout->addRow(lowerBoundLabel, minYSpinBox_);


            yBoundsLayout->addLayout(fixedBoundsLayout);
            panelLayout->addWidget(yBoundsGroup);

            // Connect radio buttons to enable/disable spin boxes
            connect(autoYBoundsRadio_, &QRadioButton::toggled, this, [this](bool checked) {
                minYSpinBox_->setEnabled(!checked);
                maxYSpinBox_->setEnabled(!checked);
            });

            panelLayout->addStretch();
        }

        void applySettings() {
            // Apply time window setting
            plotWidget_->setTimeWindow(static_cast<double>(timeWindowSpinBox_->value()));

            // Apply Y-axis bounds setting
            if (autoYBoundsRadio_->isChecked()) {
                plotWidget_->setYBoundsAuto(true);
            } else {
                plotWidget_->setYBoundsAuto(false);
                plotWidget_->setFixedYBounds(minYSpinBox_->value(), maxYSpinBox_->value());
            }
        }

        std::vector<TimeSeriesData*> timeSeries_;
        std::vector<QLabel*> valueLabels_;
        std::vector<QCheckBox*> checkboxes_;

        QPushButton* settingsButton_;
        QWidget* valuesContainer_;
        QHBoxLayout* valuesLayout_;
        PlotWidget* plotWidget_;
        QTimer* updateTimer_;

        // Settings panel widgets
        QWidget* settingsPanel_;
        QSpinBox* timeWindowSpinBox_;
        QRadioButton* autoYBoundsRadio_;
        QRadioButton* fixedYBoundsRadio_;
        QDoubleSpinBox* minYSpinBox_;
        QDoubleSpinBox* maxYSpinBox_;

        std::mt19937 randomEngine_;
};

}  // namespace spqr
