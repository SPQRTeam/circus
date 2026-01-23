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

    string simConfigName = sceneRoot["simulation_config"].as<string>();
    filesystem::path simConfigPath = filesystem::path(PROJECT_ROOT) / "resources" / "config" / "simulation_configs" / (simConfigName + ".yaml");

    if (!filesystem::exists(simConfigPath))
        throw runtime_error("Simulation config file does not exist: " + simConfigPath.string());

    YAML::Node simConfigRoot = YAML::LoadFile(simConfigPath.string());

    // Load field configuration
    if (!simConfigRoot["field"])
        throw runtime_error("Simulation config missing 'field' entry.");

    const YAML::Node& fieldNode = simConfigRoot["field"];
    scene.simulationConfig.field.name = fieldNode["name"].as<string>("fieldAdultSize");
    scene.simulationConfig.field.type = fieldNode["type"].as<string>("full");

    filesystem::path fieldPath = filesystem::path(PROJECT_ROOT) / "resources" / "includes" / "fields" / scene.simulationConfig.field.name
                                 / (scene.simulationConfig.field.name + ".yaml");
    if (!filesystem::exists(fieldPath))
        throw runtime_error("Field config file does not exist: " + fieldPath.string());

    YAML::Node fieldConfigRoot = YAML::LoadFile(fieldPath.string());

    scene.simulationConfig.field.width = fieldConfigRoot["width"].as<float>(14.0f);
    scene.simulationConfig.field.height = fieldConfigRoot["height"].as<float>(9.0f);
    scene.simulationConfig.field.center_radius = fieldConfigRoot["center_radius"].as<float>(1.5f);
    scene.simulationConfig.field.goal_area_width = fieldConfigRoot["goal_area_width"].as<float>(1.0f);
    scene.simulationConfig.field.goal_area_height = fieldConfigRoot["goal_area_height"].as<float>(4.0f);
    scene.simulationConfig.field.penalty_area_width = fieldConfigRoot["penalty_area_width"].as<float>(3.0f);
    scene.simulationConfig.field.penalty_area_height = fieldConfigRoot["penalty_area_height"].as<float>(6.5f);
    scene.simulationConfig.field.goal_width = fieldConfigRoot["goal_width"].as<float>(2.6f);
    scene.simulationConfig.field.goal_height = fieldConfigRoot["goal_height"].as<float>(1.8f);
    scene.simulationConfig.field.goal_depth = fieldConfigRoot["goal_depth"].as<float>(0.6f);
    scene.simulationConfig.field.line_width = fieldConfigRoot["line_width"].as<float>(0.08f);
    scene.simulationConfig.field.penalty_mark_distance = fieldConfigRoot["penalty_mark_distance"].as<float>(2.1f);
    scene.simulationConfig.field.ball_radius = fieldConfigRoot["ball_radius"].as<float>(0.11f);

    // Load simulation settings
    if (simConfigRoot["simulation"]) {
        const YAML::Node& simNode = simConfigRoot["simulation"];
        scene.simulationConfig.simulation.max_simulation_time = simNode["max_simulation_time"].as<int>(-1);
    }

    // Load game configuration
    if (simConfigRoot["game"]) {
        const YAML::Node& gameNode = simConfigRoot["game"];
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

    ballSpec.position = Eigen::Vector3d(0.0, 0.0, 0.12);

    const YAML::Node& teamsNode = sceneRoot["teams"];
    if (!teamsNode || teamsNode.size() > 2) {
        throw runtime_error("Scene must contain one or two teams.");
    }

    for (const auto& team : teamsNode) {
        const string& teamName = team.first.as<string>();
        const YAML::Node& robotsNode = team.second;

        if (!robotsNode.IsSequence())
            throw runtime_error("Each team must be a sequence of robots.");

        shared_ptr<Team> teamSpec = std::make_shared<Team>();
        teamSpec->name = teamName;

        uint8_t typeIndex = 0;
        for (const YAML::Node& robotNode : robotsNode) {
            if (!robotNode["type"])
                throw runtime_error("Robot missing type field.");
            if (!robotNode["number"])
                throw runtime_error("Robot missing jersey number");

            string robotType = robotNode["type"].as<string>();  // complete name <Brand>-<Model>
            uint8_t robotNumber = robotNode["number"].as<uint8_t>();
            string robotName = robotNode["name"] ? robotNode["name"].as<string>() : teamName + "_" + robotType + "_" + to_string(typeIndex++);
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

            std::tuple<int, int, int> teamColor = {255, 255, 255};  // Default to white
            if (teamName == "red") {
                teamColor = {130, 36, 51};
            } else if (teamName == "blue") {
                teamColor = {0, 103, 120};
            }

            shared_ptr<Robot> robot = RobotManager::instance().create(robotName, robotType, robotNumber, pos, ori, teamColor, teamSpec);

            robotTypes.insert(robotType);
            teamSpec->robots.push_back(std::move(robot));
        }

        scene.teams.push_back(teamSpec);
        TeamManager::instance().registerTeam(teamSpec);
    }
}

string SceneParser::buildMuJoCoXml() {
    xml_document doc;

    xml_node mujoco = doc.append_child("mujoco");

    // TODO: The simulation options can be parametrized. I don't know if we may want to change the parameters.
    xml_node option = mujoco.append_child("option");
    option.append_attribute("timestep") = "0.0001";
    option.append_attribute("iterations") = "50";
    option.append_attribute("tolerance") = "1e-10";
    option.append_attribute("solver") = "Newton";
    option.append_attribute("jacobian") = "dense";
    option.append_attribute("cone") = "pyramidal";
    // option.append_attribute("gravity") = "0 0 -9.81";

    xml_node compiler = mujoco.append_child("compiler");
    compiler.append_attribute("angle") = "radian";
    compiler.append_attribute("meshdir") = "resources/meshes/";

    xml_node include_node = mujoco.append_child("include");
    include_node.append_attribute("file") = (filesystem::path(PROJECT_ROOT) / "resources" / "includes" / "fields" / scene.simulationConfig.field.name
                                             / scene.simulationConfig.field.type / (scene.simulationConfig.field.name + ".xml"))
                                                .c_str();

    xml_node visual = mujoco.append_child("visual");
    xml_node map = visual.append_child("quality");
    // map.append_attribute("shadowsize") = "0";

    include_node = mujoco.append_child("include");
    include_node.append_attribute("file") = (filesystem::path(PROJECT_ROOT) / "resources" / "includes" / "ball.xml").c_str();

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
    light.append_attribute("ambient") = "0.6 0.6 0.6";
    light.append_attribute("diffuse") = "0.5 0.5 0.5";
    light.append_attribute("specular") = "0.5 0.5 0.5";
    light.append_attribute("pos") = "0 0 100";
    light.append_attribute("dir") = "0 0 -1";
    light.append_attribute("castshadow") = "false";

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
                    const char* geomClass = geom.attribute("class").value();
                    if (strcmp(geomClass, "visual") == 0) {
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

}  // namespace spqr
