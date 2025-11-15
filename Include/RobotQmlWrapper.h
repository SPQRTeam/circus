#pragma once

#include <qcontainerfwd.h>
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QVector3D>
#include "Robot.h"

namespace spqr {
    class RobotQmlWrapper : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(QString type READ type CONSTANT)
        Q_PROPERTY(uint8_t number READ number CONSTANT)
        Q_PROPERTY(QVariantMap imu READ imu NOTIFY imuChanged)
        Q_PROPERTY(QVariantList position READ position NOTIFY positionChanged)
        Q_PROPERTY(QVariantList orientation READ orientation NOTIFY orientationChanged)
    
       public:
        RobotQmlWrapper(const std::shared_ptr<Robot>& robot, QObject* parent = nullptr) 
            : QObject(parent), robot_(robot) {}
    
        QString name() const { return QString::fromStdString(robot_->name); }
        QString type() const { return QString::fromStdString(robot_->type); }
        uint8_t number() const { return robot_->number; }
        
        QVariantMap imu() const {
            QVariantMap imuMap;
            
            if (!robot_) return imuMap;
            
            if (auto t1 = std::dynamic_pointer_cast<T1>(robot_)) {
                if (t1->imu) {
                    auto orientation = t1->imu->getOrientation();
                    auto angularVelocity = t1->imu->getAngularVelocity();
                    imuMap["orientation"] = QVariantList{orientation(0), orientation(1), orientation(2), orientation(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            } else if (auto k1 = std::dynamic_pointer_cast<K1>(robot_)) {
                if (k1->imu) {
                    auto orientation = k1->imu->getOrientation();
                    auto angularVelocity = k1->imu->getAngularVelocity();
                    imuMap["orientation"] = QVariantList{orientation(0), orientation(1), orientation(2), orientation(3)};
                    imuMap["angularVelocity"] = QVariantList{angularVelocity(0), angularVelocity(1), angularVelocity(2)};
                }
            }
            
            return imuMap;
        }
        
        QVariantList position() const {
            if (!robot_) return QVariantList{0.0, 0.0, 0.0};
            return QVariantList{robot_->position.x(), robot_->position.y(), robot_->position.z()};
        }
        QVariantList orientation() const {
            if (!robot_) return QVariantList{0.0, 0.0, 0.0};
            return QVariantList{robot_->orientation.x(), robot_->orientation.y(), robot_->orientation.z()};
        }
        
        // Call this to notify QML that values have changed
        void update() {
            emit positionChanged();
            emit orientationChanged();
            emit imuChanged();
        }
    
       signals:
        void imuChanged();
        void positionChanged();
        void orientationChanged();
    
       private:
        std::shared_ptr<Robot> robot_;
    };


}