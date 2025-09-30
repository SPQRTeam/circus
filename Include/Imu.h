#pragma once

#include <Eigen/Eigen>
#include "Sensor.h"
#include <unordered_map>
#include <string>
#include <mujoco/mujoco.h>

namespace spqr{

class Imu : public Sensor{
public:
    Imu(mjModel* mujModel, mjData* mujData, const char* orientationName, const char* gyroName): mujModel(mujModel), mujData(mujData){
        orientationId = mj_name2id(mujModel, mjOBJ_SENSOR, orientationName);
        orientationAdr = mujModel->sensor_adr[orientationId];
        orientationDim = mujModel->sensor_dim[orientationId];

        gyroId = mj_name2id(mujModel, mjOBJ_SENSOR, gyroName);
        gyroAdr = mujModel->sensor_adr[gyroId];
        gyroDim = mujModel->sensor_dim[gyroId];
    }

    void update(){
        orientation = Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr, orientationDim);
        angularVelocity = Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr, gyroDim);;
    };

    Eigen::Vector4d orientation; // [q0, qx, qy, qz] : orientation of the Imu wrt the world frame
    Eigen::Vector3d angularVelocity; // [wx, wy, wz] : angular velocity expressed in the local frame of the Imu 

private:
    mjModel* mujModel;
    mjData* mujData;

    int orientationId, orientationAdr, orientationDim;
    int gyroId, gyroAdr, gyroDim;
};
}