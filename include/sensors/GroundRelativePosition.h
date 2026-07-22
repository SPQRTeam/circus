#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <cmath>

#include "sensors/Pose.h"
#include "sensors/Sensor.h"

namespace spqr {

// Head/camera pose in the robot ground frame:
// origin = (trunk_x, trunk_y, 0), axes = trunk yaw only (no roll/pitch).
// Standing: z ≈ head height (~1.1 m). Fallen: z low, orientation shows ~90° pitch.
class GroundRelativePosition : public Sensor {
    public:
        enum class TargetType { Body, Site };

        GroundRelativePosition(mjModel* mujModel, mjData* mujData, const char* name, TargetType type, Pose* referencePose)
            : mujData_(mujData), type_(type), referencePose_(referencePose) {
            if (type_ == TargetType::Site)
                targetId_ = mj_name2id(mujModel, mjOBJ_SITE, name);
            else
                targetId_ = mj_name2id(mujModel, mjOBJ_BODY, name);
        }

        void doUpdate() override {
            Eigen::Vector3d targetPos;
            Eigen::Matrix3d targetRot = Eigen::Matrix3d::Identity();
            if (type_ == TargetType::Site) {
                targetPos = Eigen::Vector3d(mujData_->site_xpos[3 * targetId_], mujData_->site_xpos[3 * targetId_ + 1],
                                            mujData_->site_xpos[3 * targetId_ + 2]);
                const mjtNum* m = mujData_->site_xmat + 9 * targetId_;
                targetRot << m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8];
            } else {
                targetPos = Eigen::Vector3d(mujData_->xpos[3 * targetId_], mujData_->xpos[3 * targetId_ + 1],
                                            mujData_->xpos[3 * targetId_ + 2]);
                const mjtNum* m = mujData_->xmat + 9 * targetId_;
                targetRot << m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8];
            }

            Eigen::Vector4d refQuatVec = referencePose_->getQuatOrientation();
            Eigen::Quaterniond refQuat(refQuatVec(0), refQuatVec(1), refQuatVec(2), refQuatVec(3));
            Eigen::Vector3d refPos = referencePose_->getPosition();

            double w = refQuat.w(), x = refQuat.x(), y = refQuat.y(), z = refQuat.z();
            double yaw = std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z));
            Eigen::Quaterniond groundQuat(Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()));
            Eigen::Vector3d groundPos(refPos.x(), refPos.y(), 0.0);

            position_ = groundQuat.conjugate() * (targetPos - groundPos);
            Eigen::Quaterniond targetQuat(targetRot);
            targetQuat.normalize();
            orientation_ = groundQuat.conjugate() * targetQuat;
            orientation_.normalize();
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::vector<double> position_vec = {position_(0), position_(1), position_(2)};
            std::vector<double> quat_vec = {orientation_.w(), orientation_.x(), orientation_.y(), orientation_.z()};
            std::map<std::string, msgpack::object> data;
            data["position"] = msgpack::object(position_vec, z);
            data["quat_orientation"] = msgpack::object(quat_vec, z);
            return msgpack::object(data, z);
        }

        Eigen::Vector3d getPosition() const {
            return position_;
        }

        Eigen::Quaterniond getOrientation() const {
            return orientation_;
        }

    private:
        mjData* mujData_;
        int targetId_;
        TargetType type_;
        Pose* referencePose_;

        Eigen::Vector3d position_;
        Eigen::Quaterniond orientation_{Eigen::Quaterniond::Identity()};
};

}  // namespace spqr
