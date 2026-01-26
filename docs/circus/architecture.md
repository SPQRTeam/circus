# Architecture Overview

Circus implements a distributed architecture where the physics simulation runs in the main process, while each robot's control software runs in a separate Docker container.

We have adopted this design to ensure:

- **Isolation**: Each robot runs in its own environment without interfering with others
- **Flexibility**: Support for different robot types and frameworks during the same simulation run
- **Low sim-to-real gap**: The same code that runs on real robots can run in simulation

## Table of Contents

- [System Architecture](#system-architecture)
- [Docker Container Management](#docker-container-management)
- [Communication Protocol](#communication-protocol)
- [SimBridge Integration](#simbridge-integration)
- [Complete Communication Flow](#complete-communication-flow)
- [Supported Robots](#supported-robots)
- [Adding New Robots](#adding-new-robots)


## Docker Container Management

The simulator manages Docker containers through two main components:

### Container Class

**Files:** [Container.h](), [Container.cpp]()

The `Container` class handles the lifecycle of individual Docker containers using the Docker Engine API through Unix sockets.

#### Container Creation

When creating a container, the simulator configures:

**Environment Variables:**

- `ROBOT_NAME`: Unique identifier (format: `team_RobotType_number`)
- `SERVER_IP`: Simulator address (typically `172.17.0.1` for Docker bridge)
- `CIRCUS_PORT`: Communication port (default: 5555)

**Docker Configuration:**

- Volume bindings for shared data and code
- Privileged mode for real-time performance
- IPC mode set to `host` for shared memory
- Security capabilities (`SYS_NICE`, `IPC_LOCK`) for real-time scheduling

#### Docker API Communication

The `Container` class uses cURL to communicate with the Docker daemon via the Unix socket `/var/run/docker.sock`:

| Operation | HTTP Method | Endpoint | Purpose |
|-----------|-------------|----------|---------|
| Create | POST | `/containers/create?name={name}` | Create container instance |
| Start | POST | `/containers/{id}/start` | Start the container |
| Stop | POST | `/containers/{id}/stop?t=0` | Stop the container (SIGKILL after 0s) |
| Remove | DELETE | `/containers/{id}` | Delete the container |


### RobotManager Class

**File:** [RobotManager.h]()

The `RobotManager` is a singleton that orchestrates all robot containers and manages bidirectional communication between the simulator and robot frameworks.

#### Responsibilities

1. **Robot Registration**: Maintains a registry of all robots in the simulation
2. **Container Lifecycle**: Creates, starts, and manages Docker containers
3. **Network Server**: Runs a TCP server for robot-simulator communication
4. **Message Routing**: Routes messages between simulator and correct robot containers
5. **Synchronization**: Tracks when all robots are ready to begin simulation

## Communication Protocol

### Server Architecture

The `RobotManager` implements a **polling-based TCP server** for efficient multi-client handling.

#### Server Initialization

1. Creates TCP socket and binds to specified port (default: 5555)
2. Listens for incoming connections (up to number of robots)
3. Uses `poll()` to monitor multiple connections simultaneously
4. Runs in separate thread to avoid blocking simulation

#### Connection Handshake

When a Docker container starts, it initiates the connection:

**Step 1: Container Connects**

- Container creates TCP socket to `SERVER_IP:CIRCUS_PORT`
- Simulator accepts connection and adds socket to polling list

**Step 2: Robot Identification**

- Container sends first message: robot name (MessagePack serialized string)
- Simulator unpacks and validates robot name exists in registry
- Robot marked as `isConnected = true`

**Step 3: Initial State Exchange**

- Simulator packs current robot state (joint positions, velocities, IMU data)
- Sends initial state back to container
- Container is now ready to begin control loop

#### Message Exchange Loop

The continuous communication follows a **request-response pattern**:

**From Container to Simulator:**
```json
{
  "robot_name": "red_Booster-T1_1",
  "joint_torques": [0.0, 0.1, 0.2, "...", 0.0]
}
```

**From Simulator to Container:**
```json
{
  "robot_name": "red_Booster-T1_1",
  "joint_positions": [0.0, 0.1, 0.2, "...", 0.0],
  "joint_velocities": [0.0, 0.0, 0.0, "...", 0.0],
  "imu_orientation": ["w", "x", "y", "z"],
  "imu_angular_velocity": ["x", "y", "z"],
  "imu_linear_acceleration": ["x", "y", "z"]
}
```

#### Polling Mechanism

The server uses `poll()` to efficiently handle multiple connections:

```cpp
poll(fds.data(), fds.size(), timeout_ms)
```

This allows:

- Simultaneous monitoring of all robot connections
- Non-blocking detection of new connections
- Immediate response when data arrives
- Timeout to check if server should continue running

When data arrives on a socket:

1. Read and unpack MessagePack message
2. Extract `robot_name` to identify sender
3. Find corresponding `Robot` object in registry
4. Call `robot->receiveMessage()` to process commands
5. Call `robot->sendMessage()` to get current state
6. Pack and send response back to container

## SimBridge Integration

**Repository:** [SimBridge](https://github.com/SPQRTeam/simbridge.git)

SimBridge is a ROS 2 node that runs inside each Docker container, acting as the translation layer between the robot control framework and the Circus simulator.

### BridgeNode Architecture

**Files:** [bridge_node.hpp](), [bridge_node.cpp](), [main.cpp]()

#### Initialization Process

**1. Environment Setup**

The bridge reads three critical environment variables:

```cpp
ROBOT_NAME   // e.g., "red_Booster-T1_1"
SERVER_IP    // e.g., "172.17.0.1"
CIRCUS_PORT  // e.g., "5555"
```

**2. Robot Type Detection**

Extracts robot type from robot name using the format `team_TYPE_number`:

```cpp
// "red_Booster-T1_1" → "Booster-T1"
size_t firstUnderscore = robot_name.find('_');
size_t secondUnderscore = robot_name.find('_', firstUnderscore + 1);
return robot_name.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
```

**3. Joint Name Mapping**

Each robot type has a predefined joint order:

**Booster-T1** (23 joints):
```
Head_yaw, Head_pitch,
Left_Shoulder_Pitch, Left_Shoulder_Roll, Left_Elbow_Pitch, Left_Elbow_Yaw,
Right_Shoulder_Pitch, Right_Shoulder_Roll, Right_Elbow_Pitch, Right_Elbow_Yaw,
Waist,
Left_Hip_Pitch, Left_Hip_Roll, Left_Hip_Yaw, Left_Knee_Pitch, Left_Ankle_Pitch, Left_Ankle_Roll,
Right_Hip_Pitch, Right_Hip_Roll, Right_Hip_Yaw, Right_Knee_Pitch, Right_Ankle_Pitch, Right_Ankle_Roll
```

**Booster-K1** (22 joints):
```
Head_yaw, Head_pitch,
Left_Shoulder_Pitch, Left_Shoulder_Roll, Left_Elbow_Pitch, Left_Elbow_Yaw,
Right_Shoulder_Pitch, Right_Shoulder_Roll, Right_Elbow_Pitch, Right_Elbow_Yaw,
Left_Hip_Pitch, Left_Hip_Roll, Left_Hip_Yaw, Left_Knee_Pitch, Left_Ankle_Pitch, Left_Ankle_Roll,
Right_Hip_Pitch, Right_Hip_Roll, Right_Hip_Yaw, Right_Knee_Pitch, Right_Ankle_Pitch, Right_Ankle_Roll
```

**4. Socket Connection**

Establishes TCP connection to simulator:
```cpp
socket(AF_INET, SOCK_STREAM, 0)  // Create socket
connect(server_addr)              // Connect to SERVER_IP:CIRCUS_PORT
send(robot_name)                  // Send identification message
```

#### ROS 2 Topics

SimBridge creates publishers and subscribers for robot communication:

**Subscriptions** (commands from framework):

- `/joint_command` (`sensor_msgs::msg::JointState`) - Desired joint torques/efforts

**Publications** (sensor data to framework):

- `/joint_state` (`sensor_msgs::msg::JointState`) - Current joint positions and velocities
- `/imu` (`sensor_msgs::msg::Imu`) - IMU orientation, angular velocity, linear acceleration

#### Communication Flow

**Sending Commands to Simulator**:

1. Robot framework publishes desired joint commands to `/joint_command` topic
2. `jointCommandCallback()` receives the ROS message
3. Extracts joint efforts (torques) from the message
4. Creates MessagePack map: `{robot_name, joint_torques}`
5. Serializes and sends via TCP socket to simulator

```cpp
void jointCommandCallback(const sensor_msgs::msg::JointState::SharedPtr msg) {
    std::map<std::string, msgpack::object> data_map;
    data_map["robot_name"] = msgpack::object(robotName_);
    data_map["joint_torques"] = msgpack::object(msg->effort);

    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, data_map);
    send(client_fd, sbuf.data(), sbuf.size(), 0);
}
```

**Receiving State from Simulator**:

1. Periodic timer triggers `receiveAndPublish()` at fixed rate (typically 100Hz)
2. `receiveMessageFromSimulator_()` reads from TCP socket
3. Unpacks MessagePack message and validates robot name
4. Extracts joint states and IMU data from message
5. Creates ROS messages with current timestamps
6. Publishes to `/joint_state` and `/imu` topics
7. Robot framework receives sensor data and computes next control action

#### Error Handling

The bridge implements robust error handling ([bridge_node.cpp:147](../../simbridge/src/bridge_node.cpp#L147)):

- **Timeout**: Returns gracefully if no data within timeout period
- **Connection Closure**: Detects when simulator closes connection
- **Parse Errors**: Catches MessagePack deserialization failures
- **Robot Name Validation**: Ensures messages are for correct robot
- **Socket Errors**: Handles network issues gracefully

## Complete Communication Flow

[TODO] Add image

### Timing and Synchronization

- **Physics timestep**: Typically 0.001s (1000Hz) in MuJoCo
- **Communication rate**: Typically 0.01s (100Hz) between simulator and bridge
- **Control rate**: Depends on framework, typically 100Hz or higher
- **Synchronization**: Each robot must receive commands before simulation advances

## Supported Robots

Currently, Circus supports the following humanoid robots:

### Booster-T1

- **Joints**: 23 DOF
- **Features**: Includes waist joint for torso rotation
- **Joint configuration**: Full arm and leg articulation with head movement
- **Use case**: Research platform for bipedal locomotion with upper body motion

### Booster-K1

- **Joints**: 22 DOF
- **Features**: Fixed torso without waist joint
- **Joint configuration**: Simplified version of T1
- **Use case**: Locomotion-focused research without upper body complexity

Both robots share the same communication protocol but differ in joint count and names. The SimBridge automatically selects the correct joint mapping based on the robot type extracted from the robot name.

## Adding New Robots

To integrate a new robot type into Circus:

### 1. Create Robot Model

Add the robot URDF/MJCF model to the simulator's model directory.

### 2. Implement Robot Class

Create a new C++ class inheriting from the `Robot` base class:

```cpp
// include/robots/MyRobot.h
class MyRobot : public Robot {
public:
    MyRobot(const std::string& name, const std::string& type, ...);

    void receiveMessage(const std::map<std::string, msgpack::object>& data) override;
    std::map<std::string, msgpack::object> sendMessage() override;
};
```

### 3. Register in RobotFactory

Add your robot to the factory map in [RobotManager.h:324](../../include/RobotManager.h#L324):

```cpp
std::unordered_map<std::string, RobotCreator> robotFactory = {
    {"Booster-K1", ...},
    {"Booster-T1", ...},
    {"MyRobot", [](auto&& name, auto&& type, ...) {
        return std::make_shared<MyRobot>(name, type, ...);
    }}
};
```

### 4. Define Joint Mapping in SimBridge

Add joint names to [bridge_node.cpp:4](../../simbridge/src/bridge_node.cpp#L4):

```cpp
const std::map<std::string, std::vector<std::string>> BridgeNode::ROBOT_JOINTS_NAMES_MAP = {
    {"Booster-T1", {...}},
    {"Booster-K1", {...}},
    {"MyRobot", {
        "joint_1", "joint_2", "joint_3", "..."
    }}
};
```

**Important**: Joint order must match the order in your robot's URDF/MJCF model.

### 5. Build Docker Image

Create or update a Docker image containing:
- SimBridge executable
- Your robot's control framework
- All required dependencies

### 6. Update Configuration

Modify configuration files to reference your new robot type and Docker image.

### 7. Test Integration

1. Start simulator with your new robot type
2. Verify container starts successfully
3. Check TCP connection establishes
4. Confirm sensor data flows to framework
5. Verify commands affect simulated robot
