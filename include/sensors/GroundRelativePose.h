#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <cmath>

#include "sensors/Pose.h"
#include "sensors/Sensor.h"

namespace spqr {

// Pose of a body relative to the ground projection of a reference Pose.
// Reference frame: (trunk_x, trunk_y, 0) with yaw-only rotation of the trunk.
class GroundRelativePose : public Sensor {
   public:
    GroundRelativePose(mjModel* mujModel, mjData* mujData, const char* bodyName, Pose* referencePose)
        : mujData_(mujData), referencePose_(referencePose) {
        bodyId_ = mj_name2id(mujModel, mjOBJ_BODY, bodyName);
    }

    void doUpdate() override {
        Eigen::Vector3d bodyPos(mujData_->xpos[3 * bodyId_], mujData_->xpos[3 * bodyId_ + 1], mujData_->xpos[3 * bodyId_ + 2]);
        Eigen::Quaterniond bodyQuat(mujData_->xquat[4 * bodyId_], mujData_->xquat[4 * bodyId_ + 1],
                                    mujData_->xquat[4 * bodyId_ + 2], mujData_->xquat[4 * bodyId_ + 3]);

        Eigen::Vector4d refQuatVec = referencePose_->getQuatOrientation();
        Eigen::Quaterniond refQuat(refQuatVec(0), refQuatVec(1), refQuatVec(2), refQuatVec(3));
        Eigen::Vector3d refPos = referencePose_->getPosition();

        // Extract only yaw from trunk orientation
        double w = refQuat.w(), x = refQuat.x(), y = refQuat.y(), z = refQuat.z();
        double yaw = std::atan2(2.0 * (w * z + x * y), 1.0 - 2.0 * (y * y + z * z));
        Eigen::Quaterniond groundQuat(Eigen::AngleAxisd(yaw, Eigen::Vector3d::UnitZ()));

        // Reference origin is trunk XY projected on ground (z=0)
        Eigen::Vector3d groundPos(refPos.x(), refPos.y(), 0.0);

        Eigen::Quaterniond relQuat = groundQuat.conjugate() * bodyQuat;
        position_ = groundQuat.conjugate() * (bodyPos - groundPos);
        quatOrientation_ = Eigen::Vector4d(relQuat.w(), relQuat.x(), relQuat.y(), relQuat.z());

        Eigen::Vector3d euler = relQuat.toRotationMatrix().eulerAngles(2, 1, 0);
        eulerOrientation_ = Eigen::Vector3d(euler(2), euler(1), euler(0));
        rotationMatrix_ = relQuat.toRotationMatrix();

        transformationMatrix_ = Eigen::Matrix4d::Identity();
        transformationMatrix_.block<3, 3>(0, 0) = rotationMatrix_;
        transformationMatrix_.block<3, 1>(0, 3) = position_;
    }

    msgpack::object doSerialize(msgpack::zone& z) override {
        std::vector<double> position_vec = {position_(0), position_(1), position_(2)};
        std::vector<double> quat_orientation_vec = {quatOrientation_(0), quatOrientation_(1), quatOrientation_(2), quatOrientation_(3)};
        std::vector<double> euler_orientation_vec = {eulerOrientation_(0), eulerOrientation_(1), eulerOrientation_(2)};
        std::vector<double> rot_mat_vec = {
            rotationMatrix_(0, 0), rotationMatrix_(0, 1), rotationMatrix_(0, 2),
            rotationMatrix_(1, 0), rotationMatrix_(1, 1), rotationMatrix_(1, 2),
            rotationMatrix_(2, 0), rotationMatrix_(2, 1), rotationMatrix_(2, 2)};
        std::vector<double> transformation_matrix_vec = {
            transformationMatrix_(0, 0), transformationMatrix_(0, 1), transformationMatrix_(0, 2), transformationMatrix_(0, 3),
            transformationMatrix_(1, 0), transformationMatrix_(1, 1), transformationMatrix_(1, 2), transformationMatrix_(1, 3),
            transformationMatrix_(2, 0), transformationMatrix_(2, 1), transformationMatrix_(2, 2), transformationMatrix_(2, 3),
            transformationMatrix_(3, 0), transformationMatrix_(3, 1), transformationMatrix_(3, 2), transformationMatrix_(3, 3)};

        std::map<std::string, msgpack::object> pose_data;
        pose_data["position"] = msgpack::object(position_vec, z);
        pose_data["quat_orientation"] = msgpack::object(quat_orientation_vec, z);
        pose_data["euler_orientation"] = msgpack::object(euler_orientation_vec, z);
        pose_data["rotation_matrix"] = msgpack::object(rot_mat_vec, z);
        pose_data["transformation_matrix"] = msgpack::object(transformation_matrix_vec, z);
        return msgpack::object(pose_data, z);
    }

    Eigen::Vector3d getPosition() const { return position_; }
    Eigen::Vector4d getQuatOrientation() const { return quatOrientation_; }
    Eigen::Vector3d getEulerOrientation() const { return eulerOrientation_; }
    Eigen::Matrix3d getRotationMatrix() const { return rotationMatrix_; }
    Eigen::Matrix4d getTransformationMatrix() const { return transformationMatrix_; }

   private:
    mjData* mujData_;
    int bodyId_;
    Pose* referencePose_;

    Eigen::Vector3d position_;
    Eigen::Vector4d quatOrientation_;
    Eigen::Vector3d eulerOrientation_;
    Eigen::Matrix3d rotationMatrix_;
    Eigen::Matrix4d transformationMatrix_;
};

}  // namespace spqr
