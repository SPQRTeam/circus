#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <cstdlib>
#include <cstring>
#include <iostream>
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
#include "sensors/CameraInfo.h"
#include "sensors/CameraRGB.h"
#include "sensors/GroundRelativePosition.h"
#include "sensors/Imu.h"
#include "sensors/Joint.h"
#include "sensors/Oracle.h"
#include "sensors/Pose.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

class Team;  // Forward declaration

// 2 head + 2x7 arms + 3 waist + 2x6 legs, in the order of the upstream T2_31dof model
// (see the JointValue declaration order in sensors/Joint.h).
constexpr size_t kBoosterT2JointCount = 31;

// Per-tick shared-memory payload: everything BoosterT2's packMessage() sends
// over the socket (minus robot_name, which the shared-memory file path already
// encodes), bundled into one trivially-copyable struct so a reader sees all
// fields from the same tick atomically (one SharedMemoryWriter -> one seq).
struct BoosterT2SharedState {
        PoseData pose;
        ImuData imu;
        JointState<kBoosterT2JointCount> joints;
        OracleData oracle;
        GroundRelativePositionData headPose;
        CameraInfoData cameraInfo;
};

// Identifies this exact state layout. Shared memory carries no schema, so a
// reader that opened another robot type's segment would otherwise just
// reinterpret the bytes; checking this first turns that into a clean failure.
// Must differ from every other robot type's id (Booster-T1 uses 0xB0057E71/0xB0057E72).
constexpr uint32_t kBoosterT2SchemaId = 0xB0057E73;

struct BoosterT2StateMeta {
        uint32_t schema_id = kBoosterT2SchemaId;
        uint32_t state_bytes = static_cast<uint32_t>(sizeof(BoosterT2SharedState));
};

// Identifies the image segment's layout, separately from the state segment's
// kBoosterT2SchemaId above -- state and camera frames now publish on their own
// segments (state every physics substep, images once per control step; see
// SimulationThread::run()), so each needs its own schema/meta pair.
constexpr uint32_t kBoosterT2ImageSchemaId = 0xB0057E74;

// Describes the camera frames that ride in the image segment, in the order
// sendMessageSHM() writes them. Per-robot by design: another robot type
// declares its own <Robot>ImageMeta with however many streams it actually has
// (the mechanism is generic; only the payload is robot-specific). If a robot
// ever needs a stream count that varies at runtime, bound it the way
// OracleData bounds teammates: a fixed-capacity array plus a count.
struct BoosterT2ImageMeta {
        uint32_t schema_id = kBoosterT2ImageSchemaId;
        ImageMeta rgb;
        ImageMeta depth;
};

class BoosterT2 : public Robot {
    public:
        Pose* pose = nullptr;
        GroundRelativePosition* headPose = nullptr;
        Imu* imu = nullptr;
        Joints<kBoosterT2JointCount>* joints = nullptr;
        Oracle* oracle = nullptr;
        CameraRGB* rgbCamera;
        CameraDepth* depthCamera;
        CameraInfo* rgbCameraInfo = nullptr;

        BoosterT2(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& initPosition,
                  const Eigen::Vector3d& initOrientation, const std::string& colorName, const std::shared_ptr<Team>& team)
            : Robot(name, type, number, initPosition, initOrientation, colorName, team),
              joint_map{{JointValue::HEAD_YAW, name + "_aa_head_yaw_joint"},
                        {JointValue::HEAD_PITCH, name + "_head_pitch_joint"},
                        {JointValue::SHOULDER_LEFT_PITCH, name + "_left_shoulder_pitch_joint"},
                        {JointValue::SHOULDER_LEFT_ROLL, name + "_left_shoulder_roll_joint"},
                        {JointValue::ELBOW_LEFT_PITCH, name + "_left_elbow_pitch_joint"},
                        {JointValue::ELBOW_LEFT_YAW, name + "_left_elbow_yaw_joint"},
                        {JointValue::WRIST_LEFT_PITCH, name + "_left_wrist_pitch_joint"},
                        {JointValue::WRIST_LEFT_YAW, name + "_left_wrist_yaw_joint"},
                        {JointValue::WRIST_LEFT_ROLL, name + "_left_wrist_roll_joint"},
                        {JointValue::SHOULDER_RIGHT_PITCH, name + "_right_shoulder_pitch_joint"},
                        {JointValue::SHOULDER_RIGHT_ROLL, name + "_right_shoulder_roll_joint"},
                        {JointValue::ELBOW_RIGHT_PITCH, name + "_right_elbow_pitch_joint"},
                        {JointValue::ELBOW_RIGHT_YAW, name + "_right_elbow_yaw_joint"},
                        {JointValue::WRIST_RIGHT_PITCH, name + "_right_wrist_pitch_joint"},
                        {JointValue::WRIST_RIGHT_YAW, name + "_right_wrist_yaw_joint"},
                        {JointValue::WRIST_RIGHT_ROLL, name + "_right_wrist_roll_joint"},
                        {JointValue::WAIST_PITCH, name + "_waist_pitch_joint"},
                        {JointValue::WAIST_ROLL, name + "_waist_roll_joint"},
                        {JointValue::WAIST_YAW, name + "_waist_yaw_joint"},
                        {JointValue::HIP_LEFT_PITCH, name + "_left_hip_pitch_joint"},
                        {JointValue::HIP_LEFT_ROLL, name + "_left_hip_roll_joint"},
                        {JointValue::HIP_LEFT_YAW, name + "_left_hip_yaw_joint"},
                        {JointValue::KNEE_LEFT_PITCH, name + "_left_knee_pitch_joint"},
                        {JointValue::ANKLE_LEFT_PITCH, name + "_left_ankle_pitch_joint"},
                        {JointValue::ANKLE_LEFT_ROLL, name + "_left_ankle_roll_joint"},
                        {JointValue::HIP_RIGHT_PITCH, name + "_right_hip_pitch_joint"},
                        {JointValue::HIP_RIGHT_ROLL, name + "_right_hip_roll_joint"},
                        {JointValue::HIP_RIGHT_YAW, name + "_right_hip_yaw_joint"},
                        {JointValue::KNEE_RIGHT_PITCH, name + "_right_knee_pitch_joint"},
                        {JointValue::ANKLE_RIGHT_PITCH, name + "_right_ankle_pitch_joint"},
                        {JointValue::ANKLE_RIGHT_ROLL, name + "_right_ankle_roll_joint"}} {}

        void bindMujoco(MujocoContext* mujCtx, std::string connectMode_) override {
            pose = new Pose(mujCtx->model, mujCtx->data, (name + "_position").c_str(), (name + "_orientation").c_str());
            headPose = new GroundRelativePosition(mujCtx->model, mujCtx->data, (name + "_head_rgb_cam_site").c_str(),
                                                  GroundRelativePosition::TargetType::Site, pose);
            imu = new Imu(mujCtx->model, mujCtx->data, (name + "_linear-acceleration").c_str(), (name + "_angular-velocity").c_str());
            joints = new Joints<kBoosterT2JointCount>(mujCtx->model, mujCtx->data, joint_map);

            joints->set_position({{JointValue::HEAD_YAW, 0},
                                  {JointValue::HEAD_PITCH, 0},
                                  {JointValue::SHOULDER_LEFT_PITCH, 0},
                                  {JointValue::SHOULDER_LEFT_ROLL, 0},
                                  {JointValue::ELBOW_LEFT_PITCH, 0},
                                  {JointValue::ELBOW_LEFT_YAW, 0},
                                  {JointValue::WRIST_LEFT_PITCH, 0},
                                  {JointValue::WRIST_LEFT_YAW, 0},
                                  {JointValue::WRIST_LEFT_ROLL, 0},
                                  {JointValue::SHOULDER_RIGHT_PITCH, 0},
                                  {JointValue::SHOULDER_RIGHT_ROLL, 0},
                                  {JointValue::ELBOW_RIGHT_PITCH, 0},
                                  {JointValue::ELBOW_RIGHT_YAW, 0},
                                  {JointValue::WRIST_RIGHT_PITCH, 0},
                                  {JointValue::WRIST_RIGHT_YAW, 0},
                                  {JointValue::WRIST_RIGHT_ROLL, 0},
                                  {JointValue::WAIST_PITCH, 0},
                                  {JointValue::WAIST_ROLL, 0},
                                  {JointValue::WAIST_YAW, 0},
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
            rgbCameraInfo = new CameraInfo(mujCtx->model, (name + "_rgb_cam").c_str());
            // Create Oracle with the pose and all robots
            oracle = new Oracle(mujCtx->model, mujCtx->data, name, pose);

            if (connectMode_ == "shm") {
                const ImageMeta rgbMeta = rgbCamera->serializeSHM();
                const ImageMeta depthMeta = depthCamera->serializeSHM();
                const size_t rgbBytes = static_cast<size_t>(rgbMeta.width) * rgbMeta.height * rgbMeta.channels;
                const size_t depthBytes = static_cast<size_t>(depthMeta.width) * depthMeta.height * depthMeta.channels;
                imageBuffer_.resize(rgbBytes + depthBytes);

                // Configuration of shm channels
                image_writer_.configure(image_shm_path, imageBuffer_.size(), BoosterT2ImageMeta{kBoosterT2ImageSchemaId, rgbMeta, depthMeta});
                state_writer_.configure(send_shm_path, sizeof(BoosterT2SharedState), BoosterT2StateMeta{});
                command_reader_.configure(receive_shm_path);
            }
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
            JointTorques<kBoosterT2JointCount> torques;
            if (!command_reader_.readLatest(&torques, sizeof(torques))) {
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
            msg["head_pose"] = headPose->serialize(buffer_zone_);
            msg["imu"] = imu->serialize(buffer_zone_);
            msg["joints"] = joints->serialize(buffer_zone_);
            msg["oracle"] = oracle->serialize(buffer_zone_);
            msg["camera_info"] = rgbCameraInfo->serialize(buffer_zone_);

            return msg;
        }

        void sendMessageSHM(bool publishImages) override {
            const BoosterT2SharedState state{pose->serializeSHM(), imu->serializeSHM(), joints->serializeSHM(),
                                             oracle->serializeSHM(), headPose->serializeSHM(), rgbCameraInfo->serializeSHM()};
            state_writer_.write(&state, sizeof(state));

            // Camera frames don't change between physics substeps -- rendering
            // happens on the GUI thread on its own cadence -- so only copy and
            // publish them once per control step, not on every substep.
            if (!publishImages) {
                return;
            }
            const auto& rgbFrame = rgbCamera->getImage();
            const auto& depthFrame = depthCamera->getDepth16UC1();
            std::memcpy(imageBuffer_.data(), rgbFrame.data(), rgbFrame.size());
            std::memcpy(imageBuffer_.data() + rgbFrame.size(), depthFrame.data(), depthFrame.size());
            image_writer_.write(imageBuffer_.data(), imageBuffer_.size());
        }

        std::map<std::string, SensorBase*> getSensors() override {
            std::map<std::string, SensorBase*> sensors;
            sensors["pose"] = pose;
            sensors["head_pose"] = headPose;
            sensors["imu"] = imu;
            sensors["joints"] = joints;
            sensors["rgb_camera"] = rgbCamera;
            sensors["depth_camera"] = depthCamera;
            sensors["camera_info"] = rgbCameraInfo;
            return sensors;
        }

        void applyCommands() override {
            std::lock_guard<std::mutex> lock(mutex_);
            joints->set_torque(latestTorques);
        }

        void update() override {
            pose->update();
            headPose->update();
            imu->update();
            joints->update();
            oracle->update();
            rgbCamera->update();
            depthCamera->update();
            rgbCameraInfo->update();
        }

        ~BoosterT2() = default;

    private:
        std::map<JointValue, std::string> joint_map;
        std::unordered_map<JointValue, mjtNum> latestTorques;

        std::string shm_dir_;
        SharedMemoryWriter<BoosterT2StateMeta> state_writer_;
        SharedMemoryWriter<BoosterT2ImageMeta> image_writer_;
        std::vector<uint8_t> imageBuffer_;  // persistent rgb+depth concat buffer for image_writer_
        SharedMemoryReader<> command_reader_;
};

}  // namespace spqr
