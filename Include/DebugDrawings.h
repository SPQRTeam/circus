#pragma once

#include <mujoco/mujoco.h>
#include <algorithm>

namespace spqr
{

class DebugDrawings
{

public:

    static void drawAllPrimitives();
    static void init(mjvScene* s) noexcept; 
    static void drawDebugDrawings();

    // ---------------- regular geom types (mjtGeom)
    /**
     * @brief drawCircle: initialize a mjGEOM_CIRCLE in mujoco
     * @param idLocal: id of the drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the circle
     * @param color: RGBA color of the geom
     */
    static void drawCircle(int idLocal, const double center[3], const double radius, const float color[4]);

    /**
     * @brief drawSphere: initialize a mjGEOM_SPHERE in mujoco
     * @param idLocal: id of the drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the sphere
     * @param color: RGBA color of the geom
     */
    static void drawSphere(int idLocal, const double center[3], const double radius, const float color[4]);

    /**
     * @brief drawCylinder: initialize a mjGEOM_SPHERE in mujoco
     * @param idLocal: id of the drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the cylinder
     * @param length: half length of the cylinder
     * @param color: RGBA color of the geom
     */
    static void drawCylinder(int idLocal, const double center[3], const double radius, const double length, const float color[4]);

    // ---------------- rendering-only geom types (mjtGeom)

    /**
     * @brief drawArrow: initialize a mjGEOM_ARROW in mujoco
     * @param idLocal: id of the geom object
     * @param start: position of the start point of the arrow
     * @param end: position of the end point of the arrow
     * @param thickness: thickness of the arrow
     * @param color: RGBA color of the geom
     */
    static void drawArrow(int idLocal, const double start[3], const double end[3], const double thickness, const float color[4]);

    /**
     * @brief drawLine: initialize a mjGEOM_LINE in mujoco
     * @param idLocal: id of the drawing
     * @param start: position of the start point of the line
     * @param end: position of the end point of the line
     * @param thickness: thickness of the line
     * @param color: RGBA color of the geom
     */
    static void drawLine(int idLocal, const double start[3], const double end[3], const double thickness, const float color[4]);

private:

    inline static mjvScene* ptrDebugDrawingsScene;  // pointer to the mujoco scene where to draw the debug drawings
    inline static std::vector<mjvGeom> geomsVector; // vector that stores the debug drawings to be drawn
    inline static std::vector<int> idsVector;   // vector that stores the ids of the debug drawings to be drawn

    /**
     * @brief drawRegularGeom: draw a regular geometric primitive
     * @param type: type of the geometric primitive
     * @param size: size of the geometric primitive
     * @param pos: position of the geometric primitive
     * @param color: RGBA color of the geometric primitive
     */
    static void drawRegularGeom(mjtGeom type, const double size[3], const double pos[3], const float color[4]);

    /**
     * @brief drawRenderOnlyGeom: draw a rendering-only geometric primitive
     * @param type: type of the geometric primitive
     * @param size: size of the geometric primitive
     * @param start: start position of the geometric primitive
     * @param end: end position of the geometric primitive
     * @param width: width of the geometric primitive
     * @param color: RGBA color of the geometric primitive
     */
    static void drawRenderOnlyGeom(mjtGeom type, const double size[3], const double start[3], const double end[3], const double width, const float color[4]);

    /**
     * @brief move the idLocal draw
     */
    static bool moveGeom(int idLocal, const double center[3]);
    static bool moveGeom(int idLocal, const double start[3], const double end[3]);
};

} // namespace spqr
