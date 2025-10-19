#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <random>
#include <iostream>
#include <string>

#include <fstream>

#include "Sensor.h"

namespace spqr {

class DataCapture {
    public:
    std::ofstream logFile;

    DataCapture(const std::string& filename) {
        logFile.open(filename);
        logFile << "q0,qx,qy,qz,"
                << "wx,wy,wz\n";
    }

    void update(Eigen::Vector4d orientation, Eigen::Vector3d angularVelocity) {
        logFile << orientation[0] << "," << orientation[1] << "," 
                << orientation[2] << "," << orientation[3] << ","
                << angularVelocity[0] << "," << angularVelocity[1] << ","
                << angularVelocity[2] << "\n";
    }

    ~DataCapture() {
        logFile.close();
    }
};

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

    Imu(mjModel* mujModel, mjData* mujData, const char* orientationName, const char* gyroName, const uint8_t robotNumber)
        : mujModel(mujModel), mujData(mujData) {
        orientationId = mj_name2id(mujModel, mjOBJ_SENSOR, orientationName);
        orientationAdr = mujModel->sensor_adr[orientationId];
        orientationDim = mujModel->sensor_dim[orientationId];

        gyroId = mj_name2id(mujModel, mjOBJ_SENSOR, gyroName);
        gyroAdr = mujModel->sensor_adr[gyroId];
        gyroDim = mujModel->sensor_dim[gyroId];

        capture = new DataCapture(std::to_string(robotNumber));
    }

    void update() {
        orientation = Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr, orientationDim);
        angularVelocity = Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr, gyroDim);
        
        if(capture) {
            capture->update(orientation, angularVelocity);
        }
    };

    Eigen::Vector4d orientation;      // [q0, qx, qy, qz] : orientation of the Imu wrt the world frame
    Eigen::Vector3d angularVelocity;  // [wx, wy, wz] : angular velocity expressed in the local frame of the
                                      // Imu

   private:
    mjModel* mujModel;
    mjData* mujData;

    int orientationId, orientationAdr, orientationDim;
    int gyroId, gyroAdr, gyroDim;

    DataCapture* capture;
};
}  // namespace spqr
