#include "DebugDrawings.h"

namespace spqr {

void DebugDrawings::init(mjvScene* s) noexcept {
    ptrDebugDrawingsScene = s;
}

void DebugDrawings::drawDebugDrawings() {
    if (!ptrDebugDrawingsScene)
        return;

    for (const auto& pair : mapIdGeom){

        if( ptrDebugDrawingsScene->ngeom == ptrDebugDrawingsScene->maxgeom)
            break;
        
        ptrDebugDrawingsScene->geoms[ptrDebugDrawingsScene->ngeom] = pair.second.geom;
        ptrDebugDrawingsScene->ngeom += 1;
    }
}

void DebugDrawings::drawRegularGeom(mjString idLocal, mjtGeom type, drawGeomType geomType, const double size[3], const double pos[3],
                                    const float color[4]) {
    mjvGeom geom;
    mjv_initGeom(&geom, type, size, pos, NULL, color);

    mjString extendedId = idLocal + "_" + std::to_string(static_cast<int>(geomType));
    mapIdGeom[extendedId] = { .geom = geom, .drawType = geomType };
}

void DebugDrawings::drawRenderOnlyGeom(mjString idLocal, mjtGeom type, drawGeomType geomType, const double size[3], const double start[3],
                                       const double end[3], const double width, const float color[4]) {
    mjvGeom geom;
    mjv_initGeom(&geom, type, size, start, NULL, color);
    mjv_connector(&geom, type, width, start, end);

    mjString extendedId = idLocal + "_" + std::to_string(static_cast<int>(geomType));
    mapIdGeom[extendedId] = { .geom = geom, .drawType = geomType };
}

mjvGeom* DebugDrawings::isGeomExist(mjString idLocal, drawGeomType drawType){

    mjString extendedId = idLocal + "_" + std::to_string(static_cast<int>(drawType));
    auto it = mapIdGeom.find(extendedId);
    
    if( it == mapIdGeom.end() ) 
        return nullptr;
    else if((it->second).drawType == drawType)
        return &((it->second).geom);
    else
        return nullptr;
    
}

void DebugDrawings::moveGeom(mjvGeom* g, const double center[3]) {

    g->pos[0] = center[0];
    g->pos[1] = center[1];
    g->pos[2] = center[2];
}

void DebugDrawings::moveGeom(mjvGeom* g, const double start[3], const double end[3]) {

    mjv_connector(g, g->type, g->size[0], start, end);
}

void DebugDrawings::drawSphere(mjString idLocal, const double center[3], const double radius,
                               const float color[4]) {

    mjtGeom geomType = mjGEOM_SPHERE; 
    drawGeomType drawType = drawGeomType::Sphere;

    mjvGeom* foundGeom = isGeomExist(idLocal, drawType);

    if ( foundGeom != nullptr ) {
        moveGeom(foundGeom, center);
    }
    else
    {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(idLocal, geomType, drawType, size, center, color);
    }
}

void DebugDrawings::drawCircle(mjString idLocal, const double center[3], const double radius,
                               const float color[4]) {
    mjtGeom geomType = mjGEOM_CYLINDER;
    drawGeomType drawType = drawGeomType::Circle;

    mjvGeom* foundGeom = isGeomExist(idLocal, drawType);

    if ( foundGeom != nullptr ) {
        moveGeom(foundGeom, center);
    }
    else
    {
        double size[3] = {radius, 0.0, 0.0};
        drawRegularGeom(idLocal, geomType, drawType, size, center, color);
    }
}

void DebugDrawings::drawCylinder(mjString idLocal, const double center[3], double radius, double length,
                                 const float color[4]) {
    mjtGeom geomType = mjGEOM_CYLINDER;
    drawGeomType drawType = drawGeomType::Cylinder;

    mjvGeom* foundGeom = isGeomExist(idLocal, drawType);

    if ( foundGeom != nullptr ) {
        moveGeom(foundGeom, center);
    }
    else
    {
        double size[3] = {radius, length, 0.0};
        drawRegularGeom(idLocal, geomType, drawType, size, center, color);
    }
}

void DebugDrawings::drawArrow(mjString idLocal, const double start[3], const double end[3], const double thickness,
                              const float color[4]) {
    mjtGeom geomType = mjGEOM_ARROW;
    drawGeomType drawType = drawGeomType::Arrow;

    mjvGeom* foundGeom = isGeomExist(idLocal, drawType);

    if ( foundGeom != nullptr ) {
        moveGeom(foundGeom, start, end);
    }
    else
    {
        double size[3] = {1.0, 1.0, 1.0};
        double endDoubled[3] = {end[0] * 2, end[1], end[2]};
        drawRenderOnlyGeom(idLocal, geomType, drawType, size, start, endDoubled, thickness / 100, color);
    }
}

void DebugDrawings::drawLine(mjString idLocal, const double start[3], const double end[3], const double thickness,
                             const float color[4]) {
    mjtGeom geomType = mjGEOM_LINE;
    drawGeomType drawType = drawGeomType::Line;

    mjvGeom* foundGeom = isGeomExist(idLocal, drawType);

    if ( foundGeom != nullptr ) {
        moveGeom(foundGeom, start, end);
    }
    else
    {
        drawRenderOnlyGeom(idLocal, geomType, drawType, NULL, start, end, thickness, color);
    }
}

}  // namespace spqr
