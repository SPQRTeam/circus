#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include "Robot.h"
#include "Sensor.h"
#include "Camera.h"
#include "Joint.h"
#include "Imu.h"

namespace spqr{

class K1 : spqr::Robot{
public:
    K1(mjModel* mujModel, mjData* mujData) : joint_map{
        {JointValue::HEAD_YAW, "AAHead_yaw"},
        {JointValue::HEAD_PITCH, "Head_pitch"},
        {JointValue::SHOULDER_LEFT_PITCH, "ALeft_Shoulder_Pitch"},
        {JointValue::SHOULDER_LEFT_ROLL, "Left_Shoulder_Roll"},
        {JointValue::ELBOW_LEFT_PITCH, "Left_Elbow_Pitch"},
        {JointValue::ELBOW_LEFT_YAW, "Left_Elbow_Yaw"},
        {JointValue::SHOULDER_RIGHT_PITCH, "ARight_Shoulder_Pitch"},
        {JointValue::SHOULDER_RIGHT_ROLL, "Right_Shoulder_Roll"},
        {JointValue::ELBOW_RIGHT_PITCH, "Right_Elbow_Pitch"},
        {JointValue::ELBOW_RIGHT_YAW, "Right_Elbow_Yaw"},
        {JointValue::HIP_LEFT_PITCH, "Left_Hip_Pitch"},
        {JointValue::HIP_LEFT_ROLL, "Left_Hip_Roll"},
        {JointValue::HIP_LEFT_YAW, "Left_Hip_Yaw"},
        {JointValue::KNEE_LEFT_PITCH, "Left_Knee_Pitch"},
        {JointValue::ANKLE_LEFT_PITCH, "Left_Ankle_Pitch"},
        {JointValue::ANKLE_LEFT_ROLL, "Left_Ankle_Roll"},
        {JointValue::HIP_RIGHT_PITCH, "Right_Hip_Pitch"},
        {JointValue::HIP_RIGHT_ROLL, "Right_Hip_Roll"},
        {JointValue::HIP_RIGHT_YAW, "Right_Hip_Yaw"},
        {JointValue::KNEE_RIGHT_PITCH, "Right_Knee_Pitch"},
        {JointValue::ANKLE_RIGHT_PITCH, "Right_Ankle_Pitch"},
        {JointValue::ANKLE_RIGHT_ROLL, "Right_Ankle_Roll"}
      },
      joints(mujModel, mujData, joint_map),
      imu(mujModel, mujData, "orientation", "angular-velocity")

    {
    }

private:
    std::unordered_map<JointValue, std::string> joint_map;

    std::array<Camera, 2> cameras;
    Imu imu;
    Joints joints;

};

}