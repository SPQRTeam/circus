#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <random>
#include <string>

#include "Logger.h"
#include "NoiseGenerator.h"
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

        gyroNoiseStd = mujModel->sensor_noise[gyroId];

        std::string csvHeader = "q0,qx,qy,qz,wx,wy,wz";
        capture = new Logger(std::to_string(robotNumber), csvHeader);
        captureNoise = new Logger(std::to_string(robotNumber) + "_noise", csvHeader);
        captureNoise2 = new Logger(std::to_string(robotNumber) + "_noise2", csvHeader);

        double dt_ = 0.002; // TODO: get from config when SimulationThread changes

        gyroNoiseStd = 0.03 / std::sqrt(dt_);
        gyroBiasWalkStd = 5 * std::sqrt(dt_);
        
        noiseGen = NoiseGenerator(gyroNoiseStd, gyroBiasWalkStd);
    }

    //double theta;
    void update() {
        orientation = Eigen::Map<const Eigen::Vector4d>(mujData->sensordata + orientationAdr, orientationDim);
        if (first) {
            orientationQuat = Eigen::Quaterniond(orientation[0], orientation[1], orientation[2], orientation[3]);
            orientationQuatNoisy = Eigen::Quaterniond(orientation[0], orientation[1], orientation[2], orientation[3]);
            first = false;
        } else {
            integrateOrientationRK4(orientationQuat, angularVelocity, 0.01);
            integrateOrientationRK4(orientationQuatNoisy, angularVelocityNoisy, 0.01);
        }
        angularVelocity = Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + gyroAdr, gyroDim);
        noiseGen.addNoise(angularVelocity, angularVelocityNoisy);
        
        if(capture) {
            capture->log(orientation[0], orientation[1], orientation[2], orientation[3], 
                    angularVelocity[0], angularVelocity[1], angularVelocity[2]);

            captureNoise->log(orientationQuat.w(), orientationQuat.x(), orientationQuat.y(), orientationQuat.z(), 
                    angularVelocityNoisy[0], angularVelocityNoisy[1], angularVelocityNoisy[2]);

            captureNoise2->log(orientationQuatNoisy.w(), orientationQuatNoisy.x(), orientationQuatNoisy.y(), orientationQuatNoisy.z(),
                    "", "", "");
        }
    };

    Eigen::Vector4d orientation;      // [q0, qx, qy, qz] : orientation of the Imu wrt the world frame
    bool first = true;
    Eigen::Quaterniond orientationQuat;
    Eigen::Quaterniond orientationQuatNoisy;
    Eigen::Vector3d angularVelocity;  // [wx, wy, wz] : angular velocity expressed in the local frame of the
    Eigen::Vector3d angularVelocityNoisy;

   private:
    mjModel* mujModel;
    mjData* mujData;

    int orientationId, orientationAdr, orientationDim;
    int gyroId, gyroAdr, gyroDim;

    double orientationNoiseStd;

    Logger* capture;
    Logger* captureNoise;
    Logger* captureNoise2;

    double gyroNoiseStd;       // rad/s discrete
    double gyroBiasWalkStd;    // rad/s discrete (random walk step)

    NoiseGenerator noiseGen;

    Eigen::Quaterniond quatDerivative(const Eigen::Quaterniond& q, const Eigen::Vector3d& omega) {
        Eigen::Quaterniond omegaQuat(0.0, omega.x(), omega.y(), omega.z());
        Eigen::Quaterniond dq = q * omegaQuat;
        dq.coeffs() *= 0.5;
        return dq;
    }

    Eigen::Quaterniond quatAddScaled(const Eigen::Quaterniond& q, const Eigen::Quaterniond& k, double a) {
        Eigen::Quaterniond result = q;
        result.coeffs() += a * k.coeffs();
        return result;
    }

    void integrateOrientationRK4(
        Eigen::Quaterniond& q,
        const Eigen::Vector3d& omega,
        double dt)
    {
        Eigen::Quaterniond k1 = quatDerivative(q, omega);
        Eigen::Quaterniond k2 = quatDerivative(quatAddScaled(q, k1, 0.5 * dt), omega);
        Eigen::Quaterniond k3 = quatDerivative(quatAddScaled(q, k2, 0.5 * dt), omega);
        Eigen::Quaterniond k4 = quatDerivative(quatAddScaled(q, k3, dt), omega);

        q.coeffs() += (dt / 6.0) * (k1.coeffs() + 2.0 * k2.coeffs() + 2.0 * k3.coeffs() + k4.coeffs());
        q.normalize();
    }

    inline void integrateAngle(Eigen::Quaterniond& orientation,
        const Eigen::Vector3d& angularVelocity,
        double dt)
    {
        Eigen::Quaterniond w(0., angularVelocity[0], angularVelocity[1], angularVelocity[2]);

        Eigen::Quaterniond qdot = orientation * w;
        qdot.coeffs() *= 0.5 * dt;

        orientation.coeffs() += qdot.coeffs();
        orientation.normalize();
    }
};
}  // namespace spqr
