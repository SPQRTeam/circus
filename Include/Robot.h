#pragma once

#include <mujoco/mujoco.h>
#include <sys/types.h>

#include <Eigen/Eigen>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Camera.h"
#include "Container.h"
#include "Imu.h"
#include "Joint.h"
#include "MujocoContext.h"
namespace spqr {

struct Team;  // Forward declaration

class Robot {
   public:
    Robot() = default;
    virtual ~Robot() = default;
    virtual void bindMujoco(MujocoContext* mujContext) = 0;
    virtual void update() = 0;

    std::string name;
    std::string type;
    uint8_t number;
    Eigen::Vector3d position;
    Eigen::Vector3d orientation;  // Euler angles
    std::unique_ptr<Container> container;
    std::shared_ptr<Team> team;
};

class T1 : public Robot {
   public:
    T1()
        : joint_map{{JointValue::HEAD_YAW, "AAHead_yaw"},
                    {JointValue::HEAD_PITCH, "Head_pitch"},
                    {JointValue::SHOULDER_LEFT_PITCH, "Left_Shoulder_Pitch"},
                    {JointValue::SHOULDER_LEFT_ROLL, "Left_Shoulder_Roll"},
                    {JointValue::ELBOW_LEFT_PITCH, "Left_Elbow_Pitch"},
                    {JointValue::ELBOW_LEFT_YAW, "Left_Elbow_Yaw"},
                    {JointValue::SHOULDER_RIGHT_PITCH, "Right_Shoulder_Pitch"},
                    {JointValue::SHOULDER_RIGHT_ROLL, "Right_Shoulder_Roll"},
                    {JointValue::ELBOW_RIGHT_PITCH, "Right_Elbow_Pitch"},
                    {JointValue::ELBOW_RIGHT_YAW, "Right_Elbow_Yaw"},
                    {JointValue::WAIST, "Waist"},
                    {JointValue::HIP_LEFT_PITCH, "Left_Hip_Pitch"},
                    {JointValue::HIP_LEFT_ROLL, "Left_Hip_Roll"},
                    {JointValue::HIP_LEFT_YAW, "Left_Hip_Yaw"},
                    {JointValue::KNEE_LEFT_PITCH, "Left_Knee_Pitch"},
                    {JointValue::ANKLE_LEFT_PITCH, "Left_Ankle_Pitch"},
                    {JointValue::ANKLE_LEFT_ROLL, "Left_Ankle_Roll"},
                    {JointValue::HIP_RIGHT_PITCH, "Right_Hip_Pitch"},
                    {JointValue::HIP_RIGHT_ROLL, "Right_Hip_Roll"},
                    {JointValue::HIP_RIGHT_YAW, "Right_Hip_Yaw"},
                    {JointValue::KNEE_RIGHT_PITCH, "Right_Knee_Pitch"},
                    {JointValue::ANKLE_RIGHT_PITCH, "Right_Ankle_Pitch"},
                    {JointValue::ANKLE_RIGHT_ROLL, "Right_Ankle_Roll"}} {}

    void bindMujoco(MujocoContext* mujCtx) override {
        joints = new Joints(mujCtx->model, mujCtx->data, joint_map);
        imu = new Imu(mujCtx->model, mujCtx->data, "orientation", "angular-velocity");
        cameras[0] = new Camera(mujCtx->model, mujCtx->data, (name + "_left_cam").c_str());
        cameras[1] = new Camera(mujCtx->model, mujCtx->data, (name + "_right_cam").c_str());
    }

    void update() override {
        cameras[0]->update();
        cameras[1]->update();
        joints->update();
        imu->update();
    }

    ~T1(){};

   private:
    std::unordered_map<JointValue, std::string> joint_map;

    std::array<Camera*, 2> cameras = {};
    Imu* imu;
    Joints* joints = nullptr;
};

class K1 : public Robot {
   public:
    K1()
        : joint_map{{JointValue::HEAD_YAW, "AAHead_yaw"},
                    {JointValue::HEAD_PITCH, "Head_pitch"},
                    {JointValue::SHOULDER_LEFT_PITCH, "ALeft_Shoulder_Pitch"},
                    {JointValue::SHOULDER_LEFT_ROLL, "Left_Shoulder_Roll"},
                    {JointValue::ELBOW_LEFT_PITCH, "Left_Elbow_Pitch"},
                    {JointValue::ELBOW_LEFT_YAW, "Left_Elbow_Yaw"},
                    {JointValue::SHOULDER_RIGHT_PITCH, "ARight_Shoulder_Pitch"},
                    {JointValue::SHOULDER_RIGHT_ROLL, "Right_Shoulder_Roll"},
                    {JointValue::ELBOW_RIGHT_PITCH, "Right_Elbow_Pitch"},
                    {JointValue::ELBOW_RIGHT_YAW, "Right_Elbow_Yaw"},
                    {JointValue::HIP_LEFT_PITCH, "Left_Hip_Pitch"},
                    {JointValue::HIP_LEFT_ROLL, "Left_Hip_Roll"},
                    {JointValue::HIP_LEFT_YAW, "Left_Hip_Yaw"},
                    {JointValue::KNEE_LEFT_PITCH, "Left_Knee_Pitch"},
                    {JointValue::ANKLE_LEFT_PITCH, "Left_Ankle_Pitch"},
                    {JointValue::ANKLE_LEFT_ROLL, "Left_Ankle_Roll"},
                    {JointValue::HIP_RIGHT_PITCH, "Right_Hip_Pitch"},
                    {JointValue::HIP_RIGHT_ROLL, "Right_Hip_Roll"},
                    {JointValue::HIP_RIGHT_YAW, "Right_Hip_Yaw"},
                    {JointValue::KNEE_RIGHT_PITCH, "Right_Knee_Pitch"},
                    {JointValue::ANKLE_RIGHT_PITCH, "Right_Ankle_Pitch"},
                    {JointValue::ANKLE_RIGHT_ROLL, "Right_Ankle_Roll"}} {}

    void bindMujoco(MujocoContext* mujCtx) override {
        joints = new Joints(mujCtx->model, mujCtx->data, joint_map);
        imu = new Imu(mujCtx->model, mujCtx->data, (name + "_orientation").c_str(),
                      (name + "_angular-velocity").c_str());
        cameras[0] = new Camera(mujCtx->model, mujCtx->data, (name + "_left_cam").c_str());
        cameras[1] = new Camera(mujCtx->model, mujCtx->data, (name + "_right_cam").c_str());
    }

    void update() override {
        cameras[0]->update();
        cameras[1]->update();
        joints->update();
        imu->update();
    }

    ~K1(){};

   private:
    std::unordered_map<JointValue, std::string> joint_map;

    std::array<Camera*, 2> cameras = {};
    Imu* imu;
    Joints* joints = nullptr;
};

class RobotManager {
   public:
    // Singleton class
    static RobotManager& instance() {
        static RobotManager mgr;
        return mgr;
    }

    void registerRobot(std::shared_ptr<Robot> robot) {
        std::lock_guard<std::mutex> lock(mutex_);

        robots_.push_back(std::move(robot));
    }

    std::vector<std::shared_ptr<Robot>> getRobots() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return robots_.size();
    }

    void clear() {
        std::lock_guard lock(mutex_);
        for (std::shared_ptr<Robot> r : robots_) {
            // Drop ownership first
            r->container.reset();
            r->team.reset();
        }
        robots_.clear();
    }

    void bindMujoco(MujocoContext* mujContext) {
        for (std::shared_ptr<Robot> r : robots_)
            r->bindMujoco(mujContext);
    }

    std::shared_ptr<Robot> create(const std::string& robotType) {
        auto it = robotFactory.find(robotType);
        if (it != robotFactory.end())
            return it->second();
        return nullptr;
    }

    void startContainers(const std::string& image) {
        for (std::shared_ptr<Robot> r : robots_) {
            r->container = std::make_unique<Container>(r->name + "_container");
            r->container->create(image, {});
            r->container->start();
        }
    }

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;
    std::unordered_map<std::string, std::function<std::shared_ptr<Robot>()>> robotFactory
        = {{"Booster-K1", [] { return std::make_shared<K1>(); }},
           {"Booster-T1", [] { return std::make_shared<T1>(); }}};
};

}  // namespace spqr
