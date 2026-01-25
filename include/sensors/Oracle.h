#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>

#include "sensors/Sensor.h"

namespace spqr {


// L'oracle is defined as a sensor that provides the ground truth about the world.
// It should contain information about the position of the ball, robots, obstacles, ...
// It can contain also the information their velocities, ...
class Oracle : public Sensor {
    public:
        Oracle(mjModel* mujModel, mjData* mujData, Pose* robotPose) : mujModel(mujModel), mujData(mujData), robotPose(robotPose) {
            
            ballId = mj_name2id(mujModel, mjOBJ_BODY, "ball");
            ballAdr = mujModel->body_jntadr[ballId];

            ballPosAdr = mujModel->jnt_qposadr[ballAdr];
            ballVelAdr = mujModel->jnt_dofadr[ballAdr];
            

            // AGGIUNGERE TUTTI I CAMPI CHE CI INTERESSANO SAPERE COME ORACLE

        }

        void doUpdate() override {
            Eigen::Vector3d ballPosGlobal = Eigen::Vector3d(Eigen::Map<const Eigen::Vector3d>(mujData->qpos + ballPosAdr));
            ballPosition = globalToLocalPosition(ballPosGlobal, robotPose->getTransformationMatrix());
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            std::vector<double> ball_pos = {ballPosition(0), ballPosition(1), ballPosition(2)};
            std::map<std::string, msgpack::object> oracle_data;
            oracle_data["ball_position"] = msgpack::object(ball_pos, z);
            return msgpack::object(oracle_data, z);
        }

        Eigen::Vector3d getBallPosition() const {
            return ballPosition;
        }


    private:
        /**
         * @brief Transform a global position to local position relative to a robot's pose
         * 
         * This function can be used to convert any global world coordinates to local
         * robot-centric coordinates using the transformation matrix.
         * 
         * @param globalPosition The position in global world frame (x, y, z)
         * @param robotTransformationMatrix The robot's 4x4 transformation matrix (world-to-robot)
         * @return The position relative to the robot's local frame
         */
        static Eigen::Vector3d globalToLocalPosition(
            const Eigen::Vector3d& globalPosition,
            const Eigen::Matrix4d& robotTransformationMatrix) {
            
            // The transformation matrix T converts from local to global: p_global = T * p_local
            // So to get p_local: p_local = T^(-1) * p_global
            Eigen::Vector4d globalPosHomogeneous(globalPosition(0), globalPosition(1), globalPosition(2), 1.0);
            Eigen::Vector4d localPosHomogeneous = robotTransformationMatrix.inverse() * globalPosHomogeneous;
            
            return Eigen::Vector3d(localPosHomogeneous(0), localPosHomogeneous(1), localPosHomogeneous(2));
        }


        Eigen::Vector3d ballPosition;
        int ballId, ballAdr, ballPosAdr, ballVelAdr;
        
        mjModel* mujModel;
        mjData* mujData;
        Pose* robotPose;


};
} // namespace spqr
