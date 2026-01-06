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
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "tools/Tool.h"
#include "tools/Plot.h"
#include "robots/Robot.h"
#include "sensors/Sensor.h"
#include "sensors/Pose.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Camera.h"

namespace spqr {

class GridCell : public QWidget {
        Q_OBJECT

    public:
        GridCell(QMap<QString, ToolType> streams, QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                            "  background-color: #2a2a2a; "
                            "  border: 1px solid #444444; "
                            "  border-radius: 3px;"
                            "}"
                            "QWidget:hover { "
                            "  background-color: #3a3a3a; "
                            "  border: 2px solid #1e667e; "
                            "}");
            setMinimumSize(100, 100);

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
                                     "  border: 1px solid #1e667e; "
                                     "  padding: 0; "
                                     "  margin: 0; "
                                     "  outline: 0; "
                                     "} "
                                     "QListView::item { "
                                     "  padding: 6px 10px; "
                                     "  margin: 0; "
                                     "} "
                                     "QListView::item:selected { "
                                     "  background-color: #1e667e; "
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
                                "  border: 1px solid #1e667e; "
                                "} "
                            );

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

        QString selectedItem() const { return selectedItem_; }

        Tool* getTool() const { return tool_; }

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

            // Only change tool if type is different
            if (!tool_ || tool_->type() != newToolType) {
                Tool* newTool = nullptr;

                switch (newToolType) {
                    case ToolType::PLOT:
                        newTool = new Plot(this);
                        
                        if(selectedItem_.contains("/position")) {
                            Plot* plot = dynamic_cast<Plot*>(newTool);
                            plot->addTimeSeries("X", QColor(255, 0, 0));
                            plot->addTimeSeries("Y", QColor(0, 255, 0));
                            plot->addTimeSeries("Z", QColor(0, 0, 255));
                        }
                        else if(selectedItem_.contains("/orientation")) {
                            Plot* plot = dynamic_cast<Plot*>(newTool);
                            plot->addTimeSeries("Roll", QColor(255, 0, 0));
                            plot->addTimeSeries("Pitch", QColor(0, 255, 0));
                            plot->addTimeSeries("Yaw", QColor(0, 0, 255));
                        }
                        else if(selectedItem_.contains("/linear_acceleration")) {
                            Plot* plot = dynamic_cast<Plot*>(newTool);
                            plot->addTimeSeries("Ax", QColor(255, 0, 0));
                            plot->addTimeSeries("Ay", QColor(0, 255, 0));
                            plot->addTimeSeries("Az", QColor(0, 0, 255));
                        }
                        else if(selectedItem_.contains("/angular_velocity")) {
                            Plot* plot = dynamic_cast<Plot*>(newTool);
                            plot->addTimeSeries("Wx", QColor(255, 0, 0));
                            plot->addTimeSeries("Wy", QColor(0, 255, 0));
                            plot->addTimeSeries("Wz", QColor(0, 0, 255));
                        }

                        break;

                    case ToolType::NONE:
                        newTool = new Tool(ToolType::NONE, this);
                        break;
                    
                    default:
                        newTool = new Tool(ToolType::NONE, this);
                        break;
                }

                setTool(newTool);
            }
        }

        QString selectedItem_;
        Tool* tool_;
        QVBoxLayout* cellLayout_;
        QMap<QString, ToolType> streams_;
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

            // Create the grid container
            gridContainer_ = new QWidget(this);
            mainLayout->addWidget(gridContainer_);

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

        int getNumRows() const { return numRows_; }
        int getNumCols() const { return numCols_; }

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
                qDebug() << "Updating cell with stream:" << selectedStream;
                // Parse the stream name (format: "robotName/sensorType")
                QStringList parts = selectedStream.split('/');
                if (parts.size() == 2) {
                    QString robotName = parts[0];
                    QString sensorType = parts[1];
                    qDebug() << "Robot:" << robotName << ", Sensor:" << sensorType;
                    
                    // Find the matching robot
                    for (std::shared_ptr<Robot>& robot : robots_) {
                        if (QString::fromStdString(robot->name) == robotName) {
                            // Get sensor data from the robot
                            std::map<std::string, Sensor*> sensors = robot->getSensors();
                            
                            // Get the tool for this cell
                            Tool* tool = cell->getTool(); // You'll need to add a getter
                            if (tool && tool->type() == ToolType::PLOT) {
                                Plot* plot = dynamic_cast<Plot*>(tool);
                                
                                // Add data based on sensor type
                                if (sensorType == "position") {
                                    qDebug() << "Adding position data to plot";
                                    auto it = sensors.find("pose");
                                    if (it != sensors.end()) {
                                        qDebug() << "Found pose sensor";
                                        Sensor* poseSensor = it->second;
                                        Eigen::Vector3d position = dynamic_cast<Pose*>(poseSensor)->getPosition();
                                        qDebug() << "Position:" << position(0) << position(1) << position(2);
                                        plot->addDataPoint("X", position(0));
                                        plot->addDataPoint("Y", position(1));
                                        plot->addDataPoint("Z", position(2));    
                                    }
                                    else {
                                        qDebug() << "Pose sensor not found!";
                                    }

                                }
                                else if (sensorType == "orientation") {
                                    qDebug() << "Adding orientation data to plot";
                                    auto it = sensors.find("pose");
                                    if (it != sensors.end()) {
                                        qDebug() << "Found pose sensor";
                                        Sensor* poseSensor = it->second;
                                        Eigen::Vector3d orientation = dynamic_cast<Pose*>(poseSensor)->getEulerOrientation();
                                        qDebug() << "Orientation:" << orientation(0) << orientation(1) << orientation(2);
                                        plot->addDataPoint("Roll", orientation(0));
                                        plot->addDataPoint("Pitch", orientation(1));
                                        plot->addDataPoint("Yaw", orientation(2));    
                                    }
                                }
                                else if (sensorType == "linear_acceleration") {
                                    qDebug() << "Adding linear acceleration data to plot";
                                    auto it = sensors.find("imu");
                                    if (it != sensors.end()) {
                                        qDebug() << "Found imu sensor";
                                        Sensor* imuSensor = it->second;
                                        Eigen::Vector3d linAcc = dynamic_cast<Imu*>(imuSensor)->getLinearAcceleration();
                                        qDebug() << "Linear Acceleration:" << linAcc(0) << linAcc(1) << linAcc(2);
                                        plot->addDataPoint("Ax", linAcc(0));
                                        plot->addDataPoint("Ay", linAcc(1));
                                        plot->addDataPoint("Az", linAcc(2));    
                                    }
                                }
                                else if (sensorType == "angular_velocity") {
                                    qDebug() << "Adding angular velocity data to plot";
                                    auto it = sensors.find("imu");
                                    if (it != sensors.end()) {
                                        qDebug() << "Found imu sensor";
                                        Sensor* imuSensor = it->second;
                                        qDebug() << "Angular Velocity:";
                                        Eigen::Vector3d angVel = dynamic_cast<Imu*>(imuSensor)->getAngularVelocity();
                                        qDebug() << angVel(0) << angVel(1) << angVel(2);
                                        plot->addDataPoint("Wx", angVel(0));
                                        plot->addDataPoint("Wy", angVel(1));
                                        plot->addDataPoint("Wz", angVel(2));    
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
        void rebuildGrid() {
            // Clear existing layout
            if (gridContainer_->layout()) {
                QLayout* oldLayout = gridContainer_->layout();
                QLayoutItem* item;
                while ((item = oldLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        delete item->widget();
                    }
                    delete item;
                }
                delete oldLayout;
            }

            // Build nested splitters for resizable grid
            if (numRows_ == 1 && numCols_ == 1) {
                // Simple case: single cell
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);
                GridCell* cell = new GridCell(streams_, gridContainer_);
                layout->addWidget(cell);
            } else if (numRows_ == 1) {
                // Single row: horizontal splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Horizontal, gridContainer_);
                splitter->setHandleWidth(6);
                splitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int col = 0; col < numCols_; col++) {
                    GridCell* cell = new GridCell(streams_, splitter);
                    splitter->addWidget(cell);
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
                    GridCell* cell = new GridCell(streams_, splitter);
                    splitter->addWidget(cell);
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
                        GridCell* cell = new GridCell(streams_, horizontalSplitter);
                        horizontalSplitter->addWidget(cell);
                    }

                    verticalSplitter->addWidget(horizontalSplitter);
                }

                layout->addWidget(verticalSplitter);
            }
        }

        QWidget* gridContainer_;
        int numRows_;
        int numCols_;
        std::vector<std::shared_ptr<Robot>> robots_;
        QMap<QString, ToolType> streams_;
        QTimer* updateTimer_;
};

}  // namespace spqr
