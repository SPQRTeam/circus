#pragma once

#include <pugixml.hpp>
#include <string>
#include <vector>

namespace spqr {

struct FieldConfig;

struct LineSegment {
    float x1, y1, z1;  // Start point
    float x2, y2, z2;  // End point
};

class FieldGenerator {
public:
    /**
     * Generate MuJoCo XML for a field based on field configuration
     * @param fieldConfig Field dimensions and specifications
     * @param meshDir Base directory (unused, kept for API compatibility)
     * @return XML string containing the complete field definition
     */
    static std::string generateFieldXML(const FieldConfig& fieldConfig, const std::string& meshDir);

    /**
     * Generate field XML and append it to an existing mujoco node
     * @param mujocoNode Parent mujoco XML node
     * @param fieldConfig Field dimensions and specifications
     * @param meshDir Base directory (unused, kept for API compatibility)
     */
    static void appendFieldToMuJoCo(pugi::xml_node& mujocoNode, const FieldConfig& fieldConfig, const std::string& meshDir);

private:
    /**
     * Add field assets (textures, materials, goal meshes) to the asset node
     */
    static void addFieldAssets(pugi::xml_node& assetNode, const FieldConfig& fieldConfig, const std::string& meshDir);

    /**
     * Add field geometries (ground, lines, goals) to the worldbody node
     */
    static void addFieldGeometries(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig);

    /**
     * Add the ground plane geom
     */
    static void addGroundPlane(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig);

    /**
     * Calculate all line segments that make up the field lines based on field configuration
     * This includes: boundary lines, halfway line, center circle, goal areas, penalty areas
     */
    static std::vector<LineSegment> calculateFieldLines(const FieldConfig& fieldConfig);

    /**
     * Add field lines as individual box/capsule geoms
     * Lines are generated procedurally based on field dimensions
     */
    static void addFieldLines(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig);

    /**
     * Add a single line segment as a box or capsule geom
     * @param worldbodyNode Parent worldbody node
     * @param name Geom name
     * @param segment Line segment definition
     * @param width Line width
     */
    static void addLineSegment(pugi::xml_node& worldbodyNode, const std::string& name,
                               const LineSegment& segment, float width);

    /**
     * Add center circle as a composite geom (multiple segments forming a circle)
     */
    static void addCenterCircle(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig);

    /**
     * Add goal structure at specified position and rotation
     * @param worldbodyNode Parent worldbody node
     * @param fieldConfig Field configuration with goal dimensions
     * @param goalPrefix Name prefix for goal geoms (e.g., "left_goal", "right_goal")
     * @param xPosition X position of the goal
     * @param yawRotation Yaw rotation in radians (0 or π)
     */
    static void addGoal(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig,
                        const std::string& goalPrefix, float xPosition, float yawRotation);
};

}  // namespace spqr
