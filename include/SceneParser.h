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

struct FieldConfig {
        std::string name;
        float width = 14.0f;
        float height = 9.0f;
        float center_radius = 1.5f;
        float goal_area_width = 1.0f;
        float goal_area_height = 4.0f;
        float penalty_area_width = 3.0f;
        float penalty_area_height = 6.5f;
        float goal_width = 2.6f;
        float goal_height = 1.8f;
        float goal_depth = 0.6f;
        float line_width = 0.08f;
        float penalty_mark_distance = 2.1f;
        float ball_radius = 0.11f;
};

struct GameConfig {
        int max_simulation_time = -1;
        int game_duration = 600;
        bool automatic_restart = true;
        int initial_phase_duration = 30;
        int ready_phase_duration = 45;
        int set_phase_duration = 10;
        int kickoff_subphase_duration = 10;
        int other_subphase_duration = 30;
        std::string first_kickoff_team = "red";
};

struct SimulationConfig {
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

        unordered_set<string> robotTypes;
        YAML::Node sceneRoot;
        SceneSpec scene;
        BallSpec ballSpec;
};

}  // namespace spqr
