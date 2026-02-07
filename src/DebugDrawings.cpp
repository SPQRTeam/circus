#include "DebugDrawings.h"

namespace spqr {

void DebugDrawings::init(mjvScene* s) noexcept {
    ptrDebugDrawingsScene = s;
}

void DebugDrawings::processDebugMessage(std::map<std::string, msgpack::object> data_map) {
    bool removeAll = data_map.find("removeAll") != data_map.end() && data_map["removeAll"].as<bool>();
    bool removeFigure = data_map.find("removeFigure") != data_map.end() && data_map["removeFigure"].as<bool>();
    
    if ( removeAll == true) {
        DebugDrawings::removeAll();
        return;
    }
    
    std::string idLocal = data_map["idLocal"].as<std::string>();
    std::string drawGeomTypeStr = data_map["drawGeomType"].as<std::string>();

    if (idLocal.empty()){
        throw std::runtime_error("idLocal is required in the debug message");
    } else if (drawGeomTypeStr.empty()) {
        throw std::runtime_error("drawGeomType is required in the debug message");
    }

    // Find debug figure
    DebugDrawings::drawGeomType customType;
    bool found = false;
    for (int i = 0; i < static_cast<int>(DebugDrawings::drawGeomType::COUNT); i++) {
        if (DebugDrawings::toString(static_cast<DebugDrawings::drawGeomType>(i)) == drawGeomTypeStr) {
            customType = static_cast<DebugDrawings::drawGeomType>(i);
            found = true;
            break;
        }
    }

    if (found == false){
        std::stringstream ss;
        ss << "Unknown drawGeomType: " << drawGeomTypeStr << ". Available types: ";
        for (int i = 0; i < static_cast<int>(drawGeomType::COUNT); ++i) {
            ss << toString(static_cast<drawGeomType>(i)) << " ";
        }
        throw std::runtime_error(ss.str());
    }

    if (removeFigure == true) {
        DebugDrawings::removeGeom(idLocal, customType);
        return;
    }

    if (customType == DebugDrawings::drawGeomType::Sphere) {
        DebugDrawings::drawSphere(data_map);
    } else if (customType == DebugDrawings::drawGeomType::Circle) {
        DebugDrawings::drawCircle(data_map);
    } else if (customType == DebugDrawings::drawGeomType::Cylinder) {
        DebugDrawings::drawCylinder(data_map);
    } else if (customType == DebugDrawings::drawGeomType::Arrow) {
        DebugDrawings::drawArrow(data_map);
    } else if (customType == DebugDrawings::drawGeomType::Line) {
        DebugDrawings::drawLine(data_map);
    } else{
        throw std::runtime_error("Unsupported drawGeomType: " + drawGeomTypeStr);
    }
}

void DebugDrawings::removeGeom(const std::string& idLocal, const drawGeomType customType) {
    mjString extendedId = getMapIdGeomLocalId(idLocal, customType);
    mapIdGeom.erase(mapIdGeom.find(extendedId));
}

void DebugDrawings::removeAll() {
    mapIdGeom.clear();
}

void DebugDrawings::drawDebugDrawings() {
    if (!ptrDebugDrawingsScene)
        return;
    for (const auto& pair : mapIdGeom) {
        if (ptrDebugDrawingsScene->ngeom == ptrDebugDrawingsScene->maxgeom)
            break;

        ptrDebugDrawingsScene->geoms[ptrDebugDrawingsScene->ngeom] = pair.second.geom;
        ptrDebugDrawingsScene->ngeom += 1;
    }
}

mjString DebugDrawings::getMapIdGeomLocalId(const std::string& idLocal, const drawGeomType customType) {
    mjString customTypeStr = toString(customType);
    mjString extendedIdPrefix = idLocal + "_" + customTypeStr;
    return extendedIdPrefix;
}

void DebugDrawings::drawRegularGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                    const double size[3], const double pos[3], QColor color) {
    mjvGeom geom;
    const float colorRGBA[4] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};

    mjv_initGeom(&geom, mujocoType, size, pos, NULL, colorRGBA);

    mjString extendedId = getMapIdGeomLocalId(idLocal, customType);
    mapIdGeom[extendedId] = {.geom = geom, .customType = customType};
}

void DebugDrawings::drawRenderOnlyGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                       const double size[3], const double start[3], const double end[3],
                                       const double width, QColor color) {
    mjvGeom geom;
    const float colorRGBA[4] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};

    mjv_initGeom(&geom, mujocoType, size, start, NULL, colorRGBA);
    mjv_connector(&geom, mujocoType, width, start, end);

    mjString extendedId = getMapIdGeomLocalId(idLocal, customType);
    mapIdGeom[extendedId] = {.geom = geom, .customType = customType};
}

mjvGeom* DebugDrawings::isGeomExist(mjString idLocal, drawGeomType customType) {
    mjString extendedId = getMapIdGeomLocalId(idLocal, customType);
    auto it = mapIdGeom.find(extendedId);

    if (it == mapIdGeom.end())
        return nullptr;
    else if ((it->second).customType == customType)
        return &((it->second).geom);
    else
        return nullptr;
}

void DebugDrawings::moveGeom(mjvGeom* g, const double center[3], QColor color) {
    g->pos[0] = center[0];
    g->pos[1] = center[1];
    g->pos[2] = center[2];

    const float colorRGBA[4] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
    for (int i = 0; i < 4; ++i) {
        g->rgba[i] = colorRGBA[i];
    }
}

void DebugDrawings::moveGeom(mjvGeom* g, const double start[3], const double end[3], QColor color) {
    const float colorRGBA[4] = {color.redF(), color.greenF(), color.blueF(), color.alphaF()};
    for (int i = 0; i < 4; ++i) {
        g->rgba[i] = colorRGBA[i];
    }

    mjv_connector(g, g->type, g->size[0], start, end);
}

void DebugDrawings::drawSphere(const std::map<std::string, msgpack::object>& data_map) {
    const auto centerArray = data_map.at("center").as<std::array<double, 3>>();
    const auto colorArray = data_map.at("color").as<std::array<double, 4>>();

    const auto idLocal = data_map.at("idLocal").as<std::string>();

    const double center[3] = {centerArray[0], centerArray[1], centerArray[2]};
    const double radius = data_map.at("radius").as<double>();
    QColor color = QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);

    mjtGeom geomType = mjGEOM_SPHERE;
    drawGeomType customType = drawGeomType::Sphere;

    mjvGeom* foundGeom = isGeomExist(idLocal, customType);

    if (foundGeom != nullptr) {
        moveGeom(foundGeom, center, color);
    } else {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(idLocal, geomType, customType, size, center, color);
    }
}

void DebugDrawings::drawCircle(const std::map<std::string, msgpack::object>& data_map) {
    const auto centerArray = data_map.at("center").as<std::array<double, 3>>();
    const auto colorArray = data_map.at("color").as<std::array<double, 4>>();

    const auto idLocal = data_map.at("idLocal").as<std::string>();
    const double center[3] = {centerArray[0], centerArray[1], centerArray[2]};
    const double radius = data_map.at("radius").as<double>();
    QColor color = QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);

    mjtGeom geomType = mjGEOM_CYLINDER;
    drawGeomType customType = drawGeomType::Circle;

    mjvGeom* foundGeom = isGeomExist(idLocal, customType);

    if (foundGeom != nullptr) {
        moveGeom(foundGeom, center, color);
    } else {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(idLocal, geomType, customType, size, center, color);
    }
}

void DebugDrawings::drawCylinder(const std::map<std::string, msgpack::object>& data_map) {
    const auto centerArray = data_map.at("center").as<std::array<double, 3>>();
    const auto colorArray = data_map.at("color").as<std::array<double, 4>>();

    const auto idLocal = data_map.at("idLocal").as<std::string>();
    const double center[3] = {centerArray[0], centerArray[1], centerArray[2]};
    const double radius = data_map.at("radius").as<double>();
    const double length = data_map.at("length").as<double>();
    QColor color = QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);

    mjtGeom geomType = mjGEOM_CYLINDER;
    drawGeomType customType = drawGeomType::Cylinder;

    mjvGeom* foundGeom = isGeomExist(idLocal, customType);

    if (foundGeom != nullptr) {
        moveGeom(foundGeom, center, color);
    } else {
        double size[3] = {radius, length, 0.0};
        drawRegularGeom(idLocal, geomType, customType, size, center, color);
    }
}

void DebugDrawings::drawArrow(const std::map<std::string, msgpack::object>& data_map) {
    const auto startArray = data_map.at("start").as<std::array<double, 3>>();
    const auto endArray = data_map.at("end").as<std::array<double, 3>>();
    const auto colorArray = data_map.at("color").as<std::array<double, 4>>();

    const auto idLocal = data_map.at("idLocal").as<std::string>();
    const double start[3] = {startArray[0], startArray[1], startArray[2]};
    const double end[3] = {endArray[0], endArray[1], endArray[2]};
    const double thickness = data_map.at("thickness").as<double>();
    QColor color = QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);

    mjtGeom geomType = mjGEOM_ARROW;
    drawGeomType customType = drawGeomType::Arrow;

    mjvGeom* foundGeom = isGeomExist(idLocal, customType);

    if (foundGeom != nullptr) {
        moveGeom(foundGeom, start, end, color);
    } else {
        double size[3] = {1.0, 1.0, 1.0};
        double endDoubled[3] = {end[0] * 2, end[1], end[2]};
        drawRenderOnlyGeom(idLocal, geomType, customType, size, start, endDoubled, thickness / 100, color);
    }
}

void DebugDrawings::drawLine(const std::map<std::string, msgpack::object>& data_map) {
    const auto startArray = data_map.at("start").as<std::array<double, 3>>();
    const auto endArray = data_map.at("end").as<std::array<double, 3>>();
    const auto colorArray = data_map.at("color").as<std::array<double, 4>>();

    const auto idLocal = data_map.at("idLocal").as<std::string>();
    const double start[3] = {startArray[0], startArray[1], startArray[2]};
    const double end[3] = {endArray[0], endArray[1], endArray[2]};
    const double thickness = data_map.at("thickness").as<double>();
    QColor color = QColor::fromRgbF(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);

    mjtGeom geomType = mjGEOM_LINE;
    drawGeomType customType = drawGeomType::Line;

    mjvGeom* foundGeom = isGeomExist(idLocal, customType);

    if (foundGeom != nullptr) {
        moveGeom(foundGeom, start, end, color);
    } else {
        drawRenderOnlyGeom(idLocal, geomType, customType, NULL, start, end, thickness, color);
    }
}

}  // namespace spqr