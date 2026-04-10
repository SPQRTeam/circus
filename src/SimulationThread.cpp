#include "SimulationThread.h"

#include <set>
#include <stdexcept>

#include "GameController.h"
#include "RobotManager.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data) : model_(model), data_(data), running_(true), paused_(false) {}

// Source - https://stackoverflow.com/a
// Posted by Arun, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-12, License - CC BY-SA 3.0
// TCP Communication, it sends before the size of the message and then the message itself
ssize_t SimulationThread::send_all(int fd, char* buf, size_t len) {
    // First, send the size of the message
    uint32_t msg_size = htonl(len);
    ssize_t bytes_sent = send(fd, &msg_size, sizeof(msg_size), 0);
    if (bytes_sent != sizeof(msg_size)) {
        return -1;
    }

    ssize_t total = 0;       // how many bytes we've sent
    size_t bytesleft = len;  // how many we have left to send
    ssize_t n = 0;
    while (total < len) {
        n = send(fd, buf + total, bytesleft, 0);
        if (n == -1) {
            /* print/log error details */
            return -1;
        }
        total += n;
        bytesleft -= n;
    }
    return total;
}

void SimulationThread::initializeSocket(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("Failed to create socket");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int send_buf_size = 1 * 1024 * 1024;
    if (setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size)) < 0) {
        perror("setsockopt(SO_SNDBUF)");
    }
    int recv_buf_size = 1 * 1024 * 1024;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size)) < 0) {
        perror("setsockopt(SO_RCVBUF)");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    robots_ = RobotManager::instance().getRobots();
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        throw std::runtime_error("Socket bind failed");
    }
    if (listen(server_fd, robots_.size()) < 0)
        throw std::runtime_error("Listen failed");

    fds.push_back({server_fd, POLLIN, 0});
}

void SimulationThread::receiveCommandMessages() {
    int robot_size = robots_.size();
    int done = 0;
    // Track which robots have not yet replied this step
    std::set<std::string> pendingRobots;
    for (auto& r : robots_)
        pendingRobots.insert(r->name);

    int timeoutCount = 0;
    while (done < robot_size) {
        int ret = poll(fds.data(), fds.size(), 500);
        if (ret <= 0) {
            ++timeoutCount;
            // Resend state only every 5 timeouts (2.5s) to avoid flooding the TCP buffer
            if (timeoutCount % 5 != 0)
                continue;
            std::unique_lock lock(mutex_);
            for (auto& r : robots_) {
                if (pendingRobots.count(r->name)) {
                    std::cout << "[receiveCommandMessages] Timeout, resending state to: " << r->name << std::endl;
                    msgpack::sbuffer sbuf;
                    auto answ = r->sendMessage();
                    msgpack::pack(sbuf, answ);
                    if (sbuf.size() > 0) {
                        int fd = entity_fd_map[r->name];
                        if (fd)
                            send_all(fd, sbuf.data(), sbuf.size());
                        else
                            std::cerr << "[receiveCommandMessages] No fd for: " << r->name << std::endl;
                    }
                }
            }
            continue;
        }
        timeoutCount = 0;

        for (size_t i = 0; i < fds.size(); ++i) {
            // An event occured for the i-th fd
            if (fds[i].revents & POLLIN) {
                char buffer[MAX_MSG_SIZE];
                int n = read(fds[i].fd, buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    --i;
                    continue;
                }

                msgpack::object_handle oh = msgpack::unpack(buffer, n);
                auto data_map = oh.get().as<std::map<std::string, msgpack::object>>();
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
                                if (RobotManager::instance().areAllRobotsReady()) {
                                    emit allRobotsReadySignal();
                                }
                            }

                            r->receiveMessage(data_map);
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

void SimulationThread::waitRobotConnections() {
    bool areAllConnected = false;
    while (!areAllConnected) {
        int ret = poll(fds.data(), fds.size(), 100);
        if (ret <= 0)
            continue;  // Timeout, skip iteration (timeout necessary to check whether serverRunning_ is

        for (size_t i = 0; i < fds.size(); ++i) {
            // An event occured for the i-th fd
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == server_fd) {
                    if (!entity_fd_map["server"]) {
                        entity_fd_map["server"] = server_fd;
                    }
                    // The only event for the server is someone knocking
                    int client_fd = accept(server_fd, nullptr, nullptr);
                    if (client_fd >= 0) {
                        fds.push_back({client_fd, POLLIN, 0});

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
                        entity_fd_map[robotName] = client_fd;

                        // Send message with initial state
                        msgpack::sbuffer sbuf;
                        std::map<std::string, msgpack::object> answ;
                        bool answOk = false;
                        // Pack initial message
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            for (auto& r : robots_) {
                                if (r->name == robotName) {
                                    r->isConnected = true;
                                    answ = r->sendMessage();
                                    answOk = true;
                                    break;
                                }
                            }
                        }
                        if (answOk) {
                            msgpack::pack(sbuf, answ);
                            if (sbuf.size() > 0) {
                                std::cout << "Connected Robot: " << robotName << "\n";
                                std::cout << "Sending initial message to " << robotName << std::endl;
                                ssize_t bytes_sent = send_all(client_fd, sbuf.data(), sbuf.size());
                                if (bytes_sent <= 0) {
                                    perror("Error in sending initial message");
                                }
                            }
                        }
                        if (RobotManager::instance().areAllRobotsConnected()) {
                            areAllConnected = true;
                            std::cout << "All Robots are connected!" << std::endl;
                            break;
                        }
                    }
                }
            }
        }
    }
}

/*
//  SimulationThread IDEA
    mj_step1();
    applyCommands();
    mj_step2();
    update();
    // ogni N step (per matchare ~100Hz):
    for each robot:
        send(state)       // non-blocking
        recv(torques)     // bloccante o con timeout
*/

void SimulationThread::run() {
    if (!model_)
        throw std::runtime_error("Cannot start simulation without mujoco model");

    double sim_dt = model_->opt.timestep;

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();
    while (running_) {
        if (!paused_) {
            mj_step1(model_, data_);
            RobotManager::instance().applyCommands();
            mj_step2(model_, data_);
            RobotManager::instance().update();
            GameController::instance().update();

            std::memset(data_->xfrc_applied, 0, model_->nbody * 6 * sizeof(mjtNum));

            if (maxSimulationTime_ > 0 && data_->time >= maxSimulationTime_) {
                running_ = false;
                emit maxSimulationTimeReached();
                break;
            }

            // for each robot:
            //      send(state)       // non-blocking
            //      recv(torques)     // bloccante o con timeout

            sendStateMessages();
            receiveCommandMessages();

            next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(sim_dt));
            // std::this_thread::sleep_until(next_step_time);

            // if (clock::now() > next_step_time)
            //     next_step_time = clock::now();
        } else {
            // When paused, sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Reset next_step_time when paused to avoid catching up when playd
            next_step_time = clock::now();
        }
    }
}

void SimulationThread::sendStateMessages() {
    for (auto& r : robots_) {
        msgpack::sbuffer sbuf;
        std::map<std::string, msgpack::object> answ;
        bool answOk = false;
        {
            std::unique_lock lock(mutex_);
            answ = r->sendMessage();
            answOk = true;
        }

        if (answOk) {
            msgpack::pack(sbuf, answ);
            if (sbuf.size() > 0) {
                int fd = entity_fd_map[r->name];
                if (!fd)
                    perror("file descriptor not existing");
                ssize_t bytes_sent = send_all(fd, sbuf.data(), sbuf.size());
                if (bytes_sent <= 0) {
                    perror("Sending message");
                }
            }
        }
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

void SimulationThread::pause() {
    paused_ = true;
}

void SimulationThread::play() {
    paused_ = false;
}

bool SimulationThread::isPaused() {
    return paused_;
}

void SimulationThread::setMaxSimulationTime(int maxTime) {
    maxSimulationTime_ = maxTime;
}

}  // namespace spqr
