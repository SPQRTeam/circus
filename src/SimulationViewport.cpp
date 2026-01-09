#include "SimulationViewport.h"

#include <mujoco/mjvisualize.h>
#include <qnamespace.h>
#include <qpoint.h>

#include <cmath>

#include "RobotManager.h"
#include "sensors/Sensor.h"

namespace spqr {

SimulationViewport::SimulationViewport(MujocoContext& mujContext)
    : model(mujContext.model), data(mujContext.data), cam(&mujContext.cam), opt(&mujContext.opt), scene(&mujContext.scene), context(&mujContext.ctx) {
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&SimulationViewport::update));
    timer->start(16);
    mjv_defaultPerturb(&pert);

    this->targetDistance = 15.0f;
    this->targetAzimuth = 90.f;
    this->targetElevation = -45.f;
    this->targetLookat[0] = 0;
    this->targetLookat[1] = -1;
    this->targetLookat[2] = 0;

    // Set initial camera position for animation (far away, higher elevation)
    cam->distance = this->targetDistance * 5.0f;  // Start 5x farther
    cam->elevation = -20.0f;                      // Start from lower angle
    cam->azimuth = this->targetAzimuth - 360.0f;  // Start 360 degrees before target (full rotation)
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
    // Update camera animation
    if (cameraAnimating) {
        animationTime += 0.016f;  // Assume ~60 FPS (16ms per frame)

        if (animationTime >= animationDuration) {
            // Animation complete
            cameraAnimating = false;
            cam->distance = targetDistance;
            cam->azimuth = targetAzimuth;
            cam->elevation = targetElevation;
            cam->lookat[0] = targetLookat[0];
            cam->lookat[1] = targetLookat[1];
            cam->lookat[2] = targetLookat[2];
        } else {
            // Smooth easing function (ease-in-out cubic)
            float t = animationTime / animationDuration;
            float eased = t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;

            // Initial values
            float startDistance = targetDistance * 10.0f;
            float startElevation = -40.0f;
            float startAzimuth = targetAzimuth - 360.0f;

            // Interpolate camera parameters
            cam->distance = startDistance + (targetDistance - startDistance) * eased;
            cam->elevation = startElevation + (targetElevation - startElevation) * eased;
            cam->azimuth = startAzimuth + (targetAzimuth - startAzimuth) * eased;
            cam->lookat[0] = targetLookat[0];
            cam->lookat[1] = targetLookat[1];
            cam->lookat[2] = targetLookat[2];
        }
    }

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
    mjv_moveCamera(model, mjMOUSE_ZOOM, 0, -0.0005 * event->angleDelta().y(), scene, cam);
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
            }
        }

        if (event->modifiers() & Qt::ShiftModifier) {
            mouseAction = mjMOUSE_ROTATE_V;
        } else {
            mouseAction = mjMOUSE_MOVE_H;
        }
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
        mjtNum reldy = (mjtNum)(delta.y() / (float)logicalHeight);  // note sign

        if (mouseAction == mjMOUSE_ROTATE_V) {
            mjtNum qz[4];
            mjtNum axis[3] = {0, 0, 1};

            mjtNum amp = mju_sqrt(reldx * reldx + reldy * reldy);
            mjtNum sgn = mju_max(mju_abs(reldx), mju_abs(reldy)) == mju_abs(reldx) ? mju_sign(reldx) : -mju_sign(reldy);

            mjtNum totalRotation = amp * sgn;
            mju_axisAngle2Quat(qz, axis, totalRotation);
            mju_mulQuat(pert.refquat, qz, pert.refquat);
        } else if (mouseAction == mjMOUSE_MOVE_H) {
            mjv_movePerturb(model, data, mouseAction, reldx, reldy, scene, &pert);
        }
        mjv_applyPerturbPose(model, data, &pert, /*flg_paused=*/1);
    } else {
        mjv_moveCamera(model, mouseAction, 0.003 * delta.x(), 0.003 * delta.y(), scene, cam);
    }

    lastMousePosition = event->position();
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

}  // namespace spqr
