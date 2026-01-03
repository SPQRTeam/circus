#pragma once

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector3D>

#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"
#include "sensors/Sensor.h"

namespace spqr {
class RobotQmlWrapper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QString type READ type CONSTANT)
        Q_PROPERTY(uint8_t number READ number CONSTANT)
        Q_PROPERTY(QVariantMap imu READ imu NOTIFY imuChanged)
        Q_PROPERTY(QVariantMap pose READ pose NOTIFY poseChanged) 
        Q_PROPERTY(QVariantMap image READ image NOTIFY imageChanged)
        

    public:
        RobotQmlWrapper(const std::shared_ptr<Robot>& robot, QObject* parent = nullptr) : QObject(parent), robot_(robot) {}

        QString name() const {
            return QString::fromStdString(robot_->name);
        }
        QString type() const {
            return QString::fromStdString(robot_->type);
        }
        uint8_t number() const {
            return robot_->number;
        }

        QVariantMap imu() const {
            QVariantMap imuMap;

            if (!robot_)
                return imuMap;

            if (auto t1 = std::dynamic_pointer_cast<BoosterT1>(robot_)) {
                if (t1->imu) {
                    auto linearAcceleration = t1->imu->getLinearAcceleration();
                    auto angularVelocity = t1->imu->getAngularVelocity();
                    imuMap["linearAcceleration"]
                        = QVariantList{linearAcceleration(0), linearAcceleration(1), linearAcceleration(2), linearAcceleration(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            } else if (auto k1 = std::dynamic_pointer_cast<BoosterK1>(robot_)) {
                if (k1->imu) {
                    auto linearAcceleration = k1->imu->getLinearAcceleration();
                    auto angularVelocity = k1->imu->getAngularVelocity();
                    imuMap["linearAcceleration"]
                        = QVariantList{linearAcceleration(0), linearAcceleration(1), linearAcceleration(2), linearAcceleration(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            }

            return imuMap;
        }

        QVariantMap pose() const {
            QVariantMap poseMap;

            if (!robot_)
                return poseMap;

            if (auto t1 = std::dynamic_pointer_cast<BoosterT1>(robot_)) {
                Eigen::Vector3d position = t1->pose->getPosition();
                Eigen::Vector3d euler_orientation = t1->pose->getEulerOrientation();
                poseMap["position"] = QVariantList{position.x(), position.y(), position.z()};
                poseMap["orientation"] = QVariantList{euler_orientation.x(), euler_orientation.y(), euler_orientation.z()};
            } else if (auto k1 = std::dynamic_pointer_cast<BoosterK1>(robot_)) {
                Eigen::Vector3d position = k1->pose->getPosition();
                Eigen::Vector3d euler_orientation = k1->pose->getEulerOrientation();
                poseMap["position"] = QVariantList{position.x(), position.y(), position.z()};
                poseMap["orientation"] = QVariantList{euler_orientation.x(), euler_orientation.y(), euler_orientation.z()};
            }

            return poseMap;
        }

        QVariantMap image() const {
            QVariantMap imageMap;

            if (!robot_)
                return imageMap;

            std::map<std::string, Sensor*> sensors = robot_->getSensors();
            // left_camera, right_camera
            Camera* leftCamera = dynamic_cast<Camera*>(sensors["left_camera"]);
            Camera* rightCamera = dynamic_cast<Camera*>(sensors["right_camera"]);

            if (leftCamera) {
                QVariantList leftImage;
                const std::vector<uint8_t>& imgData = leftCamera->getImage();
                for (uint8_t byte : imgData) {
                    leftImage.append(byte);
                }
                imageMap["left_camera"] = leftImage;
            }

            if (rightCamera) {
                QVariantList rightImage;
                const std::vector<uint8_t>& imgData = rightCamera->getImage();
                for (uint8_t byte : imgData) {
                    rightImage.append(byte);
                }
                imageMap["right_camera"] = rightImage;
            }

            return imageMap;
        }

        // Call this to notify QML that values have changed
        void update() {
            emit poseChanged();
            emit imuChanged();
            emit imageChanged();
        }

    signals:
        void poseChanged();
        void imuChanged();
        void imageChanged();

    private:
        std::shared_ptr<Robot> robot_;
};

}  // namespace spqr
