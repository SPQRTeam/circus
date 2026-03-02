#pragma once

#include <mujoco/mujoco.h>

#include <Eigen/Eigen>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "sensors/Sensor.h"
namespace spqr {

// Forward declarations
class Pose;

// L'oracle is defined as a sensor that provides the ground truth about the world.
// It should contain information about the position of the ball, robots, obstacles, ...
// It can contain also the information their velocities, ...
class Oracle : public Sensor {
    public:
        Oracle(mjModel* mujModel, mjData* mujData, std::string robotName, Pose* robotPose) 
            : mujModel(mujModel), mujData(mujData), robotName(robotName), robotPose(robotPose) {
            
            size_t pos = robotName.find('_');
            teamName = robotName.substr(0, pos);

            ballId = mj_name2id(mujModel, mjOBJ_BODY, "ball");
            ballAdr = mujModel->body_jntadr[ballId];

            ballPosAdr = mujModel->jnt_qposadr[ballAdr];
            ballVelAdr = mujModel->jnt_dofadr[ballAdr];
            

            // AGGIUNGERE TUTTI I CAMPI CHE CI INTERESSANO SAPERE COME ORACLE
            discoverOtherRobotSensors();
        }

        void doUpdate() override {
            // Update local ball position
            updateBallPosition();

            // Update all other robots' local positions
            updateTeammatesLocalPositions();
            updateOpponentsLocalPositions();
        }

        msgpack::object doSerialize(msgpack::zone& z) override {
            // Serialize ball position
            std::vector<double> ball_pos = {ballPosition(0), ballPosition(1), ballPosition(2)};
            std::map<std::string, msgpack::object> oracle_data;
            oracle_data["ball_position"] = msgpack::object(ball_pos, z);
            
            // Serialize teammates' positions
            std::map<std::string, msgpack::object> teammates_data;
            for (const auto& [robotName, robotLocalPos] : teammatesLocalPositions) {
                std::vector<double> robot_pos = {robotLocalPos(0), robotLocalPos(1), robotLocalPos(2)};                
                teammates_data[robotName] = msgpack::object(robot_pos, z);
            }

            // Serialize opponents' positions
            std::map<std::string, msgpack::object> opponents_data;
            for (const auto& [robotName, robotLocalPos] : opponentsLocalPositions) {
                std::vector<double> robot_pos = {robotLocalPos(0), robotLocalPos(1), robotLocalPos(2)};                
                opponents_data[robotName] = msgpack::object(robot_pos, z);
            }

            oracle_data["teammates_positions"] = msgpack::object(teammates_data, z);
            oracle_data["opponents_positions"] = msgpack::object(opponents_data, z);
            
            return msgpack::object(oracle_data, z);
        }

        Eigen::Vector3d getBallPosition() const {
            return ballPosition;
        }

        /**
         * @brief Get the local position of another robot
         * 
         * @param robotName The name of the robot
         * @return The position of the robot in the local frame, or nullopt if not found
         */
        std::optional<Eigen::Vector3d> getTeammatePosition(const std::string& robotName) const {
            auto it = teammatesLocalPositions.find(robotName);
            if (it != teammatesLocalPositions.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        /**
         * @brief Get all other robots' positions in local frame
         * 
         * @return Map of robot names to their local positions
         */
        const std::map<std::string, Eigen::Vector3d>& getAllTeammatesLocalPositions() const {
            return teammatesLocalPositions;
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

        void discoverOtherRobotSensors() {
            // Scan all sensors in MuJoCo to find position sensors of other robots
            for (int i = 0; i < mujModel->nsensor; i++) {
                const char* sensorName = mj_id2name(mujModel, mjOBJ_SENSOR, i);
                if (!sensorName) continue;
                
                std::string sName(sensorName);
                
                // Look for position sensors (assuming naming convention: {robotName}_position)
                size_t pos = sName.rfind("_position");
                if (pos != std::string::npos) {
                    std::string extractedRobotName = sName.substr(0, pos);
                    
                    // Skip if it's the current robot
                    if (extractedRobotName == robotName) {
                        continue;
                    }
                    
                    // Store the sensor address for this robot
                    int positionAdr = mujModel->sensor_adr[i];
                    
                    size_t pos = extractedRobotName.find('_');
                    std::string extractedRobotTeamName = extractedRobotName.substr(0, pos);
                    
                    if(extractedRobotTeamName == teamName) {
                        teammatesSensorAddrs[extractedRobotName] = positionAdr;
                    }
                    else {
                        opponentsSensorAddrs[extractedRobotName] = positionAdr;
                    } 
                }
            }
        }

        void updateBallPosition() {
            Eigen::Vector3d ballPosGlobal = Eigen::Vector3d(Eigen::Map<const Eigen::Vector3d>(mujData->qpos + ballPosAdr));
            ballPosition = globalToLocalPosition(ballPosGlobal, robotPose->getTransformationMatrix());
        }

        /**
         * @brief Update positions of all other robots in local frame
         * 
         * Iterates through all robots in the world (except the current one) and converts
         * their global positions to local coordinates relative to this robot's pose.
         */
        void updateTeammatesLocalPositions() {
            teammatesLocalPositions.clear();
            
            // Get positions of other robots using their sensor data
            for (const auto& [teammateName, sensorAdr] : teammatesSensorAddrs) {
                // Read position from sensor data (3D vector)
                Eigen::Vector3d robotGlobalPos = Eigen::Vector3d(
                    Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + sensorAdr)
                );
                
                // Convert to local frame
                Eigen::Vector3d robotLocalPos = globalToLocalPosition(
                    robotGlobalPos,
                    robotPose->getTransformationMatrix()
                );
                
                teammatesLocalPositions[teammateName] = robotLocalPos;
                
            }
        }

        void updateOpponentsLocalPositions() {
            opponentsLocalPositions.clear();
            
            // Get positions of other robots using their sensor data
            for (const auto& [opponentName, sensorAdr] : opponentsSensorAddrs) {
                // Read position from sensor data (3D vector)
                Eigen::Vector3d robotGlobalPos = Eigen::Vector3d(
                    Eigen::Map<const Eigen::Vector3d>(mujData->sensordata + sensorAdr)
                );
                
                // Convert to local frame
                Eigen::Vector3d robotLocalPos = globalToLocalPosition(
                    robotGlobalPos,
                    robotPose->getTransformationMatrix()
                );
                
                opponentsLocalPositions[opponentName] = robotLocalPos;
                
            }
        }

        Eigen::Vector3d ballPosition;
        int ballId, ballAdr, ballPosAdr, ballVelAdr;
        
        mjModel* mujModel;
        mjData* mujData;
        std::string robotName;
        std::string teamName;
        Pose* robotPose;
        std::map<std::string, Eigen::Vector3d> teammatesLocalPositions;
        std::map<std::string, Eigen::Vector3d> opponentsLocalPositions;

        std::map<std::string, int> teammatesSensorAddrs;  // robotName -> sensorAdr
        std::map<std::string, int> opponentsSensorAddrs;  // robotName -> sensorAdr


};
} // namespace spqr
