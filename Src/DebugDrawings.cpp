#include "DebugDrawings.h"

namespace spqr {

void DebugDrawings::init(mjvScene* s) noexcept {
    ptrDebugDrawingsScene = s;
}

void DebugDrawings::drawDebugDrawings() {
    if (!ptrDebugDrawingsScene)
        return;

    for (int i = 0; i < geomsVector.size(); i++) {
        ptrDebugDrawingsScene->geoms[ptrDebugDrawingsScene->ngeom] = geomsVector[i];
        ptrDebugDrawingsScene->ngeom += 1;
    }
}

void DebugDrawings::drawRegularGeom(mjtGeom type, const double size[3], const double pos[3],
                                    const float color[4]) {
    mjvGeom geom;
    mjv_initGeom(&geom, type, size, pos, NULL, color);

    geomsVector.push_back(geom);
}

void DebugDrawings::drawRenderOnlyGeom(mjtGeom type, const double size[3], const double start[3],
                                       const double end[3], const double width, const float color[4]) {
    mjvGeom geom;
    mjv_initGeom(&geom, type, size, start, NULL, color);
    mjv_connector(&geom, type, width, start, end);

    geomsVector.push_back(geom);
}

bool DebugDrawings::moveGeom(int idLocal, const double center[3]) {
    auto it = std::find(idsVector.begin(), idsVector.end(), idLocal);
    if (it != idsVector.end()) {
        const size_t idx = static_cast<size_t>(std::distance(idsVector.begin(), it));

        mjvGeom& g = geomsVector[idx];

        g.pos[0] = center[0];
        g.pos[1] = center[1];
        g.pos[2] = center[2];
        return true;
    } else {
        idsVector.push_back(idLocal);
        return false;
    }
}

bool DebugDrawings::moveGeom(int idLocal, const double start[3], const double end[3]) {
    auto it = std::find(idsVector.begin(), idsVector.end(), idLocal);
    if (it != idsVector.end()) {
        const size_t idx = static_cast<size_t>(std::distance(idsVector.begin(), it));

        mjvGeom& g = geomsVector[idx];

        mjv_connector(&g, g.type, g.size[0], start, end);

        return true;
    } else {
        idsVector.push_back(idLocal);
        return false;
    }
}

void DebugDrawings::drawSphere(int idLocal, const double center[3], const double radius,
                               const float color[4]) {
    if (!moveGeom(idLocal, center)) {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(mjGEOM_SPHERE, size, center, color);
    }
}

void DebugDrawings::drawCircle(int idLocal, const double center[3], const double radius,
                               const float color[4]) {
    if (!moveGeom(idLocal, center)) {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(mjGEOM_CYLINDER, size, center, color);
    }
}

void DebugDrawings::drawCylinder(int idLocal, const double center[3], double radius, double length,
                                 const float color[4]) {
    if (!moveGeom(idLocal, center)) {
        double size[3] = {radius, length, 0.0};
        drawRegularGeom(mjGEOM_CYLINDER, size, center, color);
    }
}

void DebugDrawings::drawArrow(int idLocal, const double start[3], const double end[3], const double thickness,
                              const float color[4]) {
    if (!moveGeom(idLocal, start, end)) {
        mjtGeom geomType = mjGEOM_ARROW;
        double size[3] = {1.0, 1.0, 1.0};
        double endDoubled[3] = {end[0] * 2, end[1], end[2]};
        drawRenderOnlyGeom(geomType, size, start, endDoubled, thickness / 100, color);
    }
}

void DebugDrawings::drawLine(int idLocal, const double start[3], const double end[3], const double thickness,
                             const float color[4]) {
    if (!moveGeom(idLocal, start, end)) {
        mjtGeom geomType = mjGEOM_LINE;
        drawRenderOnlyGeom(geomType, NULL, start, end, thickness, color);
    }
}

}  // namespace spqr
