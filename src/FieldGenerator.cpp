#include "FieldGenerator.h"

#include <cmath>
#include <sstream>

#include "SceneParser.h"

namespace spqr {

std::string FieldGenerator::generateFieldXML(const FieldConfig& fieldConfig, const std::string& meshDir) {
    pugi::xml_document doc;
    pugi::xml_node mujoco = doc.append_child("mujoco");
    mujoco.append_attribute("model") = "field";

    appendFieldToMuJoCo(mujoco, fieldConfig, meshDir);

    std::ostringstream oss;
    doc.save(oss, "  ");
    return oss.str();
}

void FieldGenerator::appendFieldToMuJoCo(pugi::xml_node& mujocoNode, const FieldConfig& fieldConfig, const std::string& meshDir) {
    // Add assets (textures, materials, goal meshes)
    pugi::xml_node assetNode = mujocoNode.child("asset");
    if (!assetNode) {
        assetNode = mujocoNode.append_child("asset");
    }
    addFieldAssets(assetNode, fieldConfig, meshDir);

    // Add visual settings
    pugi::xml_node visualNode = mujocoNode.child("visual");
    if (!visualNode) {
        visualNode = mujocoNode.append_child("visual");
    }
    pugi::xml_node mapNode = visualNode.child("map");
    if (!mapNode) {
        mapNode = visualNode.append_child("map");
    }
    mapNode.append_attribute("shadowclip") = "1.0";
    mapNode.append_attribute("znear") = "0.0001";

    // Add worldbody geometries
    pugi::xml_node worldbodyNode = mujocoNode.child("worldbody");
    if (!worldbodyNode) {
        worldbodyNode = mujocoNode.append_child("worldbody");
    }
    addFieldGeometries(worldbodyNode, fieldConfig);
}

void FieldGenerator::addFieldAssets(pugi::xml_node& assetNode, const FieldConfig& fieldConfig, const std::string& meshDir) {
    // Grid texture and material
    pugi::xml_node gridTex = assetNode.append_child("texture");
    gridTex.append_attribute("name") = "grid_tex";
    gridTex.append_attribute("type") = "2d";
    gridTex.append_attribute("builtin") = "checker";
    gridTex.append_attribute("rgb1") = "0.2 0.2 0.2";
    gridTex.append_attribute("rgb2") = "0.25 0.25 0.25";
    gridTex.append_attribute("width") = "512";
    gridTex.append_attribute("height") = "512";

    pugi::xml_node gridMat = assetNode.append_child("material");
    gridMat.append_attribute("name") = "grid_mat";
    gridMat.append_attribute("texture") = "grid_tex";
    gridMat.append_attribute("texrepeat") = "50 50";

    // Grass texture and material
    pugi::xml_node grassTex = assetNode.append_child("texture");
    grassTex.append_attribute("name") = "grass_tex";
    grassTex.append_attribute("type") = "2d";
    grassTex.append_attribute("builtin") = "flat";
    grassTex.append_attribute("rgb1") = "0.2 0.5 0.2";
    grassTex.append_attribute("width") = "512";
    grassTex.append_attribute("height") = "512";

    pugi::xml_node grassMat = assetNode.append_child("material");
    grassMat.append_attribute("name") = "grass";
    grassMat.append_attribute("texture") = "grass_tex";
    grassMat.append_attribute("specular") = "0.1";
    grassMat.append_attribute("shininess") = "0.05";
    grassMat.append_attribute("reflectance") = "0.0";

    // Lines material (no mesh, lines will be generated as geoms)
    pugi::xml_node linesMat = assetNode.append_child("material");
    linesMat.append_attribute("name") = "lines_mat";
    linesMat.append_attribute("rgba") = "0.9 0.9 0.9 1.0";
    linesMat.append_attribute("specular") = "0.2";
    linesMat.append_attribute("shininess") = "0.1";
    linesMat.append_attribute("reflectance") = "0.0";

    // Goal meshes
    std::string goalMeshPath = meshDir + "/goal/";

    pugi::xml_node leftPost = assetNode.append_child("mesh");
    leftPost.append_attribute("name") = "left_post";
    leftPost.append_attribute("file") = (goalMeshPath + "LeftPost.obj").c_str();

    pugi::xml_node rightPost = assetNode.append_child("mesh");
    rightPost.append_attribute("name") = "right_post";
    rightPost.append_attribute("file") = (goalMeshPath + "RightPost.obj").c_str();

    pugi::xml_node crossbar = assetNode.append_child("mesh");
    crossbar.append_attribute("name") = "crossbar";
    crossbar.append_attribute("file") = (goalMeshPath + "Crossbar.obj").c_str();

    pugi::xml_node leftBackPost = assetNode.append_child("mesh");
    leftBackPost.append_attribute("name") = "left_back_post";
    leftBackPost.append_attribute("file") = (goalMeshPath + "LeftBackPost.obj").c_str();

    pugi::xml_node rightBackPost = assetNode.append_child("mesh");
    rightBackPost.append_attribute("name") = "right_back_post";
    rightBackPost.append_attribute("file") = (goalMeshPath + "RightBackPost.obj").c_str();

    pugi::xml_node backCrossbar = assetNode.append_child("mesh");
    backCrossbar.append_attribute("name") = "back_crossbar";
    backCrossbar.append_attribute("file") = (goalMeshPath + "BackCrossbar.obj").c_str();

    pugi::xml_node rightHolder = assetNode.append_child("mesh");
    rightHolder.append_attribute("name") = "right_holder";
    rightHolder.append_attribute("file") = (goalMeshPath + "RightHolder.obj").c_str();

    pugi::xml_node leftHolder = assetNode.append_child("mesh");
    leftHolder.append_attribute("name") = "left_holder";
    leftHolder.append_attribute("file") = (goalMeshPath + "LeftHolder.obj").c_str();

    pugi::xml_node leftNet = assetNode.append_child("mesh");
    leftNet.append_attribute("name") = "left_net";
    leftNet.append_attribute("file") = (goalMeshPath + "LeftNet.obj").c_str();

    pugi::xml_node rightNet = assetNode.append_child("mesh");
    rightNet.append_attribute("name") = "right_net";
    rightNet.append_attribute("file") = (goalMeshPath + "RightNet.obj").c_str();

    pugi::xml_node backNet = assetNode.append_child("mesh");
    backNet.append_attribute("name") = "back_net";
    backNet.append_attribute("file") = (goalMeshPath + "BackNet.obj").c_str();
}

void FieldGenerator::addFieldGeometries(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig) {
    // Add grid (background plane)
    pugi::xml_node grid = worldbodyNode.append_child("geom");
    grid.append_attribute("name") = "grid";
    grid.append_attribute("type") = "plane";
    grid.append_attribute("size") = "50 50 0.1";
    grid.append_attribute("material") = "grid_mat";
    grid.append_attribute("pos") = "0 0 0";
    grid.append_attribute("euler") = "0 0 0";
    grid.append_attribute("condim") = "3";
    grid.append_attribute("friction") = "0.01 0.01 0.01";

    // Add ground (grass)
    addGroundPlane(worldbodyNode, fieldConfig);

    // Add field lines
    addFieldLines(worldbodyNode, fieldConfig);

    // Add center circle
    addCenterCircle(worldbodyNode, fieldConfig);

    // Add goals
    float halfWidth = fieldConfig.width / 2.0f;
    addGoal(worldbodyNode, "left_goal", -halfWidth, 0.0f);
    addGoal(worldbodyNode, "right_goal", halfWidth, static_cast<float>(M_PI));
}

void FieldGenerator::addGroundPlane(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig) {
    pugi::xml_node ground = worldbodyNode.append_child("geom");
    ground.append_attribute("name") = "ground";
    ground.append_attribute("type") = "plane";

    // Size: half-width, half-height, thickness
    std::ostringstream sizeStream;
    sizeStream << ((fieldConfig.width / 2.0f) + 1) << " " << ((fieldConfig.height / 2.0f) + 1) << " 0.1";
    ground.append_attribute("size") = sizeStream.str().c_str();

    ground.append_attribute("material") = "grass";
    ground.append_attribute("pos") = "0 0 0.001";
    ground.append_attribute("condim") = "3";
    ground.append_attribute("friction") = "0.01 0.01 0.01";
}

std::vector<LineSegment> FieldGenerator::calculateFieldLines(const FieldConfig& fieldConfig) {
    std::vector<LineSegment> lines;

    float halfWidth = fieldConfig.width / 2.0f;
    float halfHeight = fieldConfig.height / 2.0f;
    float z = 0.0001f;  // Slightly above ground
    float overlap = 0.04f;  // 4 cm extension on each side for proper corner overlap

    // Boundary lines (extended by 4cm on each side)
    // Top boundary - extend horizontally
    lines.push_back({-halfWidth - overlap, halfHeight, z, halfWidth + overlap, halfHeight, z});
    // Bottom boundary - extend horizontally
    lines.push_back({-halfWidth - overlap, -halfHeight, z, halfWidth + overlap, -halfHeight, z});
    // Left boundary - extend vertically
    lines.push_back({-halfWidth, -halfHeight - overlap, z, -halfWidth, halfHeight + overlap, z});
    // Right boundary - extend vertically
    lines.push_back({halfWidth, -halfHeight - overlap, z, halfWidth, halfHeight + overlap, z});

    // Halfway line - extend vertically
    lines.push_back({0, -halfHeight - overlap, z, 0, halfHeight + overlap, z});

    // Goal area - Left
    float goalAreaHalfHeight = fieldConfig.goal_area_height / 2.0f;
    // Top horizontal line - extend horizontally
    lines.push_back({-halfWidth, goalAreaHalfHeight, z,
                     -halfWidth + fieldConfig.goal_area_width + overlap, goalAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({-halfWidth, -goalAreaHalfHeight, z,
                     -halfWidth + fieldConfig.goal_area_width + overlap, -goalAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({-halfWidth + fieldConfig.goal_area_width, -goalAreaHalfHeight - overlap, z,
                     -halfWidth + fieldConfig.goal_area_width, goalAreaHalfHeight + overlap, z});

    // Goal area - Right
    // Top horizontal line - extend horizontally
    lines.push_back({halfWidth, goalAreaHalfHeight, z,
                     halfWidth - fieldConfig.goal_area_width - overlap, goalAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({halfWidth, -goalAreaHalfHeight, z,
                     halfWidth - fieldConfig.goal_area_width - overlap, -goalAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({halfWidth - fieldConfig.goal_area_width, -goalAreaHalfHeight - overlap, z,
                     halfWidth - fieldConfig.goal_area_width, goalAreaHalfHeight + overlap, z});

    // Penalty area - Left
    float penaltyAreaHalfHeight = fieldConfig.penalty_area_height / 2.0f;
    // Top horizontal line - extend horizontally
    lines.push_back({-halfWidth, penaltyAreaHalfHeight, z,
                     -halfWidth + fieldConfig.penalty_area_width + overlap, penaltyAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({-halfWidth, -penaltyAreaHalfHeight, z,
                     -halfWidth + fieldConfig.penalty_area_width + overlap, -penaltyAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({-halfWidth + fieldConfig.penalty_area_width, -penaltyAreaHalfHeight - overlap, z,
                     -halfWidth + fieldConfig.penalty_area_width, penaltyAreaHalfHeight + overlap, z});

    // Penalty area - Right
    // Top horizontal line - extend horizontally
    lines.push_back({halfWidth, penaltyAreaHalfHeight, z,
                     halfWidth - fieldConfig.penalty_area_width - overlap, penaltyAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({halfWidth, -penaltyAreaHalfHeight, z,
                     halfWidth - fieldConfig.penalty_area_width - overlap, -penaltyAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({halfWidth - fieldConfig.penalty_area_width, -penaltyAreaHalfHeight - overlap, z,
                     halfWidth - fieldConfig.penalty_area_width, penaltyAreaHalfHeight + overlap, z});

    return lines;
}

void FieldGenerator::addFieldLines(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig) {
    std::vector<LineSegment> lines = calculateFieldLines(fieldConfig);

    int lineIndex = 0;
    for (const auto& line : lines) {
        std::string lineName = "field_line_" + std::to_string(lineIndex++);
        addLineSegment(worldbodyNode, lineName, line, fieldConfig.line_width);
    }

    // Add penalty marks as crosses (two perpendicular line segments)
    // Each penalty mark is a cross: 24cm x 8cm (two segments intersecting at center)
    float halfWidth = fieldConfig.width / 2.0f;
    float penaltyMarkZ = 0.002f;
    float crossLength = 0.24f;  // 24 cm total length
    float crossWidth = 0.08f;   // 8 cm width
    float halfCrossLength = crossLength / 2.0f;  // 12 cm from center
    float halfCrossWidth = crossWidth / 2.0f;    // 4 cm from center

    // Left penalty mark - horizontal segment
    LineSegment leftHorizontal = {
        -halfWidth + fieldConfig.penalty_mark_distance - halfCrossLength, 0, penaltyMarkZ,
        -halfWidth + fieldConfig.penalty_mark_distance + halfCrossLength, 0, penaltyMarkZ
    };
    addLineSegment(worldbodyNode, "penalty_mark_left_horizontal", leftHorizontal, crossWidth);

    // Left penalty mark - vertical segment
    LineSegment leftVertical = {
        -halfWidth + fieldConfig.penalty_mark_distance, -halfCrossLength, penaltyMarkZ,
        -halfWidth + fieldConfig.penalty_mark_distance, halfCrossLength, penaltyMarkZ
    };
    addLineSegment(worldbodyNode, "penalty_mark_left_vertical", leftVertical, crossWidth);

    // Right penalty mark - horizontal segment
    LineSegment rightHorizontal = {
        halfWidth - fieldConfig.penalty_mark_distance - halfCrossLength, 0, penaltyMarkZ,
        halfWidth - fieldConfig.penalty_mark_distance + halfCrossLength, 0, penaltyMarkZ
    };
    addLineSegment(worldbodyNode, "penalty_mark_right_horizontal", rightHorizontal, crossWidth);

    // Right penalty mark - vertical segment
    LineSegment rightVertical = {
        halfWidth - fieldConfig.penalty_mark_distance, -halfCrossLength, penaltyMarkZ,
        halfWidth - fieldConfig.penalty_mark_distance, halfCrossLength, penaltyMarkZ
    };
    addLineSegment(worldbodyNode, "penalty_mark_right_vertical", rightVertical, crossWidth);


    // Center mark - horizontal segment
    LineSegment centerHorizontal = {
        -halfCrossLength, 0, penaltyMarkZ,
        halfCrossLength, 0, penaltyMarkZ
    };
    addLineSegment(worldbodyNode, "center_mark_horizontal", centerHorizontal, crossWidth);
}

void FieldGenerator::addLineSegment(pugi::xml_node& worldbodyNode, const std::string& name, const LineSegment& segment, float width) {
    // Calculate center position
    float cx = (segment.x1 + segment.x2) / 2.0f;
    float cy = (segment.y1 + segment.y2) / 2.0f;
    float cz = (segment.z1 + segment.z2) / 2.0f;

    // Calculate length
    float dx = segment.x2 - segment.x1;
    float dy = segment.y2 - segment.y1;
    float dz = segment.z2 - segment.z1;
    float length = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Calculate rotation angle around z-axis
    float angle = std::atan2(dy, dx);

    // Create box geom for flat rectangular lines
    pugi::xml_node line = worldbodyNode.append_child("geom");
    line.append_attribute("name") = name.c_str();
    line.append_attribute("type") = "box";

    // Size: half-length, half-width, half-height (very thin in z direction)
    std::ostringstream sizeStream;
    sizeStream << (length / 2.0f) << " " << (width / 2.0f) << " 0.001";
    line.append_attribute("size") = sizeStream.str().c_str();

    // Position at center
    std::ostringstream posStream;
    posStream << cx << " " << cy << " " << cz;
    line.append_attribute("pos") = posStream.str().c_str();

    // Rotation: only rotate around z-axis to align with line direction
    std::ostringstream eulerStream;
    eulerStream << "0 0 " << angle;
    line.append_attribute("euler") = eulerStream.str().c_str();

    line.append_attribute("material") = "lines_mat";
    line.append_attribute("contype") = "0";
    line.append_attribute("conaffinity") = "0";
}

void FieldGenerator::addCenterCircle(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig) {
    // Approximate circle with multiple box segments
    int numSegments = 100;  // More segments = smoother circle
    float radius = fieldConfig.center_radius;
    float z = 0.002f;
    float anglePerSegment = (2.0f * M_PI) / numSegments;

    // Add small overlap to prevent gaps between segments
    // Overlap angle is proportional to line width relative to circle circumference
    float overlapAngle = (fieldConfig.line_width / (2.0f * M_PI * radius)) * 0.5f;

    for (int i = 0; i < numSegments; ++i) {
        float angle1 = anglePerSegment * i - overlapAngle;
        float angle2 = anglePerSegment * (i + 1) + overlapAngle;

        LineSegment segment;
        segment.x1 = radius * std::cos(angle1);
        segment.y1 = radius * std::sin(angle1);
        segment.z1 = z;
        segment.x2 = radius * std::cos(angle2);
        segment.y2 = radius * std::sin(angle2);
        segment.z2 = z;

        std::string segmentName = "center_circle_seg_" + std::to_string(i);
        addLineSegment(worldbodyNode, segmentName, segment, fieldConfig.line_width);
    }
}

void FieldGenerator::addGoal(pugi::xml_node& worldbodyNode, const std::string& goalPrefix,
                              float xPosition, float yawRotation) {
    std::ostringstream posStream;
    posStream << xPosition << " 0 0";
    std::string posStr = posStream.str();

    std::ostringstream eulerStream;
    eulerStream << (M_PI / 2.0) << " " << yawRotation << " 0";
    std::string eulerStr = eulerStream.str();

    // Goal mesh names
    std::vector<std::string> meshNames = {
        "left_post", "right_post", "crossbar",
        "left_back_post", "right_back_post", "back_crossbar",
        "right_holder", "left_holder",
        "left_net", "right_net", "back_net"
    };

    for (const auto& meshName : meshNames) {
        pugi::xml_node geom = worldbodyNode.append_child("geom");
        std::string geomName = goalPrefix + "_" + meshName;
        geom.append_attribute("name") = geomName.c_str();
        geom.append_attribute("type") = "mesh";
        geom.append_attribute("mesh") = meshName.c_str();
        geom.append_attribute("pos") = posStr.c_str();
        geom.append_attribute("euler") = eulerStr.c_str();
    }
}

}  // namespace spqr
