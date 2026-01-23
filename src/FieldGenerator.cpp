#include "FieldGenerator.h"

#include <array>
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

    // Add ball
    std::array<float, 3> ballPosition = {0.0f, 0.0f, 0.4f};  // Center of field, 40cm high
    addBall(assetNode, worldbodyNode, fieldConfig.ball_radius, ballPosition);
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

    // Goals are now generated procedurally, no meshes needed
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
    addGoal(worldbodyNode, fieldConfig, "left_goal", -halfWidth, 0.0f);
    addGoal(worldbodyNode, fieldConfig, "right_goal", halfWidth, static_cast<float>(M_PI));
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
    float overlap = fieldConfig.line_width / 2.0f;

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
    lines.push_back({-halfWidth, goalAreaHalfHeight, z, -halfWidth + fieldConfig.goal_area_width + overlap, goalAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({-halfWidth, -goalAreaHalfHeight, z, -halfWidth + fieldConfig.goal_area_width + overlap, -goalAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({-halfWidth + fieldConfig.goal_area_width, -goalAreaHalfHeight - overlap, z, -halfWidth + fieldConfig.goal_area_width,
                     goalAreaHalfHeight + overlap, z});

    // Goal area - Right
    // Top horizontal line - extend horizontally
    lines.push_back({halfWidth, goalAreaHalfHeight, z, halfWidth - fieldConfig.goal_area_width - overlap, goalAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({halfWidth, -goalAreaHalfHeight, z, halfWidth - fieldConfig.goal_area_width - overlap, -goalAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({halfWidth - fieldConfig.goal_area_width, -goalAreaHalfHeight - overlap, z, halfWidth - fieldConfig.goal_area_width,
                     goalAreaHalfHeight + overlap, z});

    // Penalty area - Left
    float penaltyAreaHalfHeight = fieldConfig.penalty_area_height / 2.0f;
    // Top horizontal line - extend horizontally
    lines.push_back({-halfWidth, penaltyAreaHalfHeight, z, -halfWidth + fieldConfig.penalty_area_width + overlap, penaltyAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({-halfWidth, -penaltyAreaHalfHeight, z, -halfWidth + fieldConfig.penalty_area_width + overlap, -penaltyAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({-halfWidth + fieldConfig.penalty_area_width, -penaltyAreaHalfHeight - overlap, z, -halfWidth + fieldConfig.penalty_area_width,
                     penaltyAreaHalfHeight + overlap, z});

    // Penalty area - Right
    // Top horizontal line - extend horizontally
    lines.push_back({halfWidth, penaltyAreaHalfHeight, z, halfWidth - fieldConfig.penalty_area_width - overlap, penaltyAreaHalfHeight, z});
    // Bottom horizontal line - extend horizontally
    lines.push_back({halfWidth, -penaltyAreaHalfHeight, z, halfWidth - fieldConfig.penalty_area_width - overlap, -penaltyAreaHalfHeight, z});
    // Vertical line - extend vertically
    lines.push_back({halfWidth - fieldConfig.penalty_area_width, -penaltyAreaHalfHeight - overlap, z, halfWidth - fieldConfig.penalty_area_width,
                     penaltyAreaHalfHeight + overlap, z});

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
    float crossLength = fieldConfig.line_width * 3.0f;  // 24 cm total length
    float crossWidth = fieldConfig.line_width;          // same as line width
    float halfCrossLength = crossLength / 2.0f;         // 12 cm from center
    float halfCrossWidth = crossWidth / 2.0f;           // 4 cm from center

    // Left penalty mark - horizontal segment
    LineSegment leftHorizontal = {-halfWidth + fieldConfig.penalty_mark_distance - halfCrossLength, 0, penaltyMarkZ,
                                  -halfWidth + fieldConfig.penalty_mark_distance + halfCrossLength, 0, penaltyMarkZ};
    addLineSegment(worldbodyNode, "penalty_mark_left_horizontal", leftHorizontal, crossWidth);

    // Left penalty mark - vertical segment
    LineSegment leftVertical = {-halfWidth + fieldConfig.penalty_mark_distance, -halfCrossLength, penaltyMarkZ,
                                -halfWidth + fieldConfig.penalty_mark_distance, halfCrossLength,  penaltyMarkZ};
    addLineSegment(worldbodyNode, "penalty_mark_left_vertical", leftVertical, crossWidth);

    // Right penalty mark - horizontal segment
    LineSegment rightHorizontal = {halfWidth - fieldConfig.penalty_mark_distance - halfCrossLength, 0, penaltyMarkZ,
                                   halfWidth - fieldConfig.penalty_mark_distance + halfCrossLength, 0, penaltyMarkZ};
    addLineSegment(worldbodyNode, "penalty_mark_right_horizontal", rightHorizontal, crossWidth);

    // Right penalty mark - vertical segment
    LineSegment rightVertical = {halfWidth - fieldConfig.penalty_mark_distance, -halfCrossLength, penaltyMarkZ,
                                 halfWidth - fieldConfig.penalty_mark_distance, halfCrossLength,  penaltyMarkZ};
    addLineSegment(worldbodyNode, "penalty_mark_right_vertical", rightVertical, crossWidth);

    // Center mark - horizontal segment
    LineSegment centerHorizontal = {-halfCrossLength, 0, penaltyMarkZ, halfCrossLength, 0, penaltyMarkZ};
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

void FieldGenerator::addGoal(pugi::xml_node& worldbodyNode, const FieldConfig& fieldConfig, const std::string& goalPrefix, float xPosition,
                             float yawRotation) {
    // Goal dimensions from FieldConfig
    float goalWidth = fieldConfig.goal_width;
    float goalHeight = fieldConfig.goal_height;
    float goalDepth = fieldConfig.goal_depth;
    float postRadius = fieldConfig.line_width / 2.0f;

    float halfGoalWidth = goalWidth / 2.0f;

    // Determine direction multiplier based on which goal this is
    float directionSign = (yawRotation > 0) ? 1.0f : -1.0f;

    // Calculate goal position (posts are at the goal line)
    float goalX = xPosition;

    std::ostringstream postSize;
    postSize << postRadius << " " << (goalHeight / 2.0f);
    std::ostringstream backPostSize;
    backPostSize << (postRadius / 2.0f) << " " << (goalHeight / 2.0f);

    std::ostringstream crossbarSize;
    crossbarSize << postRadius << " " << ((goalWidth / 2.0f) + postRadius);
    std::ostringstream backCrossbarSize;
    backCrossbarSize << (postRadius / 2.0f) << " " << (goalWidth / 2.0f);

    std::ostringstream depthSize;
    depthSize << (postRadius / 2.0f) << " " << (goalDepth / 2.0f);

    std::string color = "0.8 0.8 0.8 1.0";  // Light gray color for goal posts

    // Add front left post (capsule)
    pugi::xml_node leftPost = worldbodyNode.append_child("geom");
    leftPost.append_attribute("name") = (goalPrefix + "_left_post").c_str();
    leftPost.append_attribute("type") = "capsule";
    std::ostringstream leftPostSize;
    leftPostSize << postRadius;
    leftPost.append_attribute("size") = leftPostSize.str().c_str();
    std::ostringstream leftPostFromTo;
    leftPostFromTo << goalX << " " << halfGoalWidth << " 0 " << goalX << " " << halfGoalWidth << " " << goalHeight;
    leftPost.append_attribute("fromto") = leftPostFromTo.str().c_str();
    leftPost.append_attribute("rgba") = color.c_str();

    // Add front right post (capsule)
    pugi::xml_node rightPost = worldbodyNode.append_child("geom");
    rightPost.append_attribute("name") = (goalPrefix + "_right_post").c_str();
    rightPost.append_attribute("type") = "capsule";
    rightPost.append_attribute("size") = leftPostSize.str().c_str();
    std::ostringstream rightPostFromTo;
    rightPostFromTo << goalX << " " << -halfGoalWidth << " 0 " << goalX << " " << -halfGoalWidth << " " << goalHeight;
    rightPost.append_attribute("fromto") = rightPostFromTo.str().c_str();
    rightPost.append_attribute("rgba") = color.c_str();

    // Add front crossbar (capsule connecting top of front posts)
    pugi::xml_node crossbar = worldbodyNode.append_child("geom");
    crossbar.append_attribute("name") = (goalPrefix + "_crossbar").c_str();
    crossbar.append_attribute("type") = "capsule";
    crossbar.append_attribute("size") = leftPostSize.str().c_str();
    std::ostringstream crossbarFromTo;
    crossbarFromTo << goalX << " " << -halfGoalWidth << " " << goalHeight << " " << goalX << " " << halfGoalWidth << " " << goalHeight;
    crossbar.append_attribute("fromto") = crossbarFromTo.str().c_str();
    crossbar.append_attribute("rgba") = color.c_str();

    // Add back left post (capsule)
    pugi::xml_node leftBackPost = worldbodyNode.append_child("geom");
    leftBackPost.append_attribute("name") = (goalPrefix + "_left_back_post").c_str();
    leftBackPost.append_attribute("type") = "capsule";
    std::ostringstream backPostSizeStr;
    backPostSizeStr << (postRadius / 2.0f);
    leftBackPost.append_attribute("size") = backPostSizeStr.str().c_str();
    std::ostringstream leftBackPostFromTo;
    leftBackPostFromTo << (goalX + directionSign * goalDepth) << " " << halfGoalWidth << " 0 " << (goalX + directionSign * goalDepth) << " "
                       << halfGoalWidth << " " << goalHeight;
    leftBackPost.append_attribute("fromto") = leftBackPostFromTo.str().c_str();
    leftBackPost.append_attribute("rgba") = color.c_str();

    // Add back right post (capsule)
    pugi::xml_node rightBackPost = worldbodyNode.append_child("geom");
    rightBackPost.append_attribute("name") = (goalPrefix + "_right_back_post").c_str();
    rightBackPost.append_attribute("type") = "capsule";
    rightBackPost.append_attribute("size") = backPostSizeStr.str().c_str();
    std::ostringstream rightBackPostFromTo;
    rightBackPostFromTo << (goalX + directionSign * goalDepth) << " " << -halfGoalWidth << " 0 " << (goalX + directionSign * goalDepth) << " "
                        << -halfGoalWidth << " " << goalHeight;
    rightBackPost.append_attribute("fromto") = rightBackPostFromTo.str().c_str();
    rightBackPost.append_attribute("rgba") = color.c_str();

    // Add back crossbar (capsule connecting top of back posts)
    pugi::xml_node backCrossbar = worldbodyNode.append_child("geom");
    backCrossbar.append_attribute("name") = (goalPrefix + "_back_crossbar").c_str();
    backCrossbar.append_attribute("type") = "capsule";
    backCrossbar.append_attribute("size") = backPostSizeStr.str().c_str();
    std::ostringstream backCrossbarFromTo;
    backCrossbarFromTo << (goalX + directionSign * goalDepth) << " " << -halfGoalWidth << " " << goalHeight << " "
                       << (goalX + directionSign * goalDepth) << " " << halfGoalWidth << " " << goalHeight;
    backCrossbar.append_attribute("fromto") = backCrossbarFromTo.str().c_str();
    backCrossbar.append_attribute("rgba") = color.c_str();

    // Add left side bar (capsule connecting left posts at top)
    pugi::xml_node leftSideBar = worldbodyNode.append_child("geom");
    leftSideBar.append_attribute("name") = (goalPrefix + "_left_side_bar").c_str();
    leftSideBar.append_attribute("type") = "capsule";
    leftSideBar.append_attribute("size") = backPostSizeStr.str().c_str();
    std::ostringstream leftSideBarFromTo;
    leftSideBarFromTo << goalX << " " << halfGoalWidth << " " << goalHeight << " " << (goalX + directionSign * goalDepth) << " " << halfGoalWidth
                      << " " << goalHeight;
    leftSideBar.append_attribute("fromto") = leftSideBarFromTo.str().c_str();
    leftSideBar.append_attribute("rgba") = color.c_str();

    // Add right side bar (capsule connecting right posts at top)
    pugi::xml_node rightSideBar = worldbodyNode.append_child("geom");
    rightSideBar.append_attribute("name") = (goalPrefix + "_right_side_bar").c_str();
    rightSideBar.append_attribute("type") = "capsule";
    rightSideBar.append_attribute("size") = backPostSizeStr.str().c_str();
    std::ostringstream rightSideBarFromTo;
    rightSideBarFromTo << goalX << " " << -halfGoalWidth << " " << goalHeight << " " << (goalX + directionSign * goalDepth) << " " << -halfGoalWidth
                       << " " << goalHeight;
    rightSideBar.append_attribute("fromto") = rightSideBarFromTo.str().c_str();
    rightSideBar.append_attribute("rgba") = color.c_str();

    // Add net - grid of small cylinders
    float netRadius = 0.002f;                   // 1cm radius for net strands
    float netSpacing = 0.10f;                   // 10cm spacing between strands
    std::string netColor = "0.7 0.7 0.7 0.75";  // Semi-transparent white

    // Vertical net strands (parallel to Y-axis)
    int numVerticalStrands = static_cast<int>(goalHeight / netSpacing) + 1;
    for (int i = 0; i <= numVerticalStrands; ++i) {
        float z = i * netSpacing;
        if (z > goalHeight)
            z = goalHeight;

        // Horizontal strand along width
        pugi::xml_node vertStrand = worldbodyNode.append_child("geom");
        std::ostringstream vertStrandName;
        vertStrandName << goalPrefix << "_net_horiz_" << i;
        vertStrand.append_attribute("name") = vertStrandName.str().c_str();
        vertStrand.append_attribute("type") = "cylinder";
        std::ostringstream vertStrandSize;
        vertStrandSize << netRadius << " " << (goalWidth / 2.0f);
        vertStrand.append_attribute("size") = vertStrandSize.str().c_str();
        std::ostringstream vertStrandPos;
        vertStrandPos << (goalX + directionSign * goalDepth) << " 0 " << z;
        vertStrand.append_attribute("pos") = vertStrandPos.str().c_str();
        vertStrand.append_attribute("euler") = "1.5708 0 0";  // Rotate to align with Y-axis
        vertStrand.append_attribute("rgba") = netColor.c_str();
        vertStrand.append_attribute("contype") = "0";
        vertStrand.append_attribute("conaffinity") = "0";
    }

    // Horizontal net strands (parallel to Z-axis)
    int numHorizontalStrands = static_cast<int>(goalWidth / netSpacing) + 1;
    for (int i = 0; i <= numHorizontalStrands; ++i) {
        float y = -halfGoalWidth + i * netSpacing;
        if (y > halfGoalWidth)
            y = halfGoalWidth;

        // Vertical strand along height
        pugi::xml_node horizStrand = worldbodyNode.append_child("geom");
        std::ostringstream horizStrandName;
        horizStrandName << goalPrefix << "_net_vert_" << i;
        horizStrand.append_attribute("name") = horizStrandName.str().c_str();
        horizStrand.append_attribute("type") = "cylinder";
        std::ostringstream horizStrandSize;
        horizStrandSize << netRadius << " " << (goalHeight / 2.0f);
        horizStrand.append_attribute("size") = horizStrandSize.str().c_str();
        std::ostringstream horizStrandPos;
        horizStrandPos << (goalX + directionSign * goalDepth) << " " << y << " " << (goalHeight / 2.0f);
        horizStrand.append_attribute("pos") = horizStrandPos.str().c_str();
        horizStrand.append_attribute("rgba") = netColor.c_str();
        horizStrand.append_attribute("contype") = "0";
        horizStrand.append_attribute("conaffinity") = "0";
    }

    // Side nets (left and right sides connecting front to back)
    // Left side net - vertical strands
    for (int i = 0; i <= numVerticalStrands; ++i) {
        float z = i * netSpacing;
        if (z > goalHeight)
            z = goalHeight;

        pugi::xml_node leftSideStrand = worldbodyNode.append_child("geom");
        std::ostringstream leftSideStrandName;
        leftSideStrandName << goalPrefix << "_net_left_side_horiz_" << i;
        leftSideStrand.append_attribute("name") = leftSideStrandName.str().c_str();
        leftSideStrand.append_attribute("type") = "cylinder";
        std::ostringstream leftSideStrandSize;
        leftSideStrandSize << netRadius << " " << (goalDepth / 2.0f);
        leftSideStrand.append_attribute("size") = leftSideStrandSize.str().c_str();
        std::ostringstream leftSideStrandPos;
        leftSideStrandPos << (goalX + directionSign * goalDepth / 2.0f) << " " << halfGoalWidth << " " << z;
        leftSideStrand.append_attribute("pos") = leftSideStrandPos.str().c_str();
        std::ostringstream leftSideEuler;
        leftSideEuler << "0 " << (directionSign > 0 ? 1.5708 : -1.5708) << " 0";
        leftSideStrand.append_attribute("euler") = leftSideEuler.str().c_str();
        leftSideStrand.append_attribute("rgba") = netColor.c_str();
        leftSideStrand.append_attribute("contype") = "0";
        leftSideStrand.append_attribute("conaffinity") = "0";
    }

    // Right side net - vertical strands
    for (int i = 0; i <= numVerticalStrands; ++i) {
        float z = i * netSpacing;
        if (z > goalHeight)
            z = goalHeight;

        pugi::xml_node rightSideStrand = worldbodyNode.append_child("geom");
        std::ostringstream rightSideStrandName;
        rightSideStrandName << goalPrefix << "_net_right_side_horiz_" << i;
        rightSideStrand.append_attribute("name") = rightSideStrandName.str().c_str();
        rightSideStrand.append_attribute("type") = "cylinder";
        std::ostringstream rightSideStrandSize;
        rightSideStrandSize << netRadius << " " << (goalDepth / 2.0f);
        rightSideStrand.append_attribute("size") = rightSideStrandSize.str().c_str();
        std::ostringstream rightSideStrandPos;
        rightSideStrandPos << (goalX + directionSign * goalDepth / 2.0f) << " " << -halfGoalWidth << " " << z;
        rightSideStrand.append_attribute("pos") = rightSideStrandPos.str().c_str();
        std::ostringstream rightSideEuler;
        rightSideEuler << "0 " << (directionSign > 0 ? 1.5708 : -1.5708) << " 0";
        rightSideStrand.append_attribute("euler") = rightSideEuler.str().c_str();
        rightSideStrand.append_attribute("rgba") = netColor.c_str();
        rightSideStrand.append_attribute("contype") = "0";
        rightSideStrand.append_attribute("conaffinity") = "0";
    }

    // Left side net - horizontal strands (along depth)
    int numDepthStrands = static_cast<int>(goalDepth / netSpacing) + 1;
    for (int i = 0; i <= numDepthStrands; ++i) {
        float x = goalX + directionSign * i * netSpacing;
        if (std::abs(x - goalX) > goalDepth)
            x = goalX + directionSign * goalDepth;

        pugi::xml_node leftDepthStrand = worldbodyNode.append_child("geom");
        std::ostringstream leftDepthStrandName;
        leftDepthStrandName << goalPrefix << "_net_left_side_vert_" << i;
        leftDepthStrand.append_attribute("name") = leftDepthStrandName.str().c_str();
        leftDepthStrand.append_attribute("type") = "cylinder";
        std::ostringstream leftDepthStrandSize;
        leftDepthStrandSize << netRadius << " " << (goalHeight / 2.0f);
        leftDepthStrand.append_attribute("size") = leftDepthStrandSize.str().c_str();
        std::ostringstream leftDepthStrandPos;
        leftDepthStrandPos << x << " " << halfGoalWidth << " " << (goalHeight / 2.0f);
        leftDepthStrand.append_attribute("pos") = leftDepthStrandPos.str().c_str();
        leftDepthStrand.append_attribute("rgba") = netColor.c_str();
        leftDepthStrand.append_attribute("contype") = "0";
        leftDepthStrand.append_attribute("conaffinity") = "0";
    }

    // Right side net - horizontal strands (along depth)
    for (int i = 0; i <= numDepthStrands; ++i) {
        float x = goalX + directionSign * i * netSpacing;
        if (std::abs(x - goalX) > goalDepth)
            x = goalX + directionSign * goalDepth;

        pugi::xml_node rightDepthStrand = worldbodyNode.append_child("geom");
        std::ostringstream rightDepthStrandName;
        rightDepthStrandName << goalPrefix << "_net_right_side_vert_" << i;
        rightDepthStrand.append_attribute("name") = rightDepthStrandName.str().c_str();
        rightDepthStrand.append_attribute("type") = "cylinder";
        std::ostringstream rightDepthStrandSize;
        rightDepthStrandSize << netRadius << " " << (goalHeight / 2.0f);
        rightDepthStrand.append_attribute("size") = rightDepthStrandSize.str().c_str();
        std::ostringstream rightDepthStrandPos;
        rightDepthStrandPos << x << " " << -halfGoalWidth << " " << (goalHeight / 2.0f);
        rightDepthStrand.append_attribute("pos") = rightDepthStrandPos.str().c_str();
        rightDepthStrand.append_attribute("rgba") = netColor.c_str();
        rightDepthStrand.append_attribute("contype") = "0";
        rightDepthStrand.append_attribute("conaffinity") = "0";
    }
}

void FieldGenerator::addBall(pugi::xml_node& assetNode, pugi::xml_node& worldbodyNode, float ballRadius,
                             const std::array<float, 3>& initialPosition) {
    // Add ball texture
    pugi::xml_node ballTexture = assetNode.append_child("texture");
    ballTexture.append_attribute("name") = "ball_diffuse";
    std::string ballTexturePath = std::string(PROJECT_ROOT) + "/resources/textures/ball.png";
    ballTexture.append_attribute("file") = ballTexturePath.c_str();
    ballTexture.append_attribute("type") = "2d";

    pugi::xml_node ballMat = assetNode.append_child("material");
    ballMat.append_attribute("name") = "ball_mat";
    ballMat.append_attribute("texture") = "ball_diffuse";
    ballMat.append_attribute("texrepeat") = "1 1";
    ballMat.append_attribute("specular") = "0.3";
    ballMat.append_attribute("shininess") = "0.5";
    ballMat.append_attribute("reflectance") = "0.05";
    ballMat.append_attribute("rgba") = "0.5 0.5 0.5 1";

    pugi::xml_node ballMesh = assetNode.append_child("mesh");
    ballMesh.append_attribute("name") = "ball_mesh";
    std::string ballMeshPath = std::string(PROJECT_ROOT) + "/resources/meshes/ball/ball.obj";
    ballMesh.append_attribute("file") = ballMeshPath.c_str();

    // Add ball body
    pugi::xml_node ballBody = worldbodyNode.append_child("body");
    ballBody.append_attribute("name") = "ball";
    std::ostringstream ballPosStream;
    ballPosStream << initialPosition[0] << " " << initialPosition[1] << " " << initialPosition[2];
    ballBody.append_attribute("pos") = ballPosStream.str().c_str();

    // Add freejoint for unconstrained movement
    ballBody.append_child("freejoint");

    // Calculate ball properties
    // Size 5 ball: mass = 425g, radius from config (typically 0.11m for 22cm diameter)
    float mass = 0.425f;  // kg
    // Inertia for thin spherical shell: (2/3)*m*r^2
    float inertia = (2.0f / 3.0f) * mass * ballRadius * ballRadius;

    // Add inertial properties
    pugi::xml_node inertial = ballBody.append_child("inertial");
    inertial.append_attribute("mass") = "0.425";
    std::ostringstream inertiaStream;
    inertiaStream << inertia << " " << inertia << " " << inertia;
    inertial.append_attribute("diaginertia") = inertiaStream.str().c_str();
    inertial.append_attribute("pos") = "0 0 0";

    // Add mesh geom (instead of sphere for proper UV mapping)
    pugi::xml_node ballGeom = ballBody.append_child("geom");
    ballGeom.append_attribute("name") = "ball_geom";
    ballGeom.append_attribute("type") = "mesh";
    ballGeom.append_attribute("mesh") = "ball_mesh";
    ballGeom.append_attribute("material") = "ball_mat";
    ballGeom.append_attribute("condim") = "6";
    ballGeom.append_attribute("friction") = "0.8 0.005 0.0005";  // sliding, torsional, rolling
}

}  // namespace spqr
