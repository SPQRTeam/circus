#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <cstdlib>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "MujocoContext.h"
#include "robots/Robot.h"
#include "sensors/Camera.h"
#include "sensors/ImageSharedMemoryWriter.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Pose.h"
#include "sensors/Oracle.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

class BoosterT1 : public Robot {
    public:
        Pose* pose = nullptr;
        Imu* imu = nullptr;
        Joints* joints = nullptr;
        std::array<Camera*, 2> cameras = {};
        Oracle* oracle = nullptr;

        BoosterT1(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& initPosition,
                  const Eigen::Vector3d& initOrientation, const std::tuple<int, int, int> color, const std::shared_ptr<Team>& team)
            : Robot(name, type, number, initPosition, initOrientation, color, team),
              joint_map{{JointValue::HEAD_YAW, name + "_AAHead_yaw"},
                        {JointValue::HEAD_PITCH, name + "_Head_pitch"},
                        {JointValue::SHOULDER_LEFT_PITCH, name + "_Left_Shoulder_Pitch"},
                        {JointValue::SHOULDER_LEFT_ROLL, name + "_Left_Shoulder_Roll"},
                        {JointValue::ELBOW_LEFT_PITCH, name + "_Left_Elbow_Pitch"},
                        {JointValue::ELBOW_LEFT_YAW, name + "_Left_Elbow_Yaw"},
                        {JointValue::SHOULDER_RIGHT_PITCH, name + "_Right_Shoulder_Pitch"},
                        {JointValue::SHOULDER_RIGHT_ROLL, name + "_Right_Shoulder_Roll"},
                        {JointValue::ELBOW_RIGHT_PITCH, name + "_Right_Elbow_Pitch"},
                        {JointValue::ELBOW_RIGHT_YAW, name + "_Right_Elbow_Yaw"},
                        {JointValue::WAIST, name + "_Waist"},
                        {JointValue::HIP_LEFT_PITCH, name + "_Left_Hip_Pitch"},
                        {JointValue::HIP_LEFT_ROLL, name + "_Left_Hip_Roll"},
                        {JointValue::HIP_LEFT_YAW, name + "_Left_Hip_Yaw"},
                        {JointValue::KNEE_LEFT_PITCH, name + "_Left_Knee_Pitch"},
                        {JointValue::ANKLE_LEFT_PITCH, name + "_Left_Ankle_Pitch"},
                        {JointValue::ANKLE_LEFT_ROLL, name + "_Left_Ankle_Roll"},
                        {JointValue::HIP_RIGHT_PITCH, name + "_Right_Hip_Pitch"},
                        {JointValue::HIP_RIGHT_ROLL, name + "_Right_Hip_Roll"},
                        {JointValue::HIP_RIGHT_YAW, name + "_Right_Hip_Yaw"},
                        {JointValue::KNEE_RIGHT_PITCH, name + "_Right_Knee_Pitch"},
                        {JointValue::ANKLE_RIGHT_PITCH, name + "_Right_Ankle_Pitch"},
                        {JointValue::ANKLE_RIGHT_ROLL, name + "_Right_Ankle_Roll"}} {
            //Where to put the images
            shm_dir_ = "/dev/shm/circus_ipc";
        }

        void bindMujoco(MujocoContext* mujCtx) override {
            pose = new Pose(mujCtx->model, mujCtx->data, (name + "_position").c_str(), (name + "_orientation").c_str());
            imu = new Imu(mujCtx->model, mujCtx->data, (name + "_linear-acceleration").c_str(), (name + "_angular-velocity").c_str());
            joints = new Joints(mujCtx->model, mujCtx->data, joint_map);

            joints->set_position({{JointValue::HEAD_YAW, 0},
                                  {JointValue::HEAD_PITCH, 0},
                                  {JointValue::SHOULDER_LEFT_PITCH, 0},
                                  {JointValue::SHOULDER_LEFT_ROLL, 0},
                                  {JointValue::ELBOW_LEFT_PITCH, 0},
                                  {JointValue::ELBOW_LEFT_YAW, 0},
                                  {JointValue::SHOULDER_RIGHT_PITCH, 0},
                                  {JointValue::SHOULDER_RIGHT_ROLL, 0},
                                  {JointValue::ELBOW_RIGHT_PITCH, 0},
                                  {JointValue::ELBOW_RIGHT_YAW, 0},
                                  {JointValue::WAIST, 0},
                                  {JointValue::HIP_LEFT_PITCH, 0},
                                  {JointValue::HIP_LEFT_ROLL, 0},
                                  {JointValue::HIP_LEFT_YAW, 0},
                                  {JointValue::KNEE_LEFT_PITCH, 0},
                                  {JointValue::ANKLE_LEFT_PITCH, 0},
                                  {JointValue::ANKLE_LEFT_ROLL, 0},
                                  {JointValue::HIP_RIGHT_PITCH, 0},
                                  {JointValue::HIP_RIGHT_ROLL, 0},
                                  {JointValue::HIP_RIGHT_YAW, 0},
                                  {JointValue::KNEE_RIGHT_PITCH, 0},
                                  {JointValue::ANKLE_RIGHT_PITCH, 0},
                                  {JointValue::ANKLE_RIGHT_ROLL, 0}});

            cameras[0] = new Camera(mujCtx, (name + "_left_cam").c_str());
            cameras[1] = new Camera(mujCtx, (name + "_right_cam").c_str());

            //Configure the writer for the shared memory file
            const int width = cameras[0]->getWidth();
            const int height = cameras[0]->getHeight();
            left_writer_.configure(shmFilePath_("left"), width, height, 3);
            right_writer_.configure(shmFilePath_("right"), width, height, 3);
            

            // Create Oracle with the pose and all robots
            oracle = new Oracle(mujCtx->model, mujCtx->data, name, pose);
        }

        void receiveMessage(const std::map<std::string, msgpack::object>& message) override {
            auto it = message.find("joint_torques");
            if (it == message.end()) {
                throw std::runtime_error("Error: 'joint_torques' key not found in message");
                return;
            }

            std::vector<double> joint_torques = it->second.as<std::vector<double>>();

            if (joint_torques.size() != joint_map.size()) {
                throw std::runtime_error("Error: joint_torques size (" + std::to_string(joint_torques.size()) + ") doesn't match number of joints ("
                                         + std::to_string(joint_map.size()) + ")");
            }

            std::unordered_map<JointValue, mjtNum> torque_map;
            size_t i = 0;
            for (const auto& [joint_value, joint_name] : joint_map) {
                torque_map[joint_value] = joint_torques[i++];
            }

            joints->set_torque(torque_map);
        }

        std::map<std::string, msgpack::object> sendMessage() override {
            buffer_zone_.clear();
            std::map<std::string, msgpack::object> msg;
            msg["robot_name"] = msgpack::object(name, buffer_zone_);
            msg["pose"] = pose->serialize(buffer_zone_);
            msg["imu"] = imu->serialize(buffer_zone_);
            msg["joints"] = joints->serialize(buffer_zone_);
            msg["oracle"] = oracle->serialize(buffer_zone_);

            // Write in the shared file the information 
            left_writer_.write(cameras[0]->getImage());
            right_writer_.write(cameras[1]->getImage());
            

            return msg;
        }

        std::map<std::string, Sensor*> getSensors() override {
            std::map<std::string, Sensor*> sensors;
            sensors["pose"] = pose;
            sensors["imu"] = imu;
            sensors["joints"] = joints;
            sensors["rgb_left_camera"] = cameras[0];
            sensors["rgb_right_camera"] = cameras[1];
            return sensors;
        }

        void update() override {
            pose->update();
            imu->update();
            joints->update();
            cameras[0]->update();
            cameras[1]->update();
            oracle->update();
        }

        ~BoosterT1() = default;

    private:
        std::string shmFilePath_(const std::string& camera) const {
            return shm_dir_ + "/" + name + "_" + camera + ".shm";
        }

        std::map<JointValue, std::string> joint_map;
        std::string shm_dir_;
        ImageSharedMemoryWriter left_writer_;
        ImageSharedMemoryWriter right_writer_;
};

}  // namespace spqr
