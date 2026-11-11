#include "SceneParser.h"

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stack>
#include <stdexcept>

#include "FieldGenerator.h"
#include "RobotManager.h"
#include "Team.h"
#include "robots/Robot.h"

using namespace pugi;
using namespace std;
namespace spqr {

SceneParser::SceneParser(const string& yamlPath) {
    sceneRoot = YAML::LoadFile(yamlPath);

    // Load simulation config
    if (!sceneRoot["simulation_config"])
        throw runtime_error("Scene missing 'simulation_config' entry.");

    // Simulation configs are owned by maximus (src/app/sim/simulation_configs). Resolve them from
    // the framework tree (FRAMEWORK_PATH / PIXI_PROJECT_ROOT) so circus and maximus stay in sync,
    // falling back to circus' own resources for standalone use.
    string simConfigName = sceneRoot["simulation_config"].as<string>();
    const string simConfigFileName = simConfigName + ".yaml";
    filesystem::path simConfigPath;
    const char* fwkRoot = std::getenv("FRAMEWORK_PATH") ? std::getenv("FRAMEWORK_PATH") : std::getenv("PIXI_PROJECT_ROOT");
    if (fwkRoot) {
        filesystem::path maximusSimConfigPath = filesystem::path(fwkRoot) / "src" / "app" / "sim" / "simulation_configs" / simConfigFileName;
        if (filesystem::exists(maximusSimConfigPath))
            simConfigPath = maximusSimConfigPath;
    }
    if (simConfigPath.empty())
        simConfigPath = filesystem::path(PROJECT_ROOT) / "resources" / "config" / "simulation_configs" / simConfigFileName;

    if (!filesystem::exists(simConfigPath))
        throw runtime_error("Simulation config file does not exist: " + simConfigPath.string());

    YAML::Node simConfigRoot = YAML::LoadFile(simConfigPath.string());

    // Load simulation settings
    if (simConfigRoot["simulation"]) {
        const YAML::Node& simNode = simConfigRoot["simulation"];
        scene.simulationConfig.simulation.max_simulation_time = simNode["max_simulation_time"].as<int>(-1);
    }

    // Load game configuration
    if (simConfigRoot["game"]) {
        const YAML::Node& gameNode = simConfigRoot["game"];
        scene.simulationConfig.game.field = gameNode["field"].as<string>("fieldAdultSize");
        scene.simulationConfig.game.game_state_logging = gameNode["game_state_logging"].as<bool>(true);
        scene.simulationConfig.game.game_state_logging_path = gameNode["game_state_logging_path"].as<string>("game_state.log");
        scene.simulationConfig.game.game_state_logging_interval = gameNode["game_state_logging_interval"].as<float>(1.0f);
        scene.simulationConfig.game.game_duration = gameNode["game_duration"].as<int>(600);
        scene.simulationConfig.game.automatic_restart = gameNode["automatic_restart"].as<bool>(true);
        scene.simulationConfig.game.initial_phase_duration = gameNode["initial_phase_duration"].as<int>(30);
        scene.simulationConfig.game.ready_phase_duration = gameNode["ready_phase_duration"].as<int>(45);
        scene.simulationConfig.game.set_phase_duration = gameNode["set_phase_duration"].as<int>(10);
        scene.simulationConfig.game.kickoff_subphase_duration = gameNode["kickoff_subphase_duration"].as<int>(10);
        scene.simulationConfig.game.other_subphase_duration = gameNode["other_subphase_duration"].as<int>(30);
        scene.simulationConfig.game.first_kickoff_team = gameNode["first_kickoff_team"].as<string>("red");
        scene.simulationConfig.game.penalty_duration = gameNode["penalty_duration"].as<int>(45);
    }

    // A general environment has no field: only soccer scenes need a field config.
    if (!sceneRoot["environment"]) {
        // Field configs are owned by maximus (src/app/config/fields). Resolve them from the
        // framework tree (FRAMEWORK_PATH / PIXI_PROJECT_ROOT) so circus and maximus stay in sync,
        // falling back to circus' own resources for standalone use.
        const string fieldFileName = scene.simulationConfig.game.field + ".yaml";
        filesystem::path fieldPath;
        if (fwkRoot) {
            filesystem::path maximusFieldPath = filesystem::path(fwkRoot) / "src" / "app" / "config" / "fields" / fieldFileName;
            if (filesystem::exists(maximusFieldPath))
                fieldPath = maximusFieldPath;
        }
        if (fieldPath.empty())
            fieldPath = filesystem::path(PROJECT_ROOT) / "resources" / "config" / "fields" / fieldFileName;
        if (!filesystem::exists(fieldPath))
            throw runtime_error("Field config file does not exist: " + fieldPath.string());

        YAML::Node fieldConfigRoot = YAML::LoadFile(fieldPath.string());

        scene.simulationConfig.field.width = fieldConfigRoot["width"].as<float>();
        scene.simulationConfig.field.height = fieldConfigRoot["height"].as<float>();
        scene.simulationConfig.field.center_radius = fieldConfigRoot["centerRadius"].as<float>();
        scene.simulationConfig.field.goal_area_width = fieldConfigRoot["goalAreaWidth"].as<float>();
        scene.simulationConfig.field.goal_area_height = fieldConfigRoot["goalAreaHeight"].as<float>();
        scene.simulationConfig.field.penalty_area_width = fieldConfigRoot["penaltyAreaWidth"].as<float>();
        scene.simulationConfig.field.penalty_area_height = fieldConfigRoot["penaltyAreaHeight"].as<float>();
        scene.simulationConfig.field.penalty_mark_distance = fieldConfigRoot["penaltyMarkDistance"].as<float>();
        scene.simulationConfig.field.goal_width = fieldConfigRoot["goalWidth"].as<float>();
        scene.simulationConfig.field.goal_height = fieldConfigRoot["goalHeight"].as<float>();
        scene.simulationConfig.field.goal_depth = fieldConfigRoot["goalDepth"].as<float>();
        scene.simulationConfig.field.line_width = fieldConfigRoot["lineWidth"].as<float>();
        scene.simulationConfig.field.ball_radius = fieldConfigRoot["ballRadius"].as<float>();
    }

    ballSpec.position = Eigen::Vector3d(0.0, 0.0, 0.12);

    const YAML::Node& teamsNode = sceneRoot["teams"];
    if (!teamsNode || teamsNode.size() > 2) {
        throw runtime_error("Scene must contain one or two teams.");
    }

    for (const auto& team : teamsNode) {
        const string& teamName = team.first.as<string>();
        const YAML::Node& theTeamNode = team.second;

        const int& teamNumber = theTeamNode["number"].as<int>();
        const YAML::Node& robotsNode = theTeamNode["players"];

        if (!robotsNode.IsSequence())
            throw runtime_error("Each team must be a sequence of robots.");

        shared_ptr<Team> teamSpec = std::make_shared<Team>(teamName, teamNumber);

        uint8_t typeIndex = 0;
        for (const YAML::Node& robotNode : robotsNode) {
            if (!robotNode["type"])
                throw runtime_error("Robot missing type field.");
            if (!robotNode["number"])
                throw runtime_error("Robot missing jersey number");

            string robotType = robotNode["type"].as<string>();  // complete name <Brand>-<Model>
            uint8_t robotNumber = robotNode["number"].as<uint8_t>();
            string robotName = robotNode["name"] ? robotNode["name"].as<string>() : teamName + "_" + robotType + "_" + to_string(typeIndex++);
            // Role is taken from the scene if present; otherwise fall back to a number-based default.
            string robotRole = robotNode["role"]  ? robotNode["role"].as<string>() :
                               (robotNumber == 1) ? "Goalkeeper" :
                               (robotNumber == 2) ? "Defender" :
                                                    "Striker";
            Vector3d pos = Vector3d::Zero();
            Vector3d ori = Vector3d::Zero();

            if (robotNode["position"]) {
                for (int i = 0; i < 3; ++i)
                    pos[i] = robotNode["position"][i].as<double>();
            }

            if (robotNode["orientation"]) {
                for (int i = 0; i < 3; ++i)
                    ori[i] = robotNode["orientation"][i].as<double>();
            }

            shared_ptr<Robot> robot = RobotManager::instance().create(robotName, robotType, robotNumber, pos, ori, teamName, teamSpec, robotRole);

            robotTypes.insert(robotType);
            teamSpec->robots.push_back(std::move(robot));
        }

        scene.teams.push_back(teamSpec);
        TeamManager::instance().registerTeam(teamSpec);
    }

    // A general environment replaces the soccer field (and the ball)
    if (sceneRoot["environment"])
        parseEnvironment(sceneRoot["environment"]);

    // Parse obstacles if present
    if (sceneRoot["obstacles"]) {
        const YAML::Node& obstaclesNode = sceneRoot["obstacles"];
        if (obstaclesNode.IsSequence()) {
            uint8_t obstacleIndex = 0;
            for (const YAML::Node& obstacleNode : obstaclesNode) {
                if (!obstacleNode["type"]) {
                    throw runtime_error("Obstacle missing 'type' field.");
                }

                ObstacleSpec obstacle;
                obstacle.type = obstacleNode["type"].as<string>();
                obstacle.name = obstacleNode["name"] ? obstacleNode["name"].as<string>() : ("obstacle_" + to_string(obstacleIndex));

                // Parse position [x, y, z]
                if (obstacleNode["position"]) {
                    obstacle.position = Vector3d::Zero();
                    for (int i = 0; i < 3; ++i) {
                        obstacle.position[i] = obstacleNode["position"][i].as<double>();
                    }
                }

                // Parse orientation [roll, pitch, yaw] in radians
                if (obstacleNode["orientation"]) {
                    obstacle.orientation = Vector3d::Zero();
                    for (int i = 0; i < 3; ++i) {
                        obstacle.orientation[i] = obstacleNode["orientation"][i].as<double>();
                    }
                }

                // Parse size (meaning depends on shape type)
                if (obstacleNode["size"]) {
                    obstacle.size = Vector3d::Zero();
                    for (size_t i = 0; i < obstacleNode["size"].size(); ++i) {
                        if (i < 3)
                            obstacle.size[i] = obstacleNode["size"][i].as<double>();
                    }
                }

                scene.obstacles.push_back(obstacle);
                obstacleIndex++;
            }
        }
    }
}

string SceneParser::buildMuJoCoXml() {
    xml_document doc;

    xml_node mujoco = doc.append_child("mujoco");

    // TODO: The simulation options can be parametrized. I don't know if we may want to change the parameters.
    xml_node option = mujoco.append_child("option");
    option.append_attribute("timestep") = "0.002";
    option.append_attribute("iterations") = "50";
    option.append_attribute("tolerance") = "1e-10";
    option.append_attribute("solver") = "Newton";
    option.append_attribute("jacobian") = "dense";
    option.append_attribute("cone") = "pyramidal";
    // option.append_attribute("gravity") = "0 0 -9.81";

    xml_node compiler = mujoco.append_child("compiler");
    compiler.append_attribute("angle") = "radian";
    compiler.append_attribute("meshdir") = "resources/meshes/";

    if (scene.environment.enabled) {
        // General environment: checkerboard floor and static mesh objects, no field and no ball
        addEnvironmentToMuJoCo(mujoco);
    } else {
        // Generate field dynamically using FieldGenerator (includes ball)
        std::string meshDir = (filesystem::path(PROJECT_ROOT) / "resources" / "meshes").string();
        FieldGenerator::appendFieldToMuJoCo(mujoco, scene.simulationConfig.field, meshDir);
    }

    xml_node visual = mujoco.append_child("visual");
    xml_node map = visual.append_child("quality");
    // map.append_attribute("shadowsize") = "0";

    for (const string& robotType : robotTypes)
        buildRobotCommon(robotType, mujoco);

    xml_node asset = mujoco.append_child("asset");

    // TODO: This could be parametrized as well. Kinda useless.
    xml_node texture = asset.append_child("texture");
    texture.append_attribute("type") = "skybox";
    texture.append_attribute("builtin") = "gradient";
    texture.append_attribute("rgb1") = "0.3 0.5 0.7";
    texture.append_attribute("rgb2") = "0 0 0";
    texture.append_attribute("width") = "512";
    texture.append_attribute("height") = "512";

    xml_node worldbody = mujoco.append_child("worldbody");
    xml_node actuator = mujoco.append_child("actuator");
    xml_node sensor = mujoco.append_child("sensor");

    xml_node light = worldbody.append_child("light");
    light.append_attribute("ambient") = "0.95 0.95 0.95";
    light.append_attribute("diffuse") = "0 0 0";
    light.append_attribute("specular") = "0 0 0";
    light.append_attribute("pos") = "0 0 50";
    light.append_attribute("dir") = "0 0 -1";
    light.append_attribute("castshadow") = "false";

    // Add obstacles to the scene
    addObstaclesToMuJoCo(worldbody);

    for (const shared_ptr<Team>& team : scene.teams) {
        for (const shared_ptr<Robot>& robot : team->robots) {
            buildRobotInstance(robot, worldbody, actuator, sensor);
        }
    }

    ostringstream oss;
    doc.save(oss, "  ");

    return oss.str();
}

void SceneParser::buildRobotCommon(const string& robotType, xml_node& mujoco) {
    filesystem::path commonPath = filesystem::path(PROJECT_ROOT) / "resources" / "robots" / robotType / "common.xml";
    if (!filesystem::exists(commonPath)) {
        throw runtime_error("Robot common file does not exist: " + commonPath.string());
    }
    xml_node include_node = mujoco.append_child("include");
    include_node.append_attribute("file") = commonPath.c_str();
}

void SceneParser::prefixSubtree(xml_node& root, const string& robotName) {
    // DFS traversal of the tree, appending the prefix robotName to all names
    stack<xml_node> stack;
    stack.push(root);

    while (!stack.empty()) {
        xml_node node = stack.top();
        stack.pop();

        for (xml_attribute attr : node.attributes()) {
            string current_attr(attr.name());
            // Not the cleanest solution, but tracking all the changed names would require O(n²). I hope this
            // heuristic is general enough.
            if (current_attr == "name" || current_attr == "joint" || current_attr == "objname" || current_attr == "site") {
                string original = attr.value();
                if (original.rfind(robotName, 0) != 0) {
                    attr.set_value((robotName + "_" + original).c_str());
                }
            }
        }

        for (xml_node child = node.last_child(); child; child = child.previous_sibling()) {
            stack.push(child);
        }
    }
}

void SceneParser::buildRobotInstance(const shared_ptr<Robot>& robotSpec, xml_node& worldbody, xml_node& actuator, xml_node& sensor) {
    filesystem::path instancePath = filesystem::path(PROJECT_ROOT) / "resources" / "robots" / robotSpec->type / "instance.xml";

    if (!filesystem::exists(instancePath)) {
        throw runtime_error("Robot instance file does not exist: " + instancePath.string());
    }

    xml_document instanceModel;
    if (!instanceModel.load_file(instancePath.c_str())) {
        throw runtime_error("Failed to load robot instance XML: " + instancePath.string());
    }

    xml_node mujoco = instanceModel.child("mujoco");

    xml_node worldbodyModel = mujoco.child("worldbody");
    xml_node sensorModel = mujoco.child("sensor");
    xml_node actuatorModel = mujoco.child("actuator");

    if (!worldbodyModel)
        throw runtime_error("Missing <worldbody> node in <mujoco>.");

    if (!sensorModel)
        throw runtime_error("Missing <sensor> node in <mujoco>.");

    if (!actuatorModel)
        throw runtime_error("Missing <actuator> node in <mujoco>.");

    if (distance(worldbodyModel.begin(), worldbodyModel.end()) != 1)
        throw runtime_error("<worldbody> must have exactly one direct child.");

    xml_node robotNode = *worldbodyModel.begin();

    std::ostringstream posStream;
    posStream << robotSpec->initPosition.x() << " " << robotSpec->initPosition.y() << " " << robotSpec->initPosition.z();
    xml_attribute posAttr = robotNode.attribute("pos");
    if (posAttr) {
        posAttr.set_value(posStream.str().c_str());
    } else {
        robotNode.append_attribute("pos") = posStream.str().c_str();
    }

    std::ostringstream oriStream;
    oriStream << robotSpec->initOrientation.x() << " " << robotSpec->initOrientation.y() << " " << robotSpec->initOrientation.z();
    xml_attribute eulerAttr = robotNode.attribute("euler");
    if (eulerAttr) {
        eulerAttr.set_value(oriStream.str().c_str());
    } else {
        robotNode.append_attribute("euler") = oriStream.str().c_str();
    }

    prefixSubtree(worldbodyModel, robotSpec->name);
    prefixSubtree(sensorModel, robotSpec->name);
    prefixSubtree(actuatorModel, robotSpec->name);

    // Set the Trunk body color based on robot's team color
    // Use a stack-based traversal to find all body nodes recursively
    std::stack<xml_node> nodeStack;
    for (xml_node child : worldbodyModel.children()) {
        nodeStack.push(child);
    }

    std::string expectedTrunkName = robotSpec->name + "_Trunk";
    bool foundTrunk = false;

    while (!nodeStack.empty() && !foundTrunk) {
        xml_node current = nodeStack.top();
        nodeStack.pop();

        if (strcmp(current.name(), "body") == 0) {
            const char* bodyName = current.attribute("name").value();
            if (strcmp(bodyName, expectedTrunkName.c_str()) == 0) {
                // Found the Trunk body, now find its visual geometry (class="visual")
                for (xml_node geom : current.children("geom")) {
                    // Robots whose common.xml prefixes its classes (e.g. "Booster-T2_visual") match too
                    const std::string geomClass = geom.attribute("class").value();
                    const std::string visualSuffix = "_visual";
                    if (geomClass == "visual" || (geomClass.size() > visualSuffix.size() && geomClass.ends_with(visualSuffix))) {
                        // Set the rgba attribute for this visual geometry
                        auto [r, g, b] = robotSpec->color;
                        std::ostringstream rgbaStream;
                        rgbaStream << (r / 255.0) << " " << (g / 255.0) << " " << (b / 255.0) << " 1";

                        xml_attribute rgbaAttr = geom.attribute("rgba");
                        if (rgbaAttr) {
                            rgbaAttr.set_value(rgbaStream.str().c_str());
                        } else {
                            geom.append_attribute("rgba") = rgbaStream.str().c_str();
                        }
                    }
                }

                // Hide number geometries that don't match the robot's number
                // Also assign all number geoms to group 4 so they can be hidden from robot cameras
                std::string correctNumName = robotSpec->name + "_num_" + std::to_string(robotSpec->number);
                for (xml_node geom : current.children("geom")) {
                    std::string geomName = geom.attribute("name").value();
                    // Check if this is a number geom (starts with robotName_num_)
                    std::string numPrefix = robotSpec->name + "_num_";
                    if (geomName.rfind(numPrefix, 0) == 0) {
                        // Set group 4 for all number geoms (invisible to robot cameras)
                        xml_attribute groupAttr = geom.attribute("group");
                        if (groupAttr) {
                            groupAttr.set_value("4");
                        } else {
                            geom.append_attribute("group") = "4";
                        }

                        if (geomName != correctNumName) {
                            // Hide this number by setting alpha to 0
                            xml_attribute rgbaAttr = geom.attribute("rgba");
                            if (rgbaAttr) {
                                rgbaAttr.set_value("1 1 1 0");
                            } else {
                                geom.append_attribute("rgba") = "1 1 1 0";
                            }
                        }
                    }
                }
                foundTrunk = true;
            }
        }

        // Add children to stack for traversal
        if (!foundTrunk) {
            for (xml_node child : current.children()) {
                nodeStack.push(child);
            }
        }
    }

    for (xml_node child : worldbodyModel.children()) {
        worldbody.append_copy(child);
    }

    for (xml_node child : sensorModel.children()) {
        sensor.append_copy(child);
    }

    for (xml_node child : actuatorModel.children()) {
        actuator.append_copy(child);
    }
}

void SceneParser::addObstaclesToMuJoCo(xml_node& worldbody) {
    for (const auto& obstacle : scene.obstacles) {
        xml_node body = worldbody.append_child("body");
        body.append_attribute("name") = obstacle.name.c_str();

        std::ostringstream posStream;
        posStream << obstacle.position.x() << " " << obstacle.position.y() << " " << obstacle.position.z();
        body.append_attribute("pos") = posStream.str().c_str();

        std::ostringstream eulerStream;
        eulerStream << obstacle.orientation.x() << " " << obstacle.orientation.y() << " " << obstacle.orientation.z();
        body.append_attribute("euler") = eulerStream.str().c_str();

        xml_node geom = body.append_child("geom");
        geom.append_attribute("name") = (obstacle.name + "_geom").c_str();
        geom.append_attribute("type") = obstacle.type.c_str();
        geom.append_attribute("mass") = "0";  // Static obstacle

        // Set dimensions based on type
        if (obstacle.type == "box") {
            std::ostringstream sizeStream;
            sizeStream << obstacle.size.x() / 2.0 << " " << obstacle.size.y() / 2.0 << " " << obstacle.size.z() / 2.0;
            geom.append_attribute("size") = sizeStream.str().c_str();
            geom.append_attribute("rgba") = "0.5 0.5 0.5 1.0";
        } else if (obstacle.type == "sphere") {
            std::ostringstream sizeStream;
            sizeStream << obstacle.size.x();
            geom.append_attribute("size") = sizeStream.str().c_str();
            geom.append_attribute("rgba") = "0.5 0.5 0.5 1.0";
        } else if (obstacle.type == "cylinder") {
            // For cylinder: size[0] = radius, size[1] = height
            std::ostringstream sizeStream;
            sizeStream << obstacle.size.x() << " " << obstacle.size.y() / 2.0;
            geom.append_attribute("size") = sizeStream.str().c_str();
            geom.append_attribute("rgba") = "0.5 0.5 0.5 1.0";
        } else if (obstacle.type == "capsule") {
            // For capsule: size[0] = radius, size[1] = height
            std::ostringstream sizeStream;
            sizeStream << obstacle.size.x() << " " << obstacle.size.y() / 2.0;
            geom.append_attribute("size") = sizeStream.str().c_str();
            geom.append_attribute("rgba") = "0.5 0.5 0.5 1.0";
        }
    }
}

namespace {

Vector3d readVector3(const YAML::Node& node, const Vector3d& fallback) {
    if (!node)
        return fallback;
    if (!node.IsSequence() || node.size() != 3)
        throw runtime_error("Expected a sequence of 3 numbers.");
    return Vector3d(node[0].as<double>(), node[1].as<double>(), node[2].as<double>());
}

string joinNumbers(std::initializer_list<double> values) {
    ostringstream oss;
    bool first = true;
    for (double v : values) {
        if (!first)
            oss << " ";
        oss << v;
        first = false;
    }
    return oss.str();
}

}  // namespace

void SceneParser::parseEnvironment(const YAML::Node& environmentNode) {
    EnvironmentSpec& env = scene.environment;
    env.enabled = true;

    if (const YAML::Node& floorNode = environmentNode["floor"]) {
        if (floorNode["size"]) {
            if (!floorNode["size"].IsSequence() || floorNode["size"].size() != 2)
                throw runtime_error("environment.floor.size must be [width, length].");
            env.floorSize = Vector2d(floorNode["size"][0].as<double>(), floorNode["size"][1].as<double>());
        }
        env.squareSize = floorNode["square"].as<double>(env.squareSize);
        env.rgb1 = readVector3(floorNode["rgb1"], env.rgb1);
        env.rgb2 = readVector3(floorNode["rgb2"], env.rgb2);
        if (env.squareSize <= 0.0 || env.floorSize.x() <= 0.0 || env.floorSize.y() <= 0.0)
            throw runtime_error("environment.floor size and square must be positive.");
    }

    if (const YAML::Node& objectsNode = environmentNode["objects"]) {
        if (!objectsNode.IsSequence())
            throw runtime_error("environment.objects must be a sequence.");
        int index = 0;
        for (const YAML::Node& objectNode : objectsNode) {
            if (!objectNode["mesh"])
                throw runtime_error("environment object missing 'mesh' field.");
            EnvironmentObjectSpec object;
            object.name = objectNode["name"] ? objectNode["name"].as<string>() : ("object_" + to_string(index));
            object.mesh = resolveMeshPath(objectNode["mesh"].as<string>());
            object.position = readVector3(objectNode["position"], object.position);
            object.orientation = readVector3(objectNode["orientation"], object.orientation);
            if (objectNode["scale"] && objectNode["scale"].IsScalar()) {
                object.scale = Vector3d::Constant(objectNode["scale"].as<double>());
            } else {
                object.scale = readVector3(objectNode["scale"], object.scale);
            }
            if (const YAML::Node& rgbaNode = objectNode["rgba"]) {
                if (!rgbaNode.IsSequence() || rgbaNode.size() != 4)
                    throw runtime_error("environment object rgba must be [r, g, b, a].");
                object.rgba = Vector4d(rgbaNode[0].as<double>(), rgbaNode[1].as<double>(), rgbaNode[2].as<double>(), rgbaNode[3].as<double>());
            }
            object.collision = objectNode["collision"].as<bool>(true);
            env.objects.push_back(object);
            ++index;
        }
    }
}

// Mesh paths are looked up, in order: as given if absolute; in the framework's sim meshes
// (FRAMEWORK_PATH / PIXI_PROJECT_ROOT + src/app/sim/meshes), so scenes and their assets can live in
// maximus; in circus' own resources/meshes.
string SceneParser::resolveMeshPath(const string& mesh) {
    const filesystem::path meshPath(mesh);
    if (meshPath.is_absolute()) {
        if (!filesystem::exists(meshPath))
            throw runtime_error("Mesh file does not exist: " + mesh);
        return meshPath.string();
    }

    const char* fwkRoot = std::getenv("FRAMEWORK_PATH") ? std::getenv("FRAMEWORK_PATH") : std::getenv("PIXI_PROJECT_ROOT");
    if (fwkRoot) {
        const filesystem::path frameworkMesh = filesystem::path(fwkRoot) / "src" / "app" / "sim" / "meshes" / meshPath;
        if (filesystem::exists(frameworkMesh))
            return frameworkMesh.string();
    }

    const filesystem::path circusMesh = filesystem::path(PROJECT_ROOT) / "resources" / "meshes" / meshPath;
    if (filesystem::exists(circusMesh))
        return circusMesh.string();

    throw runtime_error("Mesh file not found: " + mesh + " (looked in the framework's src/app/sim/meshes and circus' resources/meshes)");
}

void SceneParser::addEnvironmentToMuJoCo(xml_node& mujoco) {
    const EnvironmentSpec& env = scene.environment;

    xml_node asset = mujoco.append_child("asset");

    // One period of MuJoCo's builtin checker texture holds 2x2 squares.
    xml_node floorTex = asset.append_child("texture");
    floorTex.append_attribute("name") = "env_floor_tex";
    floorTex.append_attribute("type") = "2d";
    floorTex.append_attribute("builtin") = "checker";
    floorTex.append_attribute("rgb1") = joinNumbers({env.rgb1.x(), env.rgb1.y(), env.rgb1.z()}).c_str();
    floorTex.append_attribute("rgb2") = joinNumbers({env.rgb2.x(), env.rgb2.y(), env.rgb2.z()}).c_str();
    floorTex.append_attribute("width") = "512";
    floorTex.append_attribute("height") = "512";

    xml_node floorMat = asset.append_child("material");
    floorMat.append_attribute("name") = "env_floor_mat";
    floorMat.append_attribute("texture") = "env_floor_tex";
    floorMat.append_attribute("texrepeat") = joinNumbers({env.floorSize.x() / (2.0 * env.squareSize), env.floorSize.y() / (2.0 * env.squareSize)}).c_str();
    floorMat.append_attribute("reflectance") = "0.1";

    for (const EnvironmentObjectSpec& object : env.objects) {
        xml_node meshAsset = asset.append_child("mesh");
        meshAsset.append_attribute("name") = (object.name + "_mesh").c_str();
        meshAsset.append_attribute("file") = object.mesh.c_str();
        meshAsset.append_attribute("scale") = joinNumbers({object.scale.x(), object.scale.y(), object.scale.z()}).c_str();
    }

    xml_node worldbody = mujoco.append_child("worldbody");

    xml_node floor = worldbody.append_child("geom");
    floor.append_attribute("name") = "env_floor";
    floor.append_attribute("type") = "plane";
    floor.append_attribute("size") = joinNumbers({env.floorSize.x() / 2.0, env.floorSize.y() / 2.0, 0.05}).c_str();
    floor.append_attribute("material") = "env_floor_mat";
    floor.append_attribute("friction") = "1 0.005 0.0001";

    // Objects are bodies without joints, i.e. welded to the world.
    for (const EnvironmentObjectSpec& object : env.objects) {
        xml_node body = worldbody.append_child("body");
        body.append_attribute("name") = object.name.c_str();
        body.append_attribute("pos") = joinNumbers({object.position.x(), object.position.y(), object.position.z()}).c_str();
        body.append_attribute("euler") = joinNumbers({object.orientation.x(), object.orientation.y(), object.orientation.z()}).c_str();

        xml_node geom = body.append_child("geom");
        geom.append_attribute("name") = (object.name + "_geom").c_str();
        geom.append_attribute("type") = "mesh";
        geom.append_attribute("mesh") = (object.name + "_mesh").c_str();
        geom.append_attribute("rgba") = joinNumbers({object.rgba[0], object.rgba[1], object.rgba[2], object.rgba[3]}).c_str();
        if (!object.collision) {
            geom.append_attribute("contype") = "0";
            geom.append_attribute("conaffinity") = "0";
        }
    }
}

}  // namespace spqr
