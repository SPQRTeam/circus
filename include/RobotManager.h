#pragma once

#include <mujoco/mujoco.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <msgpack.hpp>
#include <msgpack/v3/object_fwd_decl.hpp>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "Constants.h"
#include "MujocoContext.h"
#include "Utils.h"
#include "robots/BoosterK1.h"
#include "robots/BoosterT1.h"
#include "robots/Robot.h"

using namespace std::filesystem;

#define MAX_MSG_SIZE 1048576  // 1MB
namespace spqr {

struct Team;  // Forward declaration

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

    void update() {
        std::lock_guard lock(mutex_);
        for (std::shared_ptr<Robot> r : robots_) {
            r->update();
        }
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

    std::shared_ptr<Robot> create(const std::string& name, const std::string& type, uint8_t number,
                                  const Eigen::Vector3d& pos, const Eigen::Vector3d& ori,
                                  const std::shared_ptr<Team> team) {
        auto it = robotFactory.find(type);
        if (it != robotFactory.end())
            return it->second(name, type, number, pos, ori, team);
        return nullptr;
    }

    void startContainers() {
        startCommunicationServer(frameworkCommunicationPort);

        YAML::Node pathsRoot = loadYamlFile(pathsConfigPath);
        YAML::Node configRoot = loadYamlFile(frameworkConfigPath);

        if (!configRoot["image"])
            throw std::runtime_error("Missing 'image' key in YAML file");

        std::string image = tryString(configRoot["image"], "'image' must be a string: ");

        if (!configRoot["volumes"] || !configRoot["volumes"].IsSequence())
            throw std::runtime_error("'volumes' key missing or not a sequence");

        std::vector<std::string> binds;
        for (const auto& v : configRoot["volumes"]) {
            std::string v2 = tryString(v, "Volume entry must be a string: ");
            if (v2.starts_with("<")) {
                int end = v2.find('>');
                std::string name = v2.substr(1, end - 1);

                if (!pathsRoot[name]) {
                    throw std::runtime_error("Entry doesn't exist in path_constants: " + name);
                }

                std::string name_str = tryString(pathsRoot[name], "path_constants entries must be strings: ");
                v2.replace(0, end + 1, name_str);
            }
            binds.push_back(v2);
        }

        for (std::shared_ptr<Robot> r : robots_) {
            r->container = std::make_unique<Container>(r->name + "_container");
            r->container->create(r->name, image, binds);
            r->container->start();
        }
    }

    void startCommunicationServer(int port) {
        if (serverRunning_)
            throw std::runtime_error("Server already running");
        serverRunning_ = true;
        serverThread_ = std::thread(&RobotManager::_serverInternal, this, port);
    }

    void stopCommunicationServer() {
        if (!serverRunning_)
            return;

        serverRunning_ = false;

        if (serverThread_.joinable())
            serverThread_.join();
    }

    void setAreAllRobotsReadyCallback(std::function<void()> cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        areAllRobotsReadyCallback_ = std::move(cb);
    }

   private:
    RobotManager() = default;
    ~RobotManager() = default;

    RobotManager(const RobotManager&) = delete;
    RobotManager& operator=(const RobotManager&) = delete;


    // Source - https://stackoverflow.com/a
    // Posted by Some programmer dude, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-01-12, License - CC BY-SA 3.0

    // TODO: la recv_all o non funziona, o vuole sapere perfettamente quale è il numero di bytes da leggere
    //       Se il numerodi bytes indicato è maggiore di quello che bisogna leggere, si blocca.
    //       Quindi può essere usato solo con un protocollo TCP-like, dove prima viene mandato un messaggio
    //       indicante il numero di bytes del messaggio che seguirà
    ssize_t recv_all(int fd, void *buf, size_t len)
    {
        size_t toread = len;
        char  *bufptr = (char*) buf;

        while (toread > 0)
        {
            ssize_t rsz = recv(fd, bufptr, toread, 0);
            if (rsz <= 0)
                return rsz;  /* Error or other end closed cnnection */

            toread -= rsz;  /* Read less next time */
            bufptr += rsz;  /* Next buffer position to read into */
        }

        return len;
    }

    // Source - https://stackoverflow.com/a
    // Posted by Arun, modified by community. See post 'Timeline' for change history
    // Retrieved 2026-01-12, License - CC BY-SA 3.0
    // TCP Communication, it sends before the size of the message and then the message itself
    ssize_t send_all(int fd, char *buf, size_t len)
    {
        // // First, send the size of the message
        // uint32_t msg_size = htonl(len);
        // ssize_t bytes_sent = send(fd, &msg_size, sizeof(msg_size), 0);
        // if (bytes_sent != sizeof(msg_size)) {
        //     return -1;
        // }

        // ssize_t total = 0; // how many bytes we've sent
        // size_t bytesleft = len; // how many we have left to send
        // ssize_t n = 0;
        // while(total < len) {
        //     n = send(fd, buf+total, bytesleft, 0);
        //     if (n == -1) { 
        //         /* print/log error details */
        //         return -1;
        //     }
        //     total += n;
        //     bytesleft -= n;
        // }
        // return total;
        return send(fd, buf, len, 0);
    }

    int create_fifo(const char* path, int mode) {
        int res = -1;
        int tries = 0;
        while (res < 0 && tries < 2) {
            res = mkfifo(path, mode);
            tries += 1;
            if (res < 0)  {
                if (errno == EEXIST) {
                    // std::cout << "Server FIFO exists, deleting" << std::endl;
                    remove(path);
                }
                else {
                    return res;
                }
            }
        }
        return res;
    }

    void _serverInternal(int port) {
        // TODO tocca usare un file su RAM
        // https://www.jamescoyle.net/knowledge/951-the-difference-between-a-tmpfs-and-ramfs-ram-disk
        std::string fifos_root = "/tmp/fifos";
        std::string server_path = fifos_root + "/server";
        create_directory(fifos_root);
        int res = create_fifo(server_path.c_str(), 0777);
        if (res < 0)  {
            perror("");
            throw std::runtime_error("Failed to create FIFO");
        }
        int server_fd = open(server_path.c_str(), O_RDONLY);

        std::vector<pollfd> fds;
        fds.push_back({server_fd, POLLIN, 0});

        // Using a polling server. It isn't immediately intuitive, but it is efficient for this use case.
        while (serverRunning_) {
            // the poll blocks until a new connection arrives on server_fd or data arrives in one of the
            // monitored fd or a socket closes or the timeout expires.
            int ret = poll(fds.data(), fds.size(), 100);
            if (ret <= 0)
                continue;  // Timeout, skip iteration (timeout necessary to check whether serverRunning_ is
                           // still true)

            for (size_t i = 0; i < fds.size(); ++i) {
                // An event occured for the i-th fd
                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == server_fd) {
                        // The only event for the server is someone knocking
                        // Receive initial message with robot name
                        char buffer[MAX_MSG_SIZE];
                        int n = read(server_fd, buffer, sizeof(buffer) - 1);

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
                        std::cout << robotName << std::endl;

                        std::string client_path = fifos_root + robotName;
                        int res = create_fifo(client_path.c_str(), 0777);
                        if (res < 0)  {
                            perror("Failed to create FIFO");
                            throw std::runtime_error("Failed to create FIFO");
                        }
                        int client_fd = open(client_path.c_str(), O_WRONLY);
                        if (client_fd < 0)  {
                            perror("Failed to create FIFO");
                            throw std::runtime_error("Failed to create FIFO");
                        }

                        // TODO creare altra fifo
                        if (client_fd >= 0) {
                            fds.push_back({client_fd, POLLIN, 0});

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
                            if (answOk){
                                msgpack::pack(sbuf, answ);
                                if(sbuf.size() > 0) {
                                    std::cout << "Connected Robot: " << robotName << "\n";
                                    std::cout << "Sending initial message to " << robotName << std::endl;
                                    ssize_t bytes_sent = write(client_fd, sbuf.data(), sbuf.size());
                                    if (bytes_sent <= 0) {
                                        perror("Sending initial message");
                                    }
                                }
                            }
                        }
                    } else {
                        // The events for other fds indicate either an incoming message or a closed connection
                        // the read call disambiguates the two cases
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
                        msgpack::sbuffer sbuf(1024*1024*7);
                        std::map<std::string, msgpack::object> answ;
                        bool answOk = false;
                        {
                            std::unique_lock lock(mutex_);
                            for (auto& r : robots_) {
                                if (r->name == messageRecipient) {
                                    if (!r->isReady) {
                                        r->isReady = true;
                                        std::cout << "Robot ready: " << r->name << std::endl;
                                        areAllRobotsReadyWrapper();
                                    }
                                    r->receiveMessage(data_map);
                                    answ = r->sendMessage();
                                    answOk = true;
                                    break;
                                }
                            }
                        }

                        if (answOk){
                            msgpack::pack(sbuf, answ);
                            if(sbuf.size() > 0){
                                ssize_t bytes_sent = send_all(fds[i].fd, sbuf.data(), sbuf.size());
                                if (bytes_sent <= 0) {
                                    perror("Sending message");
                                }
                            }
                        }
                    }
                }
            }
        }

        for (auto& fd : fds)
            close(fd.fd);
    }

    void areAllRobotsReadyWrapper() {
        if (areAllRobotsReady() && areAllRobotsReadyCallback_) {
            areAllRobotsReadyCallback_();
        }
    }
    bool areAllRobotsReady() const {
        for (const auto& r : robots_)
            if (!r->isReady)
                return false;
        std::cout << "All robots are ready!" << std::endl;
        return true;
    }

    std::atomic<bool> serverRunning_ = false;
    std::thread serverThread_;

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Robot>> robots_;
    std::function<void()> areAllRobotsReadyCallback_;

    using RobotCreator = std::function<std::shared_ptr<Robot>(const std::string&, const std::string&, uint8_t,
                                                              const Eigen::Vector3d&, const Eigen::Vector3d&,
                                                              const std::shared_ptr<Team>&)>;

    std::unordered_map<std::string, RobotCreator> robotFactory
        = {{"Booster-K1",
            [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& team) {
                return std::make_shared<BoosterK1>(name, type, number, pos, ori, team);
            }},
           {"Booster-T1", [](auto&& name, auto&& type, uint8_t number, auto&& pos, auto&& ori, auto&& team) {
                return std::make_shared<BoosterT1>(name, type, number, pos, ori, team);
            }}};
};

}  // namespace spqr
