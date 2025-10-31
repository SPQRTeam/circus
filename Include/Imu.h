#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>

#include "Sensor.h"

namespace spqr {

class Imu : public Sensor {
   public:
    Imu(mjModel* mujModel, mjData* mujData, const char* orientationName, const char* gyroName)
        : mujModel(mujModel), mujData(mujData) {
        orientationId = mj_name2id(mujModel, mjOBJ_SENSOR, orientationName);
        orientationAdr = mujModel->sensor_adr[orientationId];
        orientationDim = mujModel->sensor_dim[orientationId];

        gyroId = mj_name2id(mujModel, mjOBJ_SENSOR, gyroName);
        gyroAdr = mujModel->sensor_adr[gyroId];
        gyroDim = mujModel->sensor_dim[gyroId];
    }

    void update() {
        orientation = Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr, orientationDim);
        angularVelocity = Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr, gyroDim);
    };

    msgpack::object serialize(msgpack::zone& z){
        std::vector<double> orientation_vec = {orientation(0), orientation(1), 
                                               orientation(2), orientation(3)};

        std::vector<double> angular_vel_vec = {angularVelocity(0), angularVelocity(1), angularVelocity(2)};

        std::map<std::string, msgpack::object> imu_data;
        imu_data["orientation"] = msgpack::object(orientation_vec, z);
        imu_data["angular_velocity"] = msgpack::object(angular_vel_vec, z);
        return msgpack::object(imu_data, z);
    }

    Eigen::Vector4d orientation;      // [q0, qx, qy, qz] : orientation of the Imu wrt the world frame
    Eigen::Vector3d angularVelocity;  // [wx, wy, wz] : angular velocity expressed in the local frame of the Imu

   private:
    mjModel* mujModel;
    mjData* mujData;

    int orientationId, orientationAdr, orientationDim;
    int gyroId, gyroAdr, gyroDim;
};
}  // namespace spqr
