#pragma once

#include <mujoco/mujoco.h>
#include <sys/types.h>

#include <Eigen/Eigen>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include <netinet/in.h>

#include "Camera.h"
#include "Container.h"
#include "Imu.h"
#include "Joint.h"
#include "MujocoContext.h"

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

class Robot {
   public:
    Robot() = default;
    virtual ~Robot() = default;
    virtual void bindMujoco(MujocoContext* mujContext) = 0;
    virtual void update() = 0;
    virtual void handleMessage(const std::string& payload) = 0;

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
        cameras[0] = new Camera(mujCtx, (name + "_left_cam").c_str());
        cameras[1] = new Camera(mujCtx, (name + "_right_cam").c_str());
    }

    void handleMessage(const std::string& payload) override {
        std::cout << "Robot " << name << " received: " << payload << std::endl;
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
        cameras[0] = new Camera(mujCtx, (name + "_left_cam").c_str());
        cameras[1] = new Camera(mujCtx, (name + "_right_cam").c_str());
    }

    void handleMessage(const std::string& payload) override {
        std::cout << "Robot " << name << " received: " << payload << std::endl;
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

    void startCommunicationServer(int port){
        if (serverRunning_) throw std::runtime_error("Server already running");
        serverRunning_ = true;
        serverThread_ = std::thread(&RobotManager::_serverInternal, this, port);
    }

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;

    void _serverInternal(int port){
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) throw std::runtime_error("Failed to create socket");

        auto close_socket = [&]() { close(server_fd); };
        struct SocketGuard { 
            int fd; 
            decltype(close_socket)& f; 
            ~SocketGuard(){ f(); } 
        } guard{server_fd, close_socket};

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("Failed to set socket options");

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
            throw std::runtime_error("Socket bind failed");

        if (listen(server_fd, robots_.size()) < 0)
            throw std::runtime_error("Listen failed");

        std::cout << "TCP server listening on port " << port << std::endl;

        while (serverRunning_) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) continue;

            std::thread(&RobotManager::handleServerClient_, this, client_fd).detach();
        }
    }

    void handleServerClient_(int client_fd){
        char buffer[MAX_MSG_SIZE];

        while (true) {
            int n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n <= 0) break; // Peer disconnected 
            buffer[n] = '\0';
            std::string msg(buffer);
            auto sep = msg.find(':');
            // Invalid message, should we handle this explicitely? If so, we should use a RAAI guard for the client_fd
            if (sep == std::string::npos) continue;  
            std::string robotName = msg.substr(0, sep);
            std::string payload = msg.substr(sep + 1);

            for(std::shared_ptr<Robot> r : robots_){
                if(r->name == robotName){
                    r->handleMessage(payload);
                    break;
                }
            }
        }
    }

    std::atomic<bool> serverRunning_ = false;
    std::thread serverThread_;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;
    
    std::unordered_map<std::string, std::function<std::shared_ptr<Robot>()>> robotFactory
        = {{"Booster-K1", [] { return std::make_shared<K1>(); }},
           {"Booster-T1", [] { return std::make_shared<T1>(); }}};
};

}  // namespace spqr
