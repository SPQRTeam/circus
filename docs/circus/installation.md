# Installation

Circus is a MuJoCo-based simulator for humanoid robots. The project uses [pixi](https://pixi.prefix.dev/latest/) for dependency management.

## Prerequisites

Before installing Circus, ensure you have the following:

- **Pixi**: Package manager for the project. Install from the [official website](https://pixi.prefix.dev/latest/)
- **Docker**: Required for running robot control frameworks in isolated containers

## Architecture Overview

Circus uses a distributed architecture where each robot runs inside its own Docker container, communicating with the simulator through TCP sockets. We have implemented [SimBridge](https://github.com/SPQRTeam/simbridge.git), a ROS 2-based bridge that enables communication between Docker containers and the simulator. While SimBridge is designed for Booster T1/K1 robots, it can be easily adapted for other robot types or serve as a reference for implementing custom bridges.

For a detailed explanation of the system architecture, see [Circus Architecture Overview](./architecture_overview.md).

## Installation Steps

### 1. Install Circus Simulator

Clone and install the Circus simulator:

```bash
git clone https://github.com/SPQRTeam/circus.git
cd circus
pixi install
```

### 2. Build SimBridge

SimBridge can be installed in any location on your machine:

```bash
git clone https://github.com/SPQRTeam/simbridge.git
cd simbridge
pixi install
pixi run build
```

### 3. Setup Booster Robotics SDK

Navigate to the Circus `dockerfiles` directory and clone the required SDK repositories:

```bash
cd /path/to/circus/dockerfiles
git clone https://github.com/BoosterRobotics/booster_robotics_sdk.git
git clone https://github.com/BoosterRobotics/booster_robotics_sdk_ros2.git
```

### 4. Fix ROS 2 Message Definition

There is a known issue in the `booster_robotics_sdk_ros2` repository. You need to fix the `Subtitle.msg` file:

**File:** `booster_robotics_sdk_ros2/booster_ros2_interface/msg/Subtitle.msg`

Replace the entire content with:

```
string magic_number
string text
string language
string user_id    # Indicates the source of the subtitle. If it's from the robot, it will be the fixed value "BoosterRobot". Otherwise, it represents a human and may be a random string.
int32 seq         # subtitle sequence
bool definite     # Indicates whether the subtitle is a complete sentence.
bool paragraph    # Indicates whether the subtitle is a complete paragraph.
int32 round_id
```

### 5. Build Docker Image

Build the Docker image that will run the robot control framework:

```bash
cd /path/to/circus
docker build -t spqr:booster dockerfiles/
```

This creates a Docker image named `spqr:booster` containing SimBridge and the Booster SDK.

### 6. Configure Path Constants

Create a configuration file to specify the locations of all required components:

**File:** `circus/resources/config/path_constants.yaml`

```yaml
framework: /absolute/path/to/spqrbooster2026
circus: /absolute/path/to/circus
simbridge: /absolute/path/to/simbridge
booster_robotics_sdk: /absolute/path/to/booster_robotics_sdk
```

**Important:** All paths must be absolute paths, not relative paths.

## Running the Simulator

Once all installation steps are complete, launch the simulator:

```bash
cd /path/to/circus
pixi run circus
```

The simulator will:

1. Start the MuJoCo physics engine
2. Load the robot models and scene
3. Create and start Docker containers for each robot
4. Establish TCP connections between containers and the simulator
5. Begin the simulation loop

## Troubleshooting

### Docker Connection Issues

If robots fail to connect, verify:
- Docker daemon is running: `docker ps`
- Port 5555 is available: `netstat -an | grep 5555`
- Docker image was built successfully: `docker images | grep spqr`

### Path Configuration Issues

If the simulator can't find required files:
- Verify all paths in `path_constants.yaml` are absolute
- Check that directories exist and are readable
- Ensure the `framework` path points to your robot control code

### SimBridge Build Issues

If SimBridge fails to build:
- Ensure all pixi dependencies are installed: `pixi install`
- Check ROS 2 environment is properly configured
- Verify the `Subtitle.msg` fix was applied correctly

## Next Steps

- Read the [Architecture Overview](./architecture_overview.md) to understand how Circus works
- Explore the configuration files in `resources/config/` to customize your simulation
- Check the examples to see how to create multi-robot scenarios