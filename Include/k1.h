#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include "Robot.h"
#include "Sensor.h"
#include "Camera.h"
#include "Joint.h"

namespace spqr{

class K1 : spqr::Robot{
public:
    K1(mjModel* mujModel, mjData* mujData) : joint_map{
        {ANKLE_PITCH_LEFT, name + "_culoculo"}
      },
      joints(mujModel, mujData, joint_map)

    {
    }

private:
    std::unordered_map<JointValue, std::string> joint_map;

    std::array<Camera, 2> cameras;
    Joints joints;   

};

}