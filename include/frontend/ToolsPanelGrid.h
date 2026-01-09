#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "robots/Robot.h"
#include "sensors/Camera.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Pose.h"
#include "sensors/Sensor.h"
#include "tools/Image.h"
#include "tools/Plot.h"
#include "tools/Terminal.h"
#include "tools/Tool.h"

namespace spqr {

class GridCell : public QWidget {
        Q_OBJECT

    public:
        GridCell(std::vector<std::shared_ptr<Robot>> robots, QMap<QString, ToolType> streams, QWidget* parent = nullptr) : QWidget(parent) {
            robots_ = robots;
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                          "  background-color: #2a2a2a; "
                          "  border: 1px solid #444444; "
                          "  border-radius: 3px;"
                          "}"
                          "QWidget:hover { "
                          "  background-color: #3a3a3a; "
                          "  border: 2px solid #006778; "
                          "}");
            setMinimumSize(400, 300);

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(6, 6, 6, 6);
            layout->setSpacing(6);

            QComboBox* combo = new QComboBox(this);
            combo->setMaxVisibleItems(6);
            combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            combo->setEditable(true);

            QListView* popupView = new QListView(combo);
            popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            popupView->setFrameShape(QFrame::NoFrame);
            popupView->setSpacing(0);
            popupView->setUniformItemSizes(true);
            popupView->setContentsMargins(0, 0, 0, 0);
            popupView->setStyleSheet("QListView { "
                                     "  background-color: #2a2a2a; "
                                     "  color: white; "
                                     "  border: 1px solid #006778; "
                                     "  padding: 0; "
                                     "  margin: 0; "
                                     "  outline: 0; "
                                     "} "
                                     "QListView::item { "
                                     "  padding: 6px 10px; "
                                     "  margin: 0; "
                                     "} "
                                     "QListView::item:selected { "
                                     "  background-color: #006778; "
                                     "  color: white; "
                                     "}");
            combo->setView(popupView);

            combo->setStyleSheet("QComboBox { "
                                 "  background-color: #444444; "
                                 "  color: white; "
                                 "  border: 1px solid #666666; "
                                 "  border-radius: 3px; "
                                 "  padding: 5px 10px 5px 10px; "
                                 "  font-size: 13px; "
                                 "} "
                                 "QComboBox:hover { "
                                 "  background-color: #595959; "
                                 "  border: 1px solid #006778; "
                                 "} ");

            combo->addItems(streams.keys());
            selectedItem_ = combo->currentText();
            connect(combo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
                selectedItem_ = text;
                onSourceChanged();
            });
            layout->addWidget(combo, 0, Qt::AlignTop);

            // Add the tool widget that fills the remaining space
            tool_ = new Tool(ToolType::NONE, this);
            cellLayout_ = layout;
            layout->addWidget(tool_, 1);

            streams_ = streams;
        }

        QString selectedItem() const {
            return selectedItem_;
        }

        Tool* getTool() const {
            return tool_;
        }

        void updateTool() {
            if (tool_ && tool_->type() != ToolType::NONE) {
                tool_->update();
            }
        }

    private:
        void setTool(Tool* tool) {
            if (tool_ && cellLayout_) {
                cellLayout_->removeWidget(tool_);
                tool_->deleteLater();
            }
            tool_ = tool;
            if (cellLayout_) {
                cellLayout_->addWidget(tool_, 1);
            }
        }

        void onSourceChanged() {
            if (selectedItem_.isEmpty() || !streams_.contains(selectedItem_)) {
                setTool(new Tool(ToolType::NONE, this));
                return;
            }

            ToolType newToolType = streams_[selectedItem_];

            // ALWAYS create a new tool when source changes (not just when type differs)
            Tool* newTool = nullptr;

            switch (newToolType) {
                case ToolType::PLOT:
                    newTool = new Plot(this);

                    if (selectedItem_.contains("/position")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("X", QColor(255, 0, 0));
                        plot->addTimeSeries("Y", QColor(0, 255, 0));
                        plot->addTimeSeries("Z", QColor(0, 0, 255));
                    } else if (selectedItem_.contains("/orientation")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("Roll", QColor(255, 0, 0));
                        plot->addTimeSeries("Pitch", QColor(0, 255, 0));
                        plot->addTimeSeries("Yaw", QColor(0, 0, 255));
                    } else if (selectedItem_.contains("/joints_position")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("head_yaw");
                        plot->addTimeSeries("head_pitch");
                        plot->addTimeSeries("shoulder_left_pitch");
                        plot->addTimeSeries("shoulder_left_roll");
                        plot->addTimeSeries("elbow_left_pitch");
                        plot->addTimeSeries("elbow_left_yaw");
                        plot->addTimeSeries("shoulder_right_pitch");
                        plot->addTimeSeries("shoulder_right_roll");
                        plot->addTimeSeries("elbow_right_pitch");
                        plot->addTimeSeries("elbow_right_yaw");
                        plot->addTimeSeries("waist");
                        plot->addTimeSeries("hip_left_pitch");
                        plot->addTimeSeries("hip_left_roll");
                        plot->addTimeSeries("hip_left_yaw");
                        plot->addTimeSeries("knee_left_pitch");
                        plot->addTimeSeries("ankle_left_pitch");
                        plot->addTimeSeries("ankle_left_roll");
                        plot->addTimeSeries("hip_right_pitch");
                        plot->addTimeSeries("hip_right_roll");
                        plot->addTimeSeries("hip_right_yaw");
                        plot->addTimeSeries("knee_right_pitch");
                        plot->addTimeSeries("ankle_right_pitch");
                        plot->addTimeSeries("ankle_right_roll");
                    } else if (selectedItem_.contains("/joints_velocity")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("head_yaw");
                        plot->addTimeSeries("head_pitch");
                        plot->addTimeSeries("shoulder_left_pitch");
                        plot->addTimeSeries("shoulder_left_roll");
                        plot->addTimeSeries("elbow_left_pitch");
                        plot->addTimeSeries("elbow_left_yaw");
                        plot->addTimeSeries("shoulder_right_pitch");
                        plot->addTimeSeries("shoulder_right_roll");
                        plot->addTimeSeries("elbow_right_pitch");
                        plot->addTimeSeries("elbow_right_yaw");
                        plot->addTimeSeries("waist");
                        plot->addTimeSeries("hip_left_pitch");
                        plot->addTimeSeries("hip_left_roll");
                        plot->addTimeSeries("hip_left_yaw");
                        plot->addTimeSeries("knee_left_pitch");
                        plot->addTimeSeries("ankle_left_pitch");
                        plot->addTimeSeries("ankle_left_roll");
                        plot->addTimeSeries("hip_right_pitch");
                        plot->addTimeSeries("hip_right_roll");
                        plot->addTimeSeries("hip_right_yaw");
                        plot->addTimeSeries("knee_right_pitch");
                        plot->addTimeSeries("ankle_right_pitch");
                        plot->addTimeSeries("ankle_right_roll");
                    } else if (selectedItem_.contains("/joints_acceleration")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("head_yaw");
                        plot->addTimeSeries("head_pitch");
                        plot->addTimeSeries("shoulder_left_pitch");
                        plot->addTimeSeries("shoulder_left_roll");
                        plot->addTimeSeries("elbow_left_pitch");
                        plot->addTimeSeries("elbow_left_yaw");
                        plot->addTimeSeries("shoulder_right_pitch");
                        plot->addTimeSeries("shoulder_right_roll");
                        plot->addTimeSeries("elbow_right_pitch");
                        plot->addTimeSeries("elbow_right_yaw");
                        plot->addTimeSeries("waist");
                        plot->addTimeSeries("hip_left_pitch");
                        plot->addTimeSeries("hip_left_roll");
                        plot->addTimeSeries("hip_left_yaw");
                        plot->addTimeSeries("knee_left_pitch");
                        plot->addTimeSeries("ankle_left_pitch");
                        plot->addTimeSeries("ankle_left_roll");
                        plot->addTimeSeries("hip_right_pitch");
                        plot->addTimeSeries("hip_right_roll");
                        plot->addTimeSeries("hip_right_yaw");
                        plot->addTimeSeries("knee_right_pitch");
                        plot->addTimeSeries("ankle_right_pitch");
                        plot->addTimeSeries("ankle_right_roll");
                    } else if (selectedItem_.contains("/joints_torque")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("head_yaw");
                        plot->addTimeSeries("head_pitch");
                        plot->addTimeSeries("shoulder_left_pitch");
                        plot->addTimeSeries("shoulder_left_roll");
                        plot->addTimeSeries("elbow_left_pitch");
                        plot->addTimeSeries("elbow_left_yaw");
                        plot->addTimeSeries("shoulder_right_pitch");
                        plot->addTimeSeries("shoulder_right_roll");
                        plot->addTimeSeries("elbow_right_pitch");
                        plot->addTimeSeries("elbow_right_yaw");
                        plot->addTimeSeries("waist");
                        plot->addTimeSeries("hip_left_pitch");
                        plot->addTimeSeries("hip_left_roll");
                        plot->addTimeSeries("hip_left_yaw");
                        plot->addTimeSeries("knee_left_pitch");
                        plot->addTimeSeries("ankle_left_pitch");
                        plot->addTimeSeries("ankle_left_roll");
                        plot->addTimeSeries("hip_right_pitch");
                        plot->addTimeSeries("hip_right_roll");
                        plot->addTimeSeries("hip_right_yaw");
                        plot->addTimeSeries("knee_right_pitch");
                        plot->addTimeSeries("ankle_right_pitch");
                        plot->addTimeSeries("ankle_right_roll");
                    } else if (selectedItem_.contains("/linear_acceleration")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("Ax", QColor(255, 0, 0));
                        plot->addTimeSeries("Ay", QColor(0, 255, 0));
                        plot->addTimeSeries("Az", QColor(0, 0, 255));
                    } else if (selectedItem_.contains("/angular_velocity")) {
                        Plot* plot = dynamic_cast<Plot*>(newTool);
                        plot->addTimeSeries("Wx", QColor(255, 0, 0));
                        plot->addTimeSeries("Wy", QColor(0, 255, 0));
                        plot->addTimeSeries("Wz", QColor(0, 0, 255));
                    }

                    break;

                case ToolType::IMAGE:
                    newTool = new Image(this);
                    break;

                case ToolType::TERMINAL: {
                    // Extract robot name from selectedItem_ (format: "robot_name/terminal")
                    QString robotName = selectedItem_.split("/").first();
                    std::shared_ptr<Container> container = nullptr;

                    // Find the robot and its container
                    for (auto& robot : robots_) {
                        if (QString::fromStdString(robot->name) == robotName) {
                            if (robot->container) {
                                container = std::shared_ptr<Container>(robot->container.get(), [](Container*) {});
                            }
                            break;
                        }
                    }

                    if (container) {
                        newTool = new Terminal(container, this);
                    } else {
                        newTool = new Tool(ToolType::NONE, this);
                    }
                    break;
                }

                case ToolType::NONE:
                    newTool = new Tool(ToolType::NONE, this);
                    break;

                default:
                    newTool = new Tool(ToolType::NONE, this);
                    break;
            }

            setTool(newTool);
        }

        QString selectedItem_;
        Tool* tool_;
        QVBoxLayout* cellLayout_;
        QMap<QString, ToolType> streams_;
        std::vector<std::shared_ptr<Robot>> robots_;
};

class ToolsPanelGrid : public QWidget {
        Q_OBJECT

    public:
        ToolsPanelGrid(std::vector<std::shared_ptr<Robot>> robots, QMap<QString, ToolType> streams, QWidget* parent = nullptr) : QWidget(parent) {
            robots_ = robots;
            streams_ = streams;

            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            // Create scroll area for the grid
            scrollArea_ = new QScrollArea(this);
            scrollArea_->setWidgetResizable(true);  // Allow resize to fill viewport
            scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollArea_->setStyleSheet("QScrollArea { "
                                       "  background-color: #1a1a1a; "
                                       "  border: none; "
                                       "}"
                                       "QScrollBar:horizontal, QScrollBar:vertical { "
                                       "  background-color: #2a2a2a; "
                                       "  border: none; "
                                       "  height: 12px; "
                                       "  width: 12px; "
                                       "}"
                                       "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { "
                                       "  background-color: #555555; "
                                       "  border-radius: 6px; "
                                       "  min-width: 20px; "
                                       "  min-height: 20px; "
                                       "}"
                                       "QScrollBar::handle:horizontal:hover, QScrollBar::handle:vertical:hover { "
                                       "  background-color: #006778; "
                                       "}"
                                       "QScrollBar::add-line, QScrollBar::sub-line { "
                                       "  border: none; "
                                       "  background: none; "
                                       "}");

            // Create the grid container
            gridContainer_ = new QWidget();
            scrollArea_->setWidget(gridContainer_);
            mainLayout->addWidget(scrollArea_);

            // Initialize with 1x1 grid
            numRows_ = 1;
            numCols_ = 1;
            rebuildGrid();

            setLayout(mainLayout);

            // Setup update timer (500ms)
            updateTimer_ = new QTimer(this);
            connect(updateTimer_, &QTimer::timeout, this, &ToolsPanelGrid::updateAllCells);
            updateTimer_->start(10);
        }

        int getNumRows() const {
            return numRows_;
        }
        int getNumCols() const {
            return numCols_;
        }

    public slots:
        void addRow() {
            numRows_++;
            rebuildGrid();
            emit gridSizeChanged(numRows_, numCols_);
        }

        void removeRow() {
            if (numRows_ > 1) {
                numRows_--;
                rebuildGrid();
                emit gridSizeChanged(numRows_, numCols_);
            }
        }

        void addColumn() {
            numCols_++;
            rebuildGrid();
            emit gridSizeChanged(numRows_, numCols_);
        }

        void removeColumn() {
            if (numCols_ > 1) {
                numCols_--;
                rebuildGrid();
                emit gridSizeChanged(numRows_, numCols_);
            }
        }

    signals:
        void gridSizeChanged(int rows, int cols);

    private slots:
        void updateAllCells() {
            // Find all GridCell widgets and update their tools
            QList<GridCell*> cells = gridContainer_->findChildren<GridCell*>();

            for (GridCell* cell : cells) {
                // Get the selected stream name for this cell
                QString selectedStream = cell->selectedItem();
                // Parse the stream name (format: "robotName/sensorType")
                QStringList parts = selectedStream.split('/');
                if (parts.size() == 2) {
                    QString robotName = parts[0];
                    QString sensorType = parts[1];

                    // Find the matching robot
                    for (std::shared_ptr<Robot>& robot : robots_) {
                        if (QString::fromStdString(robot->name) == robotName) {
                            // Get sensor data from the robot
                            std::map<std::string, Sensor*> sensors = robot->getSensors();

                            // Get the tool for this cell
                            Tool* tool = cell->getTool();

                            if (tool && tool->type() == ToolType::PLOT) {
                                Plot* plot = dynamic_cast<Plot*>(tool);
                                double simTime = robot->getSimTime();

                                // Add data based on sensor type
                                if (sensorType == "position") {
                                    auto it = sensors.find("pose");
                                    if (it != sensors.end()) {
                                        Sensor* poseSensor = it->second;
                                        Eigen::Vector3d position = dynamic_cast<Pose*>(poseSensor)->getPosition();
                                        plot->addDataPoint("X", position(0), simTime);
                                        plot->addDataPoint("Y", position(1), simTime);
                                        plot->addDataPoint("Z", position(2), simTime);
                                    } else {
                                        qDebug() << "Pose sensor not found!";
                                    }
                                } else if (sensorType == "orientation") {
                                    auto it = sensors.find("pose");
                                    if (it != sensors.end()) {
                                        Sensor* poseSensor = it->second;
                                        Eigen::Vector3d orientation = dynamic_cast<Pose*>(poseSensor)->getEulerOrientation();
                                        plot->addDataPoint("Roll", orientation(0), simTime);
                                        plot->addDataPoint("Pitch", orientation(1), simTime);
                                        plot->addDataPoint("Yaw", orientation(2), simTime);
                                    }
                                } else if (sensorType == "joints_position") {
                                    auto it = sensors.find("joints");
                                    if (it != sensors.end()) {
                                        Sensor* jointsSensor = it->second;
                                        Eigen::Vector3d position = dynamic_cast<Joints*>(jointsSensor)->getPosition();
                                        plot->addDataPoint("head_yaw", position(0), simTime);
                                        plot->addDataPoint("head_pitch", position(1), simTime);
                                        plot->addDataPoint("shoulder_left_pitch", position(2), simTime);
                                        plot->addDataPoint("shoulder_left_roll", position(3), simTime);
                                        plot->addDataPoint("elbow_left_pitch", position(4), simTime);
                                        plot->addDataPoint("elbow_left_yaw", position(5), simTime);
                                        plot->addDataPoint("shoulder_right_pitch", position(6), simTime);
                                        plot->addDataPoint("shoulder_right_roll", position(7), simTime);
                                        plot->addDataPoint("elbow_right_pitch", position(8), simTime);
                                        plot->addDataPoint("elbow_right_yaw", position(9), simTime);
                                        plot->addDataPoint("waist", position(10), simTime);
                                        plot->addDataPoint("hip_left_pitch", position(11), simTime);
                                        plot->addDataPoint("hip_left_roll", position(12), simTime);
                                        plot->addDataPoint("hip_left_yaw", position(13), simTime);
                                        plot->addDataPoint("knee_left_pitch", position(14), simTime);
                                        plot->addDataPoint("ankle_left_pitch", position(15), simTime);
                                        plot->addDataPoint("ankle_left_roll", position(16), simTime);
                                        plot->addDataPoint("hip_right_pitch", position(17), simTime);
                                        plot->addDataPoint("hip_right_roll", position(18), simTime);
                                        plot->addDataPoint("hip_right_yaw", position(19), simTime);
                                        plot->addDataPoint("knee_right_pitch", position(20), simTime);
                                        plot->addDataPoint("ankle_right_pitch", position(21), simTime);
                                        plot->addDataPoint("ankle_right_roll", position(22), simTime);
                                    }
                                } else if (sensorType == "joints_velocity") {
                                    auto it = sensors.find("joints");
                                    if (it != sensors.end()) {
                                        Sensor* jointsSensor = it->second;
                                        Eigen::Vector3d velocity = dynamic_cast<Joints*>(jointsSensor)->getVelocity();
                                        plot->addDataPoint("head_yaw", velocity(0), simTime);
                                        plot->addDataPoint("head_pitch", velocity(1), simTime);
                                        plot->addDataPoint("shoulder_left_pitch", velocity(2), simTime);
                                        plot->addDataPoint("shoulder_left_roll", velocity(3), simTime);
                                        plot->addDataPoint("elbow_left_pitch", velocity(4), simTime);
                                        plot->addDataPoint("elbow_left_yaw", velocity(5), simTime);
                                        plot->addDataPoint("shoulder_right_pitch", velocity(6), simTime);
                                        plot->addDataPoint("shoulder_right_roll", velocity(7), simTime);
                                        plot->addDataPoint("elbow_right_pitch", velocity(8), simTime);
                                        plot->addDataPoint("elbow_right_yaw", velocity(9), simTime);
                                        plot->addDataPoint("waist", velocity(10), simTime);
                                        plot->addDataPoint("hip_left_pitch", velocity(11), simTime);
                                        plot->addDataPoint("hip_left_roll", velocity(12), simTime);
                                        plot->addDataPoint("hip_left_yaw", velocity(13), simTime);
                                        plot->addDataPoint("knee_left_pitch", velocity(14), simTime);
                                        plot->addDataPoint("ankle_left_pitch", velocity(15), simTime);
                                        plot->addDataPoint("ankle_left_roll", velocity(16), simTime);
                                        plot->addDataPoint("hip_right_pitch", velocity(17), simTime);
                                        plot->addDataPoint("hip_right_roll", velocity(18), simTime);
                                        plot->addDataPoint("hip_right_yaw", velocity(19), simTime);
                                        plot->addDataPoint("knee_right_pitch", velocity(20), simTime);
                                        plot->addDataPoint("ankle_right_pitch", velocity(21), simTime);
                                        plot->addDataPoint("ankle_right_roll", velocity(22), simTime);
                                    }
                                } else if (sensorType == "joints_acceleration") {
                                    auto it = sensors.find("joints");
                                    if (it != sensors.end()) {
                                        Sensor* jointsSensor = it->second;
                                        Eigen::Vector3d acceleration = dynamic_cast<Joints*>(jointsSensor)->getAcceleration();
                                        plot->addDataPoint("head_yaw", acceleration(0), simTime);
                                        plot->addDataPoint("head_pitch", acceleration(1), simTime);
                                        plot->addDataPoint("shoulder_left_pitch", acceleration(2), simTime);
                                        plot->addDataPoint("shoulder_left_roll", acceleration(3), simTime);
                                        plot->addDataPoint("elbow_left_pitch", acceleration(4), simTime);
                                        plot->addDataPoint("elbow_left_yaw", acceleration(5), simTime);
                                        plot->addDataPoint("shoulder_right_pitch", acceleration(6), simTime);
                                        plot->addDataPoint("shoulder_right_roll", acceleration(7), simTime);
                                        plot->addDataPoint("elbow_right_pitch", acceleration(8), simTime);
                                        plot->addDataPoint("elbow_right_yaw", acceleration(9), simTime);
                                        plot->addDataPoint("waist", acceleration(10), simTime);
                                        plot->addDataPoint("hip_left_pitch", acceleration(11), simTime);
                                        plot->addDataPoint("hip_left_roll", acceleration(12), simTime);
                                        plot->addDataPoint("hip_left_yaw", acceleration(13), simTime);
                                        plot->addDataPoint("knee_left_pitch", acceleration(14), simTime);
                                        plot->addDataPoint("ankle_left_pitch", acceleration(15), simTime);
                                        plot->addDataPoint("ankle_left_roll", acceleration(16), simTime);
                                        plot->addDataPoint("hip_right_pitch", acceleration(17), simTime);
                                        plot->addDataPoint("hip_right_roll", acceleration(18), simTime);
                                        plot->addDataPoint("hip_right_yaw", acceleration(19), simTime);
                                        plot->addDataPoint("knee_right_pitch", acceleration(20), simTime);
                                        plot->addDataPoint("ankle_right_pitch", acceleration(21), simTime);
                                        plot->addDataPoint("ankle_right_roll", acceleration(22), simTime);
                                    }
                                } else if (sensorType == "joints_torque") {
                                    auto it = sensors.find("joints");
                                    if (it != sensors.end()) {
                                        Sensor* jointsSensor = it->second;
                                        Eigen::Vector3d torque = dynamic_cast<Joints*>(jointsSensor)->getTorque();
                                        plot->addDataPoint("head_yaw", torque(0), simTime);
                                        plot->addDataPoint("head_pitch", torque(1), simTime);
                                        plot->addDataPoint("shoulder_left_pitch", torque(2), simTime);
                                        plot->addDataPoint("shoulder_left_roll", torque(3), simTime);
                                        plot->addDataPoint("elbow_left_pitch", torque(4), simTime);
                                        plot->addDataPoint("elbow_left_yaw", torque(5), simTime);
                                        plot->addDataPoint("shoulder_right_pitch", torque(6), simTime);
                                        plot->addDataPoint("shoulder_right_roll", torque(7), simTime);
                                        plot->addDataPoint("elbow_right_pitch", torque(8), simTime);
                                        plot->addDataPoint("elbow_right_yaw", torque(9), simTime);
                                        plot->addDataPoint("waist", torque(10), simTime);
                                        plot->addDataPoint("hip_left_pitch", torque(11), simTime);
                                        plot->addDataPoint("hip_left_roll", torque(12), simTime);
                                        plot->addDataPoint("hip_left_yaw", torque(13), simTime);
                                        plot->addDataPoint("knee_left_pitch", torque(14), simTime);
                                        plot->addDataPoint("ankle_left_pitch", torque(15), simTime);
                                        plot->addDataPoint("ankle_left_roll", torque(16), simTime);
                                        plot->addDataPoint("hip_right_pitch", torque(17), simTime);
                                        plot->addDataPoint("hip_right_roll", torque(18), simTime);
                                        plot->addDataPoint("hip_right_yaw", torque(19), simTime);
                                        plot->addDataPoint("knee_right_pitch", torque(20), simTime);
                                        plot->addDataPoint("ankle_right_pitch", torque(21), simTime);
                                        plot->addDataPoint("ankle_right_roll", torque(22), simTime);
                                    }
                                } else if (sensorType == "linear_acceleration") {
                                    auto it = sensors.find("imu");
                                    if (it != sensors.end()) {
                                        Sensor* imuSensor = it->second;
                                        Eigen::Vector3d linAcc = dynamic_cast<Imu*>(imuSensor)->getLinearAcceleration();
                                        plot->addDataPoint("Ax", linAcc(0), simTime);
                                        plot->addDataPoint("Ay", linAcc(1), simTime);
                                        plot->addDataPoint("Az", linAcc(2), simTime);
                                    }
                                } else if (sensorType == "angular_velocity") {
                                    auto it = sensors.find("imu");
                                    if (it != sensors.end()) {
                                        Sensor* imuSensor = it->second;
                                        Eigen::Vector3d angVel = dynamic_cast<Imu*>(imuSensor)->getAngularVelocity();
                                        plot->addDataPoint("Wx", angVel(0), simTime);
                                        plot->addDataPoint("Wy", angVel(1), simTime);
                                        plot->addDataPoint("Wz", angVel(2), simTime);
                                    }
                                }

                            }

                            else if (tool && tool->type() == ToolType::IMAGE) {
                                Image* imageTool = dynamic_cast<Image*>(tool);

                                // Add image data based on sensor type
                                if (sensorType == "rgb_left_camera" || sensorType == "rgb_right_camera") {
                                    std::string cameraName = (sensorType == "rgb_left_camera") ? "rgb_left_camera" : "rgb_right_camera";
                                    auto it = sensors.find(cameraName);

                                    if (it != sensors.end()) {
                                        Camera* camera = dynamic_cast<Camera*>(it->second);

                                        if (camera) {
                                            const std::vector<uint8_t>& imageData = camera->getImage();
                                            int width = camera->getWidth();
                                            int height = camera->getHeight();

                                            if (!imageData.empty() && width > 0 && height > 0) {
                                                imageTool->setImage(imageData.data(), width, height, 3);
                                            } else {
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                }

                // Call update on the tool
                cell->updateTool();
            }
        }

    private:
        GridCell* getCellAt(int row, int col) {
            int index = row * prevNumCols_ + col;
            if (index >= 0 && index < cellGrid_.size()) {
                return cellGrid_[index];
            }
            return nullptr;
        }

        void rebuildGrid() {
            // Save the previous grid dimensions
            int oldRows = prevNumRows_;
            int oldCols = prevNumCols_;

            // Build a 2D representation of existing cells for easier access
            // cellGrid_ is maintained as a flat array in row-major order
            QList<GridCell*> oldCellGrid = cellGrid_;

            // Clear the cell grid for rebuilding
            cellGrid_.clear();

            // Build the new cell grid (row-major order)
            for (int row = 0; row < numRows_; row++) {
                for (int col = 0; col < numCols_; col++) {
                    GridCell* cell = nullptr;

                    // Reuse existing cell if it's within the old grid bounds
                    if (row < oldRows && col < oldCols) {
                        int oldIndex = row * oldCols + col;
                        if (oldIndex < oldCellGrid.size()) {
                            cell = oldCellGrid[oldIndex];
                            if (cell && cell->parent()) {
                                cell->setParent(nullptr);
                            }
                        }
                    }

                    // Create new cell if needed
                    if (!cell) {
                        cell = new GridCell(robots_, streams_, nullptr);
                    }

                    cellGrid_.append(cell);
                }
            }

            // Delete cells that are no longer needed (those outside the new grid bounds)
            for (int row = 0; row < oldRows; row++) {
                for (int col = 0; col < oldCols; col++) {
                    // If this cell is outside the new grid bounds, delete it
                    if (row >= numRows_ || col >= numCols_) {
                        int oldIndex = row * oldCols + col;
                        if (oldIndex < oldCellGrid.size() && oldCellGrid[oldIndex]) {
                            delete oldCellGrid[oldIndex];
                        }
                    }
                }
            }

            // Clear existing layout
            if (gridContainer_->layout()) {
                QLayout* oldLayout = gridContainer_->layout();
                QLayoutItem* item;
                while ((item = oldLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        item->widget()->setParent(nullptr);
                    }
                    delete item;
                }
                delete oldLayout;
            }

            // Build the UI layout
            int cellIndex = 0;

            if (numRows_ == 1 && numCols_ == 1) {
                // Simple case: single cell
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);
                layout->addWidget(cellGrid_[cellIndex++]);
            } else if (numRows_ == 1) {
                // Single row: horizontal splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Horizontal, gridContainer_);
                splitter->setHandleWidth(6);
                splitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int col = 0; col < numCols_; col++) {
                    splitter->addWidget(cellGrid_[cellIndex++]);
                }

                layout->addWidget(splitter);
            } else if (numCols_ == 1) {
                // Single column: vertical splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Vertical, gridContainer_);
                splitter->setHandleWidth(6);
                splitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int row = 0; row < numRows_; row++) {
                    splitter->addWidget(cellGrid_[cellIndex++]);
                }

                layout->addWidget(splitter);
            } else {
                // Multiple rows and columns: nested splitters
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* verticalSplitter = new QSplitter(Qt::Vertical, gridContainer_);
                verticalSplitter->setHandleWidth(6);
                verticalSplitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int row = 0; row < numRows_; row++) {
                    QSplitter* horizontalSplitter = new QSplitter(Qt::Horizontal, verticalSplitter);
                    horizontalSplitter->setHandleWidth(6);
                    horizontalSplitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                    for (int col = 0; col < numCols_; col++) {
                        horizontalSplitter->addWidget(cellGrid_[cellIndex++]);
                    }

                    verticalSplitter->addWidget(horizontalSplitter);
                }

                layout->addWidget(verticalSplitter);
            }

            // Update the previous dimensions for next rebuild
            prevNumRows_ = numRows_;
            prevNumCols_ = numCols_;
        }

        QScrollArea* scrollArea_;
        QWidget* gridContainer_;
        int numRows_;
        int numCols_;
        int prevNumRows_ = 1;
        int prevNumCols_ = 1;
        QList<GridCell*> cellGrid_;
        std::vector<std::shared_ptr<Robot>> robots_;
        QMap<QString, ToolType> streams_;
        QTimer* updateTimer_;
};

}  // namespace spqr
