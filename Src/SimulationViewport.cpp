#include "SimulationViewport.h"

#include <mujoco/mjvisualize.h>
#include <qnamespace.h>
#include <qpoint.h>

#include <iostream>

#include "Robot.h"

namespace spqr {

SimulationViewport::SimulationViewport(MujocoContext& mujContext, const RobotManager& robotManager)
    : model(mujContext.model),
      data(mujContext.data),
      camField(&mujContext.camField),
      opt(&mujContext.opt),
      scene(&mujContext.scene),
      robotManager(robotManager) {
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, QOverload<>::of(&SimulationViewport::update));
	timer->start(16);
	mjv_defaultPerturb(&pert);
}

void SimulationViewport::initializeGL() {
	mjr_defaultContext(&context);
	mjr_makeContext(model, &context, mjFONTSCALE_100);
}

void SimulationViewport::resizeGL(int w, int h) {
	const float frameBufferFactor = devicePixelRatioF();
	logicalWidth = w;
	logicalHeight = h;
	width = int(w * frameBufferFactor);
	height = int(h * frameBufferFactor);
}

/*
    camera:
        type: "realsense" # realsense or zed
        intrin:
            fx: 643.898
            fy: 643.216
            cx: 649.038
            cy: 357.21
            distortion_coeffs: [-0.0553056,0.065975,-0.000994232,2.98548e-05,-0.0216579]
            distortion_model: 2 # 0: none, 1: opencv format, 2: for rs d455 only
        extrin:
            - [ 0.05659255, 0.03014561, 0.99794215, 0.04217854]
            - [-0.99839673, 0.00283376, 0.05653273, 0.01405556]
            - [-0.00112372,-0.9995415 , 0.03025765,-0.01538294]
            - [ 0.        , 0.        , 0.        , 1.        ]
        pitch_compensation: 0.0 # in degree
        yaw_compensation: 0.0
        z_compensation: 0.0

    camera_type_ = node["camera"]["type"].as<std::string>();
        intr_ = Intrinsics(node["camera"]["intrin"]);
        p_eye2head_ = as_or<Pose>(node["camera"]["extrin"], Pose());

        float pitch_comp = as_or<float>(node["camera"]["pitch_compensation"], 0.0);
        float yaw_comp = as_or<float>(node["camera"]["yaw_compensation"], 0.0);

        p_headprime2head_ = Pose(0, 0, 0, 0, pitch_comp * M_PI / 180, yaw_comp * M_PI / 180);
*/

void SimulationViewport::paintGL() {
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mjv_updateScene(model, data, opt, &pert, camField, mjCAT_ALL, scene);

	if (selectedRobot >= 0) {
		robotManager.highlightRobot(selectedRobot, scene);
	}

	mjrRect viewport = {0, 0, width, height};
	mjr_setBuffer(mjFB_WINDOW, &context);
	mjr_render(viewport, scene, &context);

	// picture in picture for the robot camera --> we'll need different scenes later one
	int pipWidth = int(0.15 * width);
	int pipHeight = int(pipWidth * 9.0 / 16.0);  // 16:9 aspect ratio
	pipHeight = std::min(pipHeight, height / 2);

	for (int i = 0; i < robotManager.numRobots(); i++) {
		const Robot* robot = robotManager.get(i);  // non-const to allow camera modification

		mjrRect pip{};

		// left camera
		pip = {width - pipWidth, height - pipHeight - i * (pipHeight + 10), pipWidth, pipHeight};
		mjv_updateScene(model, data, opt, nullptr, const_cast<mjvCamera*>(&robot->leftCam), mjCAT_ALL, scene);
		mjr_render(pip, scene, &context);

		// right camera
		pip = {width - 2 * pipWidth - 10, height - pipHeight - i * (pipHeight + 10), pipWidth, pipHeight};
		mjv_updateScene(model, data, opt, nullptr, const_cast<mjvCamera*>(&robot->rightCam), mjCAT_ALL, scene);
		mjr_render(pip, scene, &context);
	}

	// fixes the drag and drop of the field camera
	mjv_updateScene(model, data, opt, nullptr, camField, mjCAT_ALL, scene);
}

void SimulationViewport::wheelEvent(QWheelEvent* event) {
	mjv_moveCamera(model, mjMOUSE_ZOOM, 0, -0.0005 * event->angleDelta().y(), scene, camField);
}

void SimulationViewport::mousePressEvent(QMouseEvent* event) {
	lastMousePosition = event->position();
	selectedRobot = -1;

	if (event->button() == Qt::LeftButton) {
		float relx = event->position().x() / logicalWidth;
		float rely = 1.0 - event->position().y() / logicalHeight;  // Flip Y for MuJoCo
		int selectedBody = selectBody(relx, rely);
		selectedRobot = robotManager.rootBodyIndex(selectedBody);

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
			mjtNum sgn
			    = mju_max(mju_abs(reldx), mju_abs(reldy)) == mju_abs(reldx) ? mju_sign(reldx) : -mju_sign(reldy);

			mjtNum totalRotation = amp * sgn;
			mju_axisAngle2Quat(qz, axis, totalRotation);
			mju_mulQuat(pert.refquat, qz, pert.refquat);
		} else if (mouseAction == mjMOUSE_MOVE_H) {
			mjv_movePerturb(model, data, mouseAction, reldx, reldy, scene, &pert);
		}
		mjv_applyPerturbPose(model, data, &pert, /*flg_paused=*/1);
	} else {
		mjv_moveCamera(model, mouseAction, 0.003 * delta.x(), 0.003 * delta.y(), scene, camField);
	}

	lastMousePosition = event->position();
}

int SimulationViewport::selectBody(float relx, float rely) const {
	if (!model || !data || !opt || !scene || height == 0)
		return -1;

	mjtNum selpnt[3];
	int geomid = -1, flexid = -1, skinid = -1;
	mjtNum aspect = (mjtNum)width / (mjtNum)height;
	int bodyid = mjv_select(model, data, opt, aspect, (mjtNum)relx, (mjtNum)rely, scene, selpnt, &geomid,
	                        &flexid, &skinid);

	return bodyid;
}

}  // namespace spqr
