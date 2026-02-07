#pragma once

#include <mujoco/mujoco.h>

#include <QColor>
#include <algorithm>
#include <iostream>
#include <map>
#include <msgpack.hpp>

namespace spqr {
class DebugDrawings {

   enum class drawGeomType; // forward declaration for a private member enumeration

   public:
    /**
     * @brief init: initialize the DebugDrawings class with the pointer to the mujoco scene where to draw the debug
     */
    static void init(mjvScene* s) noexcept;

    /**
     * @brief processDebugMessage: process the debug message received from the socket and call the appropriate drawing function at every frame
     */
    static void drawDebugDrawings();

    /**
     * @brief processDebugMessage: process the debug message received from the socket and call the appropriate drawing function
     * @param data_map: map containing the debug message data
     */
    static void processDebugMessage(std::map<std::string, msgpack::object> data_map);

    /**
     * @brief removeGeom: remove the debug figure with the given idLocal and customType
     * @param idLocal: identifier of the debug drawing
     * @param customType: custom geometric type used to select the appropriate mjvGeom
     */
    static void removeGeom(const std::string& idLocal, const drawGeomType customType);

    /**
     * @brief removeAll: remove all debug figures
     */
    static void removeAll();

    // ---------------- regular geom types (mjtGeom)
    /**
     * @brief drawCircle: initialize a mjGEOM_CIRCLE in mujoco processing a predefined message from the socker
     * with the framework
     * @param idLocal: identifier of the debug drawing
     * @param center: position of the center of the geom
     * @param radius: radius of the circle
     * @param color: RGBA color of the geom
     */
    static void drawCircle(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawSphere: initialize a mjGEOM_SPHERE in mujoco processing a predefined message from the socker
     * with the framework
    */
    static void drawSphere(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawCylinder: initialize a mjGEOM_SPHERE in mujoco processing a predefined message from the
     * socker with the framework
     */
    static void drawCylinder(const std::map<std::string, msgpack::object>& data_map);

    // ---------------- rendering-only geom types (mjtGeom)

    /**
     * @brief drawArrow: initialize a mjGEOM_ARROW in mujoco processing a predefined message from the socker
     * with the framework
    */
    static void drawArrow(const std::map<std::string, msgpack::object>& data_map);

    /**
     * @brief drawLine: initialize a mjGEOM_LINE in mujoco processing a predefined message from the socker
     * with the framework
     */
    static void drawLine(const std::map<std::string, msgpack::object>& data_map);

   private:

    // Used to create custom type to avoid errore: A circle and a Cylinder are called both with mjGEOM_CYLINDER
    enum class drawGeomType {
        Sphere,
        Cylinder,
        Circle,
        Arrow,
        Line,

        COUNT,  // keep this last
    };

    // Helper functions to converted drawGeomType in a string
    static const char* toString(drawGeomType t) {
        switch (t) {
            case drawGeomType::Sphere:
                return "Sphere";
            case drawGeomType::Cylinder:
                return "Cylinder";
            case drawGeomType::Circle:
                return "Circle";
            case drawGeomType::Arrow:
                return "Arrow";
            case drawGeomType::Line:
                return "Line";
        }
        return "Unknown";
    }

    // Helper function to check if the drawGeomType is a regular geometric primitive (i.e., defined by a standard mjtGeom type)
    constexpr bool isRegularGeom(drawGeomType t) {
        switch (t) {
            case drawGeomType::Sphere:
            case drawGeomType::Cylinder:
            case drawGeomType::Circle:
                return true;
            default:
                return false;
        }
    }

    // Struct to store the data of a debug figure
    struct GeomData {
        mjvGeom geom;
        drawGeomType customType;
    };

    inline static mjvScene* ptrDebugDrawingsScene;  // pointer to the mujoco scene where to draw the debug
                                                    // drawings
    inline static std::map<mjString, GeomData> mapIdGeom;

    static mjString getMapIdGeomLocalId(const std::string& idLocal, const drawGeomType customType);

    /**
     * @brief drawRegularGeom: draw a 'regular'-defined geometric primitive
     * @param idLocal: identifier of the debug drawing
     * @param mujocoType: standard MuJoCo geometric type (mjtGeom)
     * @param customType: custom geometric type used to select the appropriate mjvGeom
     * @param size: size of the geometric primitive
     * @param pos: position of the geometric primitive
     * @param color: color of the primitive (RGBA), can use QColorConstants.
     */
    static void drawRegularGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                const double size[3], const double pos[3], QColor color);

    /**
     * @brief drawRenderOnlyGeom: draw a 'rendering-only'-defined geometric primitive
     * @param idLocal: identifier of the debug drawing
     * @param mujocoType: standard MuJoCo geometric type (mjtGeom)
     * @param customType: custom geometric type used to select the appropriate mjvGeom
     * @param size: size of the geometric primitive
     * @param start: start position of the geometric primitive
     * @param end: end position of the geometric primitive
     * @param width: width of the geometric primitive
     * @param color: RGBA color of the geometric primitive, can be a QColorConstants
     */
    static void drawRenderOnlyGeom(mjString idLocal, mjtGeom mujocoType, drawGeomType customType,
                                   const double size[3], const double start[3], const double end[3],
                                   const double width, QColor color);

    /**
     * @brief check is the mjv_Geom already exist. If true return the right mjvGeom into mjvScene->geoms
     */

    static mjvGeom* isGeomExist(mjString idLocal, drawGeomType type);

    /**
     * @brief move the mjvGeom object
     */
    static void moveGeom(mjvGeom* geom, const double center[3], QColor color);

    /**
     * @brief move the mjvGeom object
     */
    static void moveGeom(mjvGeom* geom, const double start[3], const double end[3], QColor color);
};

}  // namespace spqr