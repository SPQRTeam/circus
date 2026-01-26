#include "SimulationViewport.h"

#include <mujoco/mjvisualize.h>
#include <qevent.h>
#include <qnamespace.h>
#include <qpoint.h>

#include "RobotManager.h"
#include "sensors/Sensor.h"

namespace spqr {

SimulationViewport::SimulationViewport(MujocoContext& mujContext)
    : model(mujContext.model), data(mujContext.data), cam(&mujContext.cam), opt(&mujContext.opt), scene(&mujContext.scene), context(&mujContext.ctx) {
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&SimulationViewport::update));
    timer->start(16);
    mjv_defaultPerturb(&pert);

    cam->distance = 15.f;
    cam->azimuth = 90.f;
    cam->elevation = -45.f;
    cam->lookat[0] = 0.f;
    cam->lookat[1] = -1.f;
    cam->lookat[2] = 0.f;
}

void SimulationViewport::initializeGL() {
    mjr_defaultContext(context);
    mjr_makeContext(model, context, mjFONTSCALE_100);
}

void SimulationViewport::resizeGL(int w, int h) {
    const float frameBufferFactor = devicePixelRatioF();
    logicalWidth = w;
    logicalHeight = h;
    width = int(w * frameBufferFactor);
    height = int(h * frameBufferFactor);
}

void SimulationViewport::paintGL() {
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render main viewport
    mjrRect viewport = {0, 0, width, height};
    mjr_setBuffer(mjFB_WINDOW, context);
    mjv_updateScene(model, data, opt, nullptr, cam, mjCAT_ALL, scene);
    mjr_render(viewport, scene, context);

    // Render cameras offscreen and save images
    for (int i = 0; i < RobotManager::instance().getRobots().size(); ++i) {
        auto robot = RobotManager::instance().getRobots()[i];

        std::map<std::string, Sensor*> sensors = robot->getSensors();
        Camera* leftCamera = dynamic_cast<Camera*>(sensors["rgb_left_camera"]);
        Camera* rightCamera = dynamic_cast<Camera*>(sensors["rgb_right_camera"]);

        if (!leftCamera || !rightCamera)
            continue;

        // Render and capture camera images (save every 60 frames)
        leftCamera->render();
        rightCamera->render();
    }

    // Restore main viewport scene
    mjv_updateScene(model, data, opt, nullptr, cam, mjCAT_ALL, scene);
}

void SimulationViewport::wheelEvent(QWheelEvent* event) {
    mjv_moveCamera(model, mjMOUSE_ZOOM, 0, 0.0005 * event->angleDelta().y(), scene, cam);
}

void SimulationViewport::mousePressEvent(QMouseEvent* event) {
    lastMousePosition = event->position();
    selectedRobot = -1;

    if (event->button() == Qt::LeftButton) {
        float relx = event->position().x() / logicalWidth;
        float rely = 1.0 - event->position().y() / logicalHeight;  // Flip Y for MuJoCo
        int selectedBody = selectBody(relx, rely);
        selectedRobot = findBodyRoot(selectedBody);

        if (selectedBody >= 0) {
            pert.select = selectedRobot;
            mjv_initPerturb(model, data, scene, &pert);

            if (event->modifiers() & Qt::ShiftModifier) {
                pert.active = mjPERT_ROTATE;
                mouseAction = mjMOUSE_ROTATE_V;  // use rotate mouse action when moving
            } else {
                pert.active = mjPERT_TRANSLATE;
                mouseAction = mjMOUSE_MOVE_H;  // use horizontal-plane move when moving

                // Store the Z height of the object for planar dragging
                dragPlaneZ = data->xpos[selectedRobot * 3 + 2];

                // Compute the offset from click point to object center
                mjtNum clickPos[3];
                if (screenToWorldPlane(relx, rely, dragPlaneZ, clickPos)) {
                    dragOffset[0] = data->xpos[selectedRobot * 3 + 0] - clickPos[0];
                    dragOffset[1] = data->xpos[selectedRobot * 3 + 1] - clickPos[1];
                } else {
                    dragOffset[0] = 0.0;
                    dragOffset[1] = 0.0;
                }
            }
        } else {  // i.e. selected_body < 0
            if (event->modifiers() & Qt::ShiftModifier) {
                mouseAction = mjMOUSE_ROTATE_V;
            } else {
                mouseAction = mjMOUSE_MOVE_H;
            }
        }
    } else if (event->button() == Qt::MiddleButton) {
        mouseAction = mjMOUSE_ROTATE_V;
    }
}

void SimulationViewport::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    mouseAction = mjMOUSE_NONE;
    selectedRobot = -1;
}

void SimulationViewport::mouseMoveEvent(QMouseEvent* event) {
    if (mouseAction == mjMOUSE_NONE)
        return;

    const QPointF delta = event->position() - lastMousePosition;

    if (pert.select > 0 && pert.active) {
        mjtNum reldx = (mjtNum)(delta.x() / (float)logicalHeight);
        mjtNum reldy = (mjtNum)(delta.y() / (float)logicalHeight);

        if (mouseAction == mjMOUSE_ROTATE_V) {
            mjtNum qz[4];
            mjtNum axis[3] = {0, 0, 1};

            mjtNum amp = mju_sqrt(reldx * reldx + reldy * reldy);
            mjtNum sgn = mju_max(mju_abs(reldx), mju_abs(reldy)) == mju_abs(reldx) ? mju_sign(reldx) : -mju_sign(reldy);

            mjtNum totalRotation = amp * sgn;
            mju_axisAngle2Quat(qz, axis, totalRotation);
            mju_mulQuat(pert.refquat, qz, pert.refquat);
            mjv_applyPerturbPose(model, data, &pert, /*flg_paused=*/1);
        } else if (mouseAction == mjMOUSE_MOVE_H) {
            // Compute world position under cursor on the drag plane
            float relx = event->position().x() / logicalWidth;
            float rely = 1.0 - event->position().y() / logicalHeight;
            mjtNum worldPos[3];
            if (screenToWorldPlane(relx, rely, dragPlaneZ, worldPos)) {
                // Set the object position directly (with offset to maintain grab point)
                pert.refpos[0] = worldPos[0] + dragOffset[0];
                pert.refpos[1] = worldPos[1] + dragOffset[1];
                // Keep Z at the drag plane height
                pert.refpos[2] = dragPlaneZ;
                mjv_applyPerturbPose(model, data, &pert, /*flg_paused=*/1);
            }
        } else if (mouseAction == mjMOUSE_MOVE_V || mouseAction == mjMOUSE_ROTATE_H) {
            mjv_movePerturb(model, data, mouseAction, reldx, reldy, scene, &pert);
            mjv_applyPerturbPose(model, data, &pert, /*flg_paused=*/1);
        }
    } else {
        mjv_moveCamera(model, mouseAction, 0.003 * delta.x(), 0.003 * delta.y(), scene, cam);
    }

    lastMousePosition = event->position();
}

void SimulationViewport::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_G) {
        if (selectedRobot >= 0) {
            pert.active = mjPERT_TRANSLATE;
            mouseAction = mjMOUSE_MOVE_H;
        }
    }
    if (event->key() == Qt::Key_R) {
        if (selectedRobot >= 0) {
            pert.active = mjPERT_ROTATE;
            mouseAction = mjMOUSE_ROTATE_V;
        }
    }
    if (event->key() == Qt::Key_H) {
        if (mouseAction == mjMOUSE_MOVE_V)
            mouseAction = mjMOUSE_MOVE_H;
        if (mouseAction == mjMOUSE_ROTATE_V)
            mouseAction = mjMOUSE_ROTATE_H;
    }
    if (event->key() == Qt::Key_V) {
        if (mouseAction == mjMOUSE_MOVE_H)
            mouseAction = mjMOUSE_MOVE_V;
        if (mouseAction == mjMOUSE_ROTATE_H)
            mouseAction = mjMOUSE_ROTATE_V;
    }
}

int SimulationViewport::findBodyRoot(int bodyId) const {
    int root = bodyId;
    while (model->body_parentid[root] != 0)
        root = model->body_parentid[root];
    return root;
}

int SimulationViewport::selectBody(float relx, float rely) const {
    if (!model || !data || !opt || !scene || !cam || height == 0)
        return -1;

    mjtNum selpnt[3];
    int geomid = -1, flexid = -1, skinid = -1;
    mjtNum aspect = (mjtNum)width / (mjtNum)height;
    int bodyid = mjv_select(model, data, opt, aspect, (mjtNum)relx, (mjtNum)rely, scene, selpnt, &geomid, &flexid, &skinid);

    return bodyid;
}

bool SimulationViewport::screenToWorldPlane(float relx, float rely, mjtNum planeZ, mjtNum worldPos[3]) const {
    if (!model || !scene || !cam || height == 0)
        return false;

    // Get camera position and orientation from the scene (convert float to mjtNum)
    mjtNum camPos[3], camForward[3], camUp[3], camRight[3];
    for (int i = 0; i < 3; i++) {
        camPos[i] = scene->camera[0].pos[i];
        camForward[i] = scene->camera[0].forward[i];
        camUp[i] = scene->camera[0].up[i];
    }
    mju_cross(camRight, camForward, camUp);

    // Compute the field of view
    mjtNum fovy = model->vis.global.fovy * (M_PI / 180.0);
    mjtNum aspect = (mjtNum)width / (mjtNum)height;
    mjtNum tanFovY = mju_tan(fovy / 2.0);
    mjtNum tanFovX = tanFovY * aspect;

    // Convert relative screen coords to normalized device coords (-1 to 1)
    mjtNum ndcX = 2.0 * relx - 1.0;
    mjtNum ndcY = 2.0 * rely - 1.0;

    // Compute ray direction in world space
    mjtNum rayDir[3];
    for (int i = 0; i < 3; i++) {
        rayDir[i] = camForward[i] + ndcX * tanFovX * camRight[i] + ndcY * tanFovY * camUp[i];
    }
    mju_normalize3(rayDir);

    // Intersect ray with horizontal plane at z = planeZ
    // Ray: P = camPos + t * rayDir
    // Plane: z = planeZ
    // Solve: camPos[2] + t * rayDir[2] = planeZ
    if (mju_abs(rayDir[2]) < 1e-9)
        return false;  // Ray is parallel to the plane

    mjtNum t = (planeZ - camPos[2]) / rayDir[2];
    if (t < 0)
        return false;  // Intersection is behind the camera

    worldPos[0] = camPos[0] + t * rayDir[0];
    worldPos[1] = camPos[1] + t * rayDir[1];
    worldPos[2] = planeZ;

    return true;
}

}  // namespace spqr
