#pragma once

#include <mujoco/mujoco.h>

#include <string>
#include <iostream>
#include <unordered_map>

#include "Sensor.h"

namespace spqr {

enum class JointValue {
    HEAD_YAW,
    HEAD_PITCH,
    SHOULDER_LEFT_PITCH,
    SHOULDER_LEFT_ROLL,
    ELBOW_LEFT_PITCH,
    ELBOW_LEFT_YAW,
    SHOULDER_RIGHT_PITCH,
    SHOULDER_RIGHT_ROLL,
    ELBOW_RIGHT_PITCH,
    ELBOW_RIGHT_YAW,
    WAIST,
    HIP_LEFT_PITCH,
    HIP_LEFT_ROLL,
    HIP_LEFT_YAW,
    KNEE_LEFT_PITCH,
    ANKLE_LEFT_PITCH,
    ANKLE_LEFT_ROLL,
    HIP_RIGHT_PITCH,
    HIP_RIGHT_ROLL,
    HIP_RIGHT_YAW,
    KNEE_RIGHT_PITCH,
    ANKLE_RIGHT_PITCH,
    ANKLE_RIGHT_ROLL,
};

class Joints : public Sensor {
   public:
    Joints(mjModel* mujModel, mjData* mujData, std::unordered_map<JointValue, std::string> map)
        : mujModel(mujModel), mujData(mujData) {
        for (auto& [jv, joint_name] : map) {
            int jointId = mj_name2id(mujModel, mjOBJ_JOINT, joint_name.c_str());
            if(jointId == -1)
                throw std::runtime_error("Joint not found: " + joint_name);

            joint_ids[jv] = jointId;

            for (int act_id = 0; act_id < mujModel->nu; act_id++) {
                if (mujModel->actuator_trntype[act_id] == mjTRN_JOINT
                    && mujModel->actuator_trnid[2 * act_id] == jointId) {
                    actuator_ids[jv] = act_id;
                }
            }
        }
    }

    void update() {
        for (const auto& [joint, joint_id] : joint_ids) {
            position[joint] = mujData->qpos[mujModel->jnt_qposadr[joint_id]];  // TODO: assuming size = 1
            velocity[joint] = mujData->qvel[mujModel->jnt_dofadr[joint_id]];
            acceleration[joint] = mujData->qacc[mujModel->jnt_dofadr[joint_id]];
            torque[joint] = mujData->qfrc_actuator[mujModel->jnt_dofadr[joint_id]];
        }
    };

    void set_position(const std::unordered_map<JointValue, mjtNum>& values) {
        for (const auto& [joint, val] : values) {
            auto it = joint_ids.find(joint);
            if (it == joint_ids.end())
                continue;
            int joint_id = it->second;
            int adr = mujModel->jnt_qposadr[joint_id];
            mujData->qpos[adr] = val;  // TODO: assuming size=1
        }
        mj_forward(mujModel, mujData);
    }

    void set_torque(const std::unordered_map<JointValue, mjtNum>& values) {
        for (const auto& [joint, val] : values) {
            auto it = actuator_ids.find(joint);
            //for (const auto& [j, act_id] : actuator_ids) {
            //    std::cout << "[ "<< static_cast<int>(joint) << " ]"<< static_cast<int>(j) << " -> " << act_id << "\n";
            //}
            if (it == actuator_ids.end())
                continue;
            int act_id = it->second;
            mujData->ctrl[act_id] = val;
        }
    }

   private:
    mjModel* mujModel;
    mjData* mujData;

    std::unordered_map<JointValue, int> joint_ids, actuator_ids;

    std::unordered_map<JointValue, mjtNum> position, velocity, acceleration, torque;
};
}  // namespace spqr
