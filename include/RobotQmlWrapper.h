#pragma once

#include <qcontainerfwd.h>
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QVector3D>
#include "robots/Robot.h"
#include "robots/BoosterT1.h"
#include "robots/BoosterK1.h"

namespace spqr {
    class RobotQmlWrapper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QString type READ type CONSTANT)
        Q_PROPERTY(uint8_t number READ number CONSTANT)
        Q_PROPERTY(QVariantMap imu READ imu NOTIFY imuChanged)
        Q_PROPERTY(QVariantMap pose READ pose NOTIFY poseChanged)
    
       public:
        RobotQmlWrapper(const std::shared_ptr<Robot>& robot, QObject* parent = nullptr) 
            : QObject(parent), robot_(robot) {}
    
        QString name() const { return QString::fromStdString(robot_->name); }
        QString type() const { return QString::fromStdString(robot_->type); }
        uint8_t number() const { return robot_->number; }
        
        QVariantMap imu() const {
            QVariantMap imuMap;
            
            if (!robot_) return imuMap;
            
            if (auto t1 = std::dynamic_pointer_cast<BoosterT1>(robot_)) {
                if (t1->imu) {
                    auto linearAcceleration = t1->imu->getLinearAcceleration();
                    auto angularVelocity = t1->imu->getAngularVelocity();
                    imuMap["linearAcceleration"] = QVariantList{linearAcceleration(0), linearAcceleration(1), linearAcceleration(2), linearAcceleration(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            } else if (auto k1 = std::dynamic_pointer_cast<BoosterK1>(robot_)) {
                if (k1->imu) {
                    auto linearAcceleration = k1->imu->getLinearAcceleration();
                    auto angularVelocity = k1->imu->getAngularVelocity();
                    imuMap["linearAcceleration"] = QVariantList{linearAcceleration(0), linearAcceleration(1), linearAcceleration(2), linearAcceleration(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            }
            
            return imuMap;
        }
        
        QVariantMap pose() const {
            QVariantMap poseMap;

            if (!robot_) return poseMap;
            
            if(auto t1 = std::dynamic_pointer_cast<BoosterT1>(robot_)) {
                Eigen::Vector3d position = t1->pose->getPosition();
                Eigen::Vector3d euler_orientation = t1->pose->getEulerOrientation();
                poseMap["position"] = QVariantList{position.x(), position.y(), position.z()};
                poseMap["orientation"] = QVariantList{euler_orientation.x(), euler_orientation.y(), euler_orientation.z()};
            } else if(auto k1 = std::dynamic_pointer_cast<BoosterK1>(robot_)) {
                Eigen::Vector3d position = k1->pose->getPosition();
                Eigen::Vector3d euler_orientation = k1->pose->getEulerOrientation();
                poseMap["position"] = QVariantList{position.x(), position.y(), position.z()};
                poseMap["orientation"] = QVariantList{euler_orientation.x(), euler_orientation.y(), euler_orientation.z()};
            }

            return poseMap;
        }
        
        // Call this to notify QML that values have changed
        void update() {
            emit poseChanged();
            emit imuChanged();
        }
    
       signals:
       void poseChanged();
       void imuChanged();
    
       private:
        std::shared_ptr<Robot> robot_;
    };


}