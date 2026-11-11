#pragma once

#include <yaml-cpp/yaml.h>

#include <Eigen/Eigen>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <unordered_set>

#include "Team.h"
#include "robots/Robot.h"

using namespace pugi;
using namespace std;
using namespace Eigen;

namespace spqr {

struct BallSpec {
        Vector3d position;
};

struct ObstacleSpec {
        std::string type;       // "box", "cylinder", "sphere", etc.
        Vector3d position;      // [x, y, z]
        Vector3d orientation;   // [roll, pitch, yaw] in radians
        Vector3d size;          // Dimensions depending on type
        std::string name = "";  // Optional name for the obstacle
};

// A static object placed in a general (non-soccer) environment, loaded from a mesh file.
struct EnvironmentObjectSpec {
        std::string name;
        std::string mesh;                                  // path to an .obj/.stl, see SceneParser::resolveMeshPath
        Vector3d position = Vector3d::Zero();              // [x, y, z]
        Vector3d orientation = Vector3d::Zero();           // [roll, pitch, yaw] in radians
        Vector3d scale = Vector3d::Ones();                 // per-axis mesh scale
        Vector4d rgba = Vector4d(0.55, 0.38, 0.22, 1.0);   // default: wood brown
        bool collision = true;                             // false = visual only
};

// A general environment that replaces the soccer field: a checkerboard floor plus static mesh objects.
struct EnvironmentSpec {
        bool enabled = false;
        Vector2d floorSize = Vector2d(10.0, 10.0);  // [m] visible floor extent (collision plane is infinite)
        double squareSize = 0.5;                    // [m] side of one checkerboard square
        Vector3d rgb1 = Vector3d(0.85, 0.85, 0.85);
        Vector3d rgb2 = Vector3d(0.15, 0.15, 0.15);
        std::vector<EnvironmentObjectSpec> objects;
};

struct FieldConfig {
        float width = 14.0f;
        float height = 9.0f;
        float center_radius = 1.5f;
        float goal_area_width = 1.0f;
        float goal_area_height = 4.0f;
        float penalty_area_width = 3.0f;
        float penalty_area_height = 6.5f;
        float penalty_mark_distance = 2.1f;
        float goal_width = 2.6f;
        float goal_height = 1.8f;
        float goal_depth = 0.6f;
        float line_width = 0.08f;
        float ball_radius = 0.11f;
};

struct SimulationSettings {
        int max_simulation_time = -1;  // -1 means no limit
};

struct GameConfig {
        std::string field = "fieldAdultSize";
        bool game_state_logging = true;
        std::string game_state_logging_path = "game_state.log";
        float game_state_logging_interval = 1.0f;
        int game_duration = 600;
        bool automatic_restart = true;
        int initial_phase_duration = 30;
        int ready_phase_duration = 45;
        int set_phase_duration = 10;
        int kickoff_subphase_duration = 10;
        int other_subphase_duration = 30;
        std::string first_kickoff_team = "red";
        int penalty_duration = 45;
};

struct SimulationConfig {
        SimulationSettings simulation;
        FieldConfig field;
        GameConfig game;
};

struct CellData {
        int row;
        int column;
        string stream;
};

struct GuiConfig {
        int rows = 1;
        int columns = 4;
        std::vector<CellData> cellData;
};

struct SceneSpec {
        SimulationConfig simulationConfig;
        std::vector<std::shared_ptr<Team>> teams;
        std::vector<ObstacleSpec> obstacles;
        EnvironmentSpec environment;
        GuiConfig guiConfig;
};

class SceneParser {
    public:
        SceneParser(const string& yamlPath);
        string buildMuJoCoXml();
        const SceneSpec& getSceneInfo() const {
            return scene;
        }

    private:
        void buildRobotCommon(const string& robotType, xml_node& mujoco);
        void buildRobotInstance(const shared_ptr<Robot>& robotSpec, xml_node& worldbody, xml_node& actuator, xml_node& sensor);
        void prefixSubtree(xml_node& root, const std::string& robotName);
        void addObstaclesToMuJoCo(xml_node& worldbody);
        void parseEnvironment(const YAML::Node& environmentNode);
        void addEnvironmentToMuJoCo(xml_node& mujoco);
        static string resolveMeshPath(const string& mesh);

        unordered_set<string> robotTypes;
        YAML::Node sceneRoot;
        SceneSpec scene;
        BallSpec ballSpec;
};

}  // namespace spqr
