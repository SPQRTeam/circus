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
#include "sensors/Camera.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

class BoosterK1 : public Robot {
   public:
    std::array<Camera*, 2> cameras = {};
    Imu* imu;
    Joints* joints = nullptr;

    BoosterK1(const std::string& name, const std::string& type, uint8_t number,
              const Eigen::Vector3d& position, const Eigen::Vector3d& orientation,
              const std::shared_ptr<Team>& team)
        : Robot(name, type, number, position, orientation, team),
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
        joints = new Joints(mujCtx->model, mujCtx->data, joint_map);
        imu = new Imu(mujCtx->model, mujCtx->data, (name + "_orientation").c_str(),
                      (name + "_angular-velocity").c_str());
        cameras[0] = new Camera(mujCtx, (name + "_left_cam").c_str());
        cameras[1] = new Camera(mujCtx, (name + "_right_cam").c_str());
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
        return {};
    }

    void update() override {
        /*
        cameras[0]->update();
        cameras[1]->update();
        */
        joints->update();
        imu->update();
    }

    ~BoosterK1() = default;

   private:
    std::map<JointValue, std::string> joint_map;
};

}  // namespace spqr
