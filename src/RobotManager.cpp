#include "RobotManager.h"

#include <filesystem>
#include <unistd.h>

#include "Constants.h"
#include "Team.h"
#include "Utils.h"

namespace spqr {

void RobotManager::applyCommands() {
    for (std::shared_ptr<Robot> r : robots_) {
        r->applyCommands();
    }
}

void RobotManager::registerRobot(std::shared_ptr<Robot> robot) {
    std::lock_guard<std::mutex> lock(mutex_);

    robots_.push_back(std::move(robot));
}

std::vector<std::shared_ptr<Robot>> RobotManager::getRobots() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return robots_;
}

size_t RobotManager::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return robots_.size();
}

void RobotManager::setRobotFd(const std::string& name, int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    robotFdMap_[name] = fd;
}

int RobotManager::getRobotFd(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = robotFdMap_.find(name);
    return it != robotFdMap_.end() ? it->second : -1;
}

void RobotManager::removeRobotFd(int fd) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = robotFdMap_.begin(); it != robotFdMap_.end(); ++it) {
        if (it->second == fd) {
            robotFdMap_.erase(it);
            break;
        }
    }
}

void RobotManager::update() {
    std::lock_guard lock(mutex_);
    for (std::shared_ptr<Robot> r : robots_) {
        r->update();
    }
}

void RobotManager::clear() {
    std::lock_guard lock(mutex_);
    for (std::shared_ptr<Robot> r : robots_) {
        // Drop ownership first
        r->container.reset();
        r->team.reset();
    }
    robots_.clear();
}

void RobotManager::sendStateMessages() {
    if (connectMode_ == "shm") {
        // std::cout << "[sendStateMessages] sending message with SHM" << std::endl;
        sendStateMessagesSHM();
    }
    else {
        // std::cout << "[sendStateMessages] sending message with Socket" << std::endl;
        sendStateMessagesSocket();
    }
}

void RobotManager::receiveCommandMessages() {
    if (connectMode_ == "shm") {
        // std::cout << "[receiveCommandMessages] receiving message with SHM" << std::endl;
        receiveCommandMessagesSHM();
    }
    else {
        // std::cout << "[receiveCommandMessages] receiving message with Socket" << std::endl;
        receiveCommandMessagesSocket();
    }
}


void RobotManager::sendStateMessagesSHM() {
    for (auto& r : robots_) {
        std::unique_lock lock(mutex_);
        r->sendMessageSHM();
    }
}

void RobotManager::sendStateMessagesSocket() {
    for (auto& r : robots_) {
        std::unique_lock lock(mutex_);
        auto fdIt = robotFdMap_.find(r->name);
        r->sendMessageSocket(fdIt != robotFdMap_.end() ? fdIt->second : -1);
    }
}

void RobotManager::receiveCommandMessagesSHM() {
    int robot_size = robots_.size();
    int done = 0;
    std::set<std::string> pendingRobots;
    for (auto& r : robots_)
        pendingRobots.insert(r->name);

    auto windowStart = std::chrono::steady_clock::now();
    while (done < robot_size) {
        {
            std::unique_lock lock(mutex_);
            for (auto& r : robots_) {
                if (!pendingRobots.count(r->name))
                    continue;
                if (r->receiveMessageSHM()) {
                    if (!r->isReady) {
                        r->isReady = true;
                        std::cout << "Robot ready: " << r->name << std::endl;
                        if (areAllRobotsReady() && areAllRobotsReadyCallback_) {
                            areAllRobotsReadyCallback_();
                        }
                    }
                    pendingRobots.erase(r->name);
                    ++done;
                }
            }
        }
        if (done < robot_size) {
            if (std::chrono::steady_clock::now() - windowStart > std::chrono::milliseconds(2500)) {
                std::unique_lock lock(mutex_);
                for (auto& r : robots_) {
                    if (!pendingRobots.count(r->name))
                        continue;
                    std::cout << "[receiveCommandMessagesSHM] Timeout, resending state to: " << r->name << std::endl;
                    r->sendMessageSHM();
                }
                windowStart = std::chrono::steady_clock::now();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }
}

void RobotManager::receiveCommandMessagesSocket() {
    int robot_size = robots_.size();
    int done = 0;
    // Track which robots have not yet replied this step
    std::set<std::string> pendingRobots;
    for (auto& r : robots_)
        pendingRobots.insert(r->name);

    int timeoutCount = 0;
    while (done < robot_size) {
        int ret = poll(pollFds_.data(), pollFds_.size(), 500);
        if (ret <= 0) {
            ++timeoutCount;
            // Resend state only every 5 timeouts (2.5s) to avoid flooding the TCP buffer
            if (timeoutCount % 5 != 0)
                continue;
            std::unique_lock lock(mutex_);
            for (auto& r : robots_) {
                if (pendingRobots.count(r->name)) {
                    std::cout << "[receiveCommandMessagesSocket] Timeout, resending state to: " << r->name << std::endl;

                    auto fdIt = robotFdMap_.find(r->name);
                    r->sendMessageSocket(fdIt != robotFdMap_.end() ? fdIt->second : -1);
                }
            }
            continue;
        }
        timeoutCount = 0;

        for (size_t i = 0; i < pollFds_.size(); ++i) {
            // An event occured for the i-th fd
            if (pollFds_[i].revents & POLLIN) {
                int fd = pollFds_[i].fd;
                msgpack::unpacker& unp = unpackers_[fd];
                unp.reserve_buffer(MAX_MSG_SIZE);
                int n = read(fd, unp.buffer(), unp.buffer_capacity());
                if (n <= 0) {
                    close(fd);
                    pollFds_.erase(pollFds_.begin() + i);
                    --i;
                    unpackers_.erase(fd);
                    removeRobotFd(fd);
                    continue;
                }
                unp.buffer_consumed(static_cast<size_t>(n));

                // Drain every complete message currently buffered (a single
                // read() can return several concatenated messages if we were
                // slow to read), keeping only the latest one -- older ones
                // queued up behind it are superseded and discarded on
                // purpose, never applied.
                msgpack::object_handle oh;
                msgpack::object_handle latest;
                bool gotOne = false;
                while (unp.next(oh)) {
                    latest = std::move(oh);
                    gotOne = true;
                }
                if (!gotOne)
                    continue;  // only a partial message so far; wait for more bytes

                auto data_map = latest.get().as<std::map<std::string, msgpack::object>>();
                auto it = data_map.find("robot_name");
                if (it == data_map.end())
                    continue;

                std::string messageRecipient = it->second.as<std::string>();

                {
                    std::unique_lock lock(mutex_);
                    for (auto& r : robots_) {
                        if (r->name == messageRecipient) {
                            if (!r->isReady) {
                                r->isReady = true;
                                std::cout << "Robot ready: " << r->name << std::endl;
                                if (areAllRobotsReady() && areAllRobotsReadyCallback_) {
                                    areAllRobotsReadyCallback_();
                                }
                            }

                            r->receiveMessageSocket(data_map);
                            pendingRobots.erase(messageRecipient);
                            ++done;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void RobotManager::initializeSocket(int port) {
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0)
        throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int send_buf_size = 1 * 1024 * 1024;
    if (setsockopt(serverFd_, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)) < 0) {
        perror("setsockopt(SO_SNDBUF)");
    }
    int recv_buf_size = 1 * 1024 * 1024;
    if (setsockopt(serverFd_, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size)) < 0) {
        perror("setsockopt(SO_RCVBUF)");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        throw std::runtime_error("Socket bind failed");
    }
    if (listen(serverFd_, count()) < 0)
        throw std::runtime_error("Listen failed");

    pollFds_.push_back({serverFd_, POLLIN, 0});
}

void RobotManager::waitRobotConnections() {
    if(connectMode_ == "shm") {
        waitRobotConnectionsSHM();
    }
    else {
        waitRobotConnectionsSocket();
    }
}

void RobotManager::waitRobotConnectionsSocket() {
    bool areAllConnected = false;
    while (!areAllConnected) {
        int ret = poll(pollFds_.data(), pollFds_.size(), 100);
        if (ret <= 0)
            continue;  // Timeout, skip iteration (timeout necessary to check whether serverRunning_ is

        for (size_t i = 0; i < pollFds_.size(); ++i) {
            // An event occured for the i-th fd
            if (pollFds_[i].revents & POLLIN) {
                if (pollFds_[i].fd == serverFd_) {
                    // The only event for the server is someone knocking
                    int client_fd = accept(serverFd_, nullptr, nullptr);
                    if (client_fd >= 0) {
                        pollFds_.push_back({client_fd, POLLIN, 0});

                        // Receive initial message with robot name
                        char buffer[MAX_MSG_SIZE];
                        int n = read(client_fd, buffer, sizeof(buffer) - 1);

                        if (n <= 0) {
                            std::cerr << "Error reading the initial message.\n";
                            // close(client_fd);
                            continue;
                        }

                        // unpack of the MsgPack message
                        msgpack::object_handle oh = msgpack::unpack(buffer, n);
                        msgpack::object obj = oh.get();

                        // First message is the robot name as a string
                        if (obj.type != msgpack::type::STR) {
                            std::cerr << "First message must be a string. Ignore it...\n";
                            continue;
                        }

                        std::string robotName = obj.as<std::string>();
                        setRobotFd(robotName, client_fd);

                        // Answer the handshake with the initial state over shared memory.
                        // The socket is only used to receive the robot name above.
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            for (auto& r : robots_) {
                                if (r->name == robotName) {
                                    r->isConnected = true;
                                    r->sendMessageSocket(client_fd);
                                    std::cout << "Connected Robot (socket): " << robotName << "\n";
                                    std::cout << "Published initial state to " << robotName << " via socket" << std::endl;
                                    break;
                                }
                            }
                        }
                        if (areAllRobotsConnected()) {
                            areAllConnected = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    std::cout << "All Robots are connected!" << std::endl;
}

void RobotManager::waitRobotConnectionsSHM() {
    std::set<std::string> pendingRobots;
    {
        std::unique_lock lock(mutex_);
        for (auto& r : robots_)
            if (!r->isConnected)
                pendingRobots.insert(r->name);
    }

    auto lastStatusLog = std::chrono::steady_clock::now();
    while (!pendingRobots.empty()) {
        {
            std::unique_lock lock(mutex_);
            for (auto& r : robots_) {
                if (pendingRobots.count(r->name) && r->hasConnectSignalSHM()) {
                    r->isConnected = true;
                    r->sendMessageSHM();
                    pendingRobots.erase(r->name);
                    std::cout << "Connected Robot (shm): " << r->name << "\n";
                }
            }
        }
        if (pendingRobots.empty())
            break;

        if (std::chrono::steady_clock::now() - lastStatusLog > std::chrono::milliseconds(2500)) {
            std::unique_lock lock(mutex_);
            std::cout << "[waitRobotConnectionsSHM] still waiting for:";
            for (auto& n : pendingRobots)
                std::cout << " " << n;
            std::cout << std::endl;
            lastStatusLog = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "All Robots are connected!" << std::endl;
}

void RobotManager::bindMujoco(MujocoContext* mujContext) {
    for (std::shared_ptr<Robot> r : robots_)
        r->bindMujoco(mujContext);
}

std::shared_ptr<Robot> RobotManager::create(const std::string& name, const std::string& type, uint8_t number, const Eigen::Vector3d& pos,
                                            const Eigen::Vector3d& ori, const std::string& colorName, const std::shared_ptr<Team> team,
                                            const std::string& role) {
    auto it = robotFactory.find(type);
    if (it != robotFactory.end()) {
        auto robot = it->second(name, type, number, pos, ori, colorName, team);
        if (robot) robot->role = role;
        return robot;
    }
    return nullptr;
}

void RobotManager::startContainers(const std::string& fwkCfgPath, const std::string& pathsCfgPath, const std::string& connectMode) {
    // Save connection mode
    connectMode_ = connectMode;
    
    YAML::Node configRoot = loadYamlFile(fwkCfgPath.c_str());

    if (!configRoot["image"])
        throw std::runtime_error("Missing 'image' key in YAML file");

    std::string image = tryString(configRoot["image"], "'image' must be a string: ");

    if (!configRoot["volumes"] || !configRoot["volumes"].IsSequence())
        throw std::runtime_error("'volumes' key missing or not a sequence");

    // Paths in framework config can be relative to PIXI_PROJECT_ROOT.
    const char* pixi_project_root = std::getenv("FRAMEWORK_PATH") ? std::getenv("FRAMEWORK_PATH") : std::getenv("PIXI_PROJECT_ROOT");

    std::vector<std::string> binds;
    std::optional<YAML::Node> pathsRoot;
    for (const auto& v : configRoot["volumes"]) {
        std::string v2 = tryString(v, "Volume entry must be a string: ");
        if (v2.starts_with("<")) {
            if (!pathsRoot)
                pathsRoot = loadYamlFile(pathsCfgPath.c_str());
            int end = v2.find('>');
            std::string name = v2.substr(1, end - 1);

            if (!(*pathsRoot)[name]) {
                throw std::runtime_error("Entry doesn't exist in path_constants: " + name);
            }

            std::string name_str = tryString((*pathsRoot)[name], "path_constants entries must be strings: ");
            v2.replace(0, end + 1, name_str);
        } else {
            // Resolve relative / empty host paths against PIXI_PROJECT_ROOT.
            auto colon = v2.find(':');
            if (colon != std::string::npos) {
                std::string host = v2.substr(0, colon);
                if (host.empty() || host[0] != '/') {
                    if (!pixi_project_root)
                        throw std::runtime_error("PIXI_PROJECT_ROOT and FRAMEWORK_PATH environment variables are not set");
                    namespace fs = std::filesystem;
                    fs::path hp = fs::weakly_canonical(fs::path(pixi_project_root) / host);
                    v2 = hp.string() + v2.substr(colon);
                }
            }
        }
        binds.push_back(v2);
    }
    for (std::shared_ptr<Robot> r : robots_) {
        r->container = std::make_unique<Container>("CIRCUS_" + r->name + "_container");
        r->container->create(r, image, binds, connectMode);
        r->container->start();
    }
    std::cout << "Containers started successfully!" << std::endl;
}

bool RobotManager::areAllRobotsConnected() const {
    for (auto& r : robots_) {
        if (!r->isConnected) {
            return false;
        }
    }
    return true;
}

bool RobotManager::areAllRobotsReady() const {
    for (const auto& r : robots_)
        if (!r->isReady)
            return false;
    std::cout << "All robots are ready!" << std::endl;
    return true;
}

void RobotManager::setAreAllRobotsReadyCallback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    areAllRobotsReadyCallback_ = std::move(cb);
}

}  // namespace spqr
