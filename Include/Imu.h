#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <random>
#include <string>

#include "Logger.h"
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

    Imu(mjModel* mujModel, mjData* mujData, const char* orientationName, const char* gyroName, const uint8_t robotNumber)
        : mujModel(mujModel), mujData(mujData) {
        orientationId = mj_name2id(mujModel, mjOBJ_SENSOR, orientationName);
        orientationAdr = mujModel->sensor_adr[orientationId];
        orientationDim = mujModel->sensor_dim[orientationId];

        gyroId = mj_name2id(mujModel, mjOBJ_SENSOR, gyroName);
        gyroAdr = mujModel->sensor_adr[gyroId];
        gyroDim = mujModel->sensor_dim[gyroId];

        orientationNoiseStd = mujModel->sensor_noise[orientationId];
        gyroNoiseStd = mujModel->sensor_noise[gyroId];

        std::string csvHeader = "q0,qx,qy,qz,wx,wy,wz";
        capture = new Logger(std::to_string(robotNumber), csvHeader);
        captureNoise = new Logger(std::to_string(robotNumber) + "_noise", csvHeader);
    }

    void update() {
        orientation = Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr, orientationDim);
        angularVelocity = Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr, gyroDim);
        
        if(capture) {
            capture->log(orientation[0], orientation[1], orientation[2], orientation[3], 
                    angularVelocity[0], angularVelocity[1], angularVelocity[2]);

            addNoise(orientation, angularVelocity);
            captureNoise->log(orientation[0], orientation[1], orientation[2], orientation[3], 
                    angularVelocity[0], angularVelocity[1], angularVelocity[2]);
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

    double orientationNoiseStd, gyroNoiseStd;

    Logger* capture;
    Logger* captureNoise;

    void addNoise(Eigen::Vector4d& orientation, Eigen::Vector3d& angularVelocity,
            double gyroscopeRandomWalk = 1, double gyroscopeNoiseDensity = 5) {
        static std::default_random_engine generator(std::random_device{}());
        std::normal_distribution<double> gyroscopeBiasDist(0., gyroNoiseStd);
        std::normal_distribution<double> gyroscopeNoiseDist(0., gyroNoiseStd);
        for (int i = 0; i < 4; ++i)
            angularVelocity(i) += gyroscopeRandomWalk * gyroscopeBiasDist(generator)
                                + gyroscopeNoiseDensity * gyroscopeNoiseDist(generator);
    }
};
}  // namespace spqr
