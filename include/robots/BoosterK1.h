#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <iostream>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <ostream>
#include <string>

#include "MujocoContext.h"
#include "robots/Robot.h"
#include "sensors/CameraDepth.h"
#include "sensors/CameraRGB.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Pose.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

class Team;  // Forward declaration

class BoosterK1 : public Robot {
    public:
        Pose* pose = nullptr;
        Imu* imu;
        Joints* joints = nullptr;
        CameraRGB* rgbCamera;
        CameraDepth* depthCamera;

        BoosterK1(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& initPosition,
                  const Eigen::Vector3d& initOrientation, const std::string& colorName, const std::shared_ptr<Team>& team)
            : Robot(name, type, number, initPosition, initOrientation, colorName, team),
              joint_map{{JointValue::HEAD_YAW, name + "_AAHead_yaw"},
                        {JointValue::HEAD_PITCH, name + "_Head_pitch"},
                        {JointValue::SHOULDER_LEFT_PITCH, name + "_ALeft_Shoulder_Pitch"},
                        {JointValue::SHOULDER_LEFT_ROLL, name + "_Left_Shoulder_Roll"},
                        {JointValue::ELBOW_LEFT_PITCH, name + "_Left_Elbow_Pitch"},
                        {JointValue::ELBOW_LEFT_YAW, name + "_Left_Elbow_Yaw"},
                        {JointValue::SHOULDER_RIGHT_ROLL, name + "_Right_Shoulder_Roll"},
                        {JointValue::SHOULDER_RIGHT_PITCH, name + "_ARight_Shoulder_Pitch"},
                        {JointValue::ELBOW_RIGHT_PITCH, name + "_Right_Elbow_Pitch"},
                        {JointValue::ELBOW_RIGHT_YAW, name + "_Right_Elbow_Yaw"},
                        {JointValue::HIP_LEFT_PITCH, name + "_Left_Hip_Pitch"},
                        {JointValue::HIP_LEFT_ROLL, name + "_Left_Hip_Roll"},
                        {JointValue::HIP_LEFT_YAW, name + "_Left_Hip_Yaw"},
                        {JointValue::KNEE_LEFT_PITCH, name + "_Left_Knee_Pitch"},
                        {JointValue::ANKLE_LEFT_PITCH, name + "_Left_Ankle_Pitch"},
                        {JointValue::ANKLE_LEFT_ROLL, name + "_Left_Ankle_Roll"},
                        {JointValue::HIP_RIGHT_PITCH, name + "_Right_Hip_Pitch"},
                        {JointValue::HIP_RIGHT_ROLL, name + "_Right_Hip_Roll"},
                        {JointValue::HIP_RIGHT_YAW, name + "_Right_Hip_Yaw"},
                        {JointValue::KNEE_RIGHT_PITCH, name + "_Right_Knee_Pitch"},
                        {JointValue::ANKLE_RIGHT_PITCH, name + "_Right_Ankle_Pitch"},
                        {JointValue::ANKLE_RIGHT_ROLL, name + "_Right_Ankle_Roll"}} {}

        void bindMujoco(MujocoContext* mujCtx) override {
            pose = new Pose(mujCtx->model, mujCtx->data, (name + "_position").c_str(), (name + "_orientation").c_str());
            imu = new Imu(mujCtx->model, mujCtx->data, (name + "_linear-acceleration").c_str(), (name + "_angular-velocity").c_str());
            joints = new Joints(mujCtx->model, mujCtx->data, joint_map);
            rgbCamera = new CameraRGB(mujCtx, (name + "_rgb_cam").c_str());
            depthCamera = new CameraDepth(mujCtx, (name + "_depth_cam").c_str());
        }

        void receiveMessage(const std::map<std::string, msgpack::object>& message) override {
            std::cout << "Hi I'm " << name << " message received: {";
            bool first = true;
            for (const auto& [key, val] : message) {
                if (!first)
                    std::cout << ", ";
                std::cout << key << ": " << val;
                first = false;
            }
            std::cout << "}" << std::endl;
        }

        std::map<std::string, msgpack::object> sendMessage() override {
            buffer_zone_.clear();
            std::map<std::string, msgpack::object> msg;
            msg["robot_name"] = msgpack::object(name, buffer_zone_);
            msg["pose"] = pose->serialize(buffer_zone_);
            msg["imu"] = imu->serialize(buffer_zone_);
            msg["joints"] = joints->serialize(buffer_zone_);

            return msg;
        }

        std::map<std::string, Sensor*> getSensors() override {
            std::map<std::string, Sensor*> sensors;
            sensors["pose"] = pose;
            sensors["imu"] = imu;
            sensors["joints"] = joints;
            sensors["rgb_camera"] = rgbCamera;
            sensors["depth_camera"] = depthCamera;
            return sensors;
        }

        void update() override {
            pose->update();
            imu->update();
            joints->update();
            rgbCamera->update();
            depthCamera->update();
        }

        ~BoosterK1() = default;

    private:
        std::map<JointValue, std::string> joint_map;
};

}  // namespace spqr
