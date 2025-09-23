#pragma once

#include "Sensor.h"
#include <unordered_map>
#include <string>
#include <mujoco/mjmodel.h>
#include <mujoco/mjtnum.h>
#include <mujoco/mujoco.h>

namespace spqr{

enum JointValue {
    ANKLE_PITCH_LEFT,
};


class Joints : Sensor{
public:
    Joints(mjModel* mujModel, mjData* mujData, std::unordered_map<JointValue, std::string> map): mujModel(mujModel), mujData(mujData){
        for(auto& [jv, joint_name] : map){
            int jointId = mj_name2id(mujModel, mjOBJ_JOINT, joint_name.c_str());
            joint_ids[jv] = jointId;
        }
    }

    void update(){
        for (const auto& [joint, joint_id] : joint_ids){
            position[joint] = mujData->qpos[mujModel->jnt_qposadr[joint_id]]; // TODO: assuming size = 1
            velocity[joint] = mujData->qvel[mujModel->jnt_dofadr[joint_id]]; 
            acceleration[joint] = mujData->qacc[mujModel->jnt_dofadr[joint_id]];   
            torque[joint] = mujData->ctrl[joint_id]; 
        }
    };

    void set_position(std::unordered_map<JointValue, mjtNum> values){
        // Setto dentro mujoco facendo check aggiuntivi
    }

    void set_torque(std::unordered_map<JointValue, mjtNum> values){
        // Setto dentro mujoco facendo check aggiuntivi
    }

private:
    mjModel* mujModel;
    mjData* mujData;

    std::unordered_map<JointValue, int> joint_ids;

    std::unordered_map<JointValue, mjtNum> position;
    std::unordered_map<JointValue, mjtNum> velocity;
    std::unordered_map<JointValue, mjtNum> acceleration;
    std::unordered_map<JointValue, mjtNum> torque;
};
}