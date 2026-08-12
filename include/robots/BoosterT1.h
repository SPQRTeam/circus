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
#include "ipc/SharedMemoryReader.h"
#include "ipc/SharedMemoryWriter.h"
#include "robots/Robot.h"
#include "sensors/CameraDepth.h"
#include "sensors/CameraRGB.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Oracle.h"
#include "sensors/Pose.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

class Team;  // Forward declaration

constexpr size_t kBoosterT1JointCount = 23;

// Per-tick shared-memory payload: everything BoosterT1's packMessage() sends
// over the socket (minus robot_name, which the shared-memory file path already
// encodes), bundled into one trivially-copyable struct so a reader sees all
// fields from the same tick atomically (one SharedMemoryWriter -> one seq).
struct BoosterT1SharedState {
        PoseData pose;
        ImuData imu;
        JointState<kBoosterT1JointCount> joints;
        OracleData oracle;
};

// Identifies this exact state layout. Shared memory carries no schema, so a
// reader that opened another robot type's segment would otherwise just
// reinterpret the bytes; checking this first turns that into a clean failure.
constexpr uint32_t kBoosterT1SchemaId = 0xB0057E71;

// Describes the camera frames that ride along in the trailing region of the
// state slot, in the order sendMessageSHM() writes them. Per-robot by
// design: another robot type declares its own <Robot>SharedState /
// <Robot>StateMeta pair with however many streams it actually has (the
// mechanism is generic; only the payload is robot-specific). If a robot ever
// needs a stream count that varies at runtime, bound it the way OracleData
// bounds teammates: a fixed-capacity array plus a count.
struct BoosterT1StateMeta {
        uint32_t schema_id = kBoosterT1SchemaId;
        uint32_t state_bytes = static_cast<uint32_t>(sizeof(BoosterT1SharedState));
        ImageMeta rgb;
        ImageMeta depth;
};

class BoosterT1 : public Robot {
    public:
        Pose* pose = nullptr;
        Imu* imu = nullptr;
        Joints* joints = nullptr;
        Oracle* oracle = nullptr;
        CameraRGB* rgbCamera;
        CameraDepth* depthCamera;

        BoosterT1(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& initPosition,
                  const Eigen::Vector3d& initOrientation, const std::string& colorName, const std::shared_ptr<Team>& team)
            : Robot(name, type, number, initPosition, initOrientation, colorName, team),
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
            // Where to put the images
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

            rgbCamera = new CameraRGB(mujCtx, (name + "_rgb_cam").c_str());
            // Use RGB viewpoint for simulated depth to provide aligned depth-to-color.
            // This avoids parallax between rgb_cam and depth_cam when unprojecting RGB detections.
            depthCamera = new CameraDepth(mujCtx, (name + "_rgb_cam").c_str());

            // State and both camera frames share a single segment. The frames are
            // runtime-sized (resolution comes from the MuJoCo model) but fixed once
            // known, so they fit the writer's trailing region, with their shape
            // carried in the meta blob for the reader.
            const uint32_t width = static_cast<uint32_t>(rgbCamera->getWidth());
            const uint32_t height = static_cast<uint32_t>(rgbCamera->getHeight());
            const size_t rgbBytes = static_cast<size_t>(width) * height * 3;
            const size_t depthBytes = static_cast<size_t>(width) * height * 1;
            state_writer_.configure(shmFilePath_("state"), rgbBytes + depthBytes,
                                    BoosterT1StateMeta{kBoosterT1SchemaId, static_cast<uint32_t>(sizeof(BoosterT1SharedState)),
                                                       ImageMeta{width, height, 3}, ImageMeta{width, height, 1}});
            command_reader_.configure(shmFilePath_("commands"));

            // Create Oracle with the pose and all robots
            oracle = new Oracle(mujCtx->model, mujCtx->data, name, pose);
        }

        void receiveMessageSocket(const std::map<std::string, msgpack::object>& message) override {
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

            size_t i = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto& [joint_value, joint_name] : joint_map) {
                    latestTorques[joint_value] = joint_torques[i++];
                }
            }
        }

        bool receiveMessageSHM() override {
            JointTorques<kBoosterT1JointCount> torques;
            if (!command_reader_.readLatest(torques)) {
                return false;
            }

            std::lock_guard<std::mutex> lock(mutex_);
            size_t i = 0;
            for (const auto& [joint_value, joint_name] : joint_map) {
                latestTorques[joint_value] = torques.torque[i++];
            }
            return true;
        }

        std::map<std::string, msgpack::object> packMessage() override {
            buffer_zone_.clear();
            std::map<std::string, msgpack::object> msg;
            msg["robot_name"] = msgpack::object(name, buffer_zone_);
            msg["pose"] = pose->serialize(buffer_zone_);
            msg["imu"] = imu->serialize(buffer_zone_);
            msg["joints"] = joints->serialize(buffer_zone_);
            msg["oracle"] = oracle->serialize(buffer_zone_);

            return msg;
        }

        void sendMessageSHM() override {
            // One slot, one seq: pose/imu/joints/oracle and both frames are published
            // together, so a reader can never pair a frame with a pose from a
            // different tick -- which matters as soon as anything unprojects an image
            // detection using the pose at capture time.
            const auto& rgbFrame = rgbCamera->getImage();
            const auto& depthFrame = depthCamera->getDepth8bit();
            state_writer_.write(BoosterT1SharedState{pose->toSharedState(), imu->toSharedState(),
                                                     joints->toSharedState<kBoosterT1JointCount>(), oracle->toSharedState()},
                                {{rgbFrame.data(), rgbFrame.size()}, {depthFrame.data(), depthFrame.size()}});
        }

        std::map<std::string, Sensor*> getSensors() override {
            std::map<std::string, Sensor*> sensors;
            sensors["pose"] = pose;
            sensors["imu"] = imu;
            sensors["joints"] = joints;
            sensors["rgb_camera"] = rgbCamera;
            sensors["depth_camera"] = depthCamera;
            return sensors;
        }

        void applyCommands() override {
            std::lock_guard<std::mutex> lock(mutex_);
            joints->set_torque(latestTorques);
        }

        void update() override {
            pose->update();
            imu->update();
            joints->update();
            oracle->update();
            rgbCamera->update();
            depthCamera->update();
        }

        ~BoosterT1() = default;

    private:
        std::string shmFilePath_(const std::string& camera) const {
            return shm_dir_ + "/" + name + "_" + camera + ".shm";
        }

        std::map<JointValue, std::string> joint_map;
        std::unordered_map<JointValue, mjtNum> latestTorques;

        std::string shm_dir_;
        SharedMemoryWriter<BoosterT1SharedState, BoosterT1StateMeta> state_writer_;
        SharedMemoryReader<JointTorques<kBoosterT1JointCount>> command_reader_;
};

}  // namespace spqr
