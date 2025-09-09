#include "RobotManager.h"

#include <iostream>

#include "Robot.h"

namespace spqr {

RobotManager::RobotManager(const mjModel* m, const SceneInfo& sceneSpec) {
	robots.clear();
	bodyToRobot.assign(m->nbody, -1);

	// Create robots from static info
	int robotIndex = 0;
	for (const TeamInfo& team : sceneSpec.teams) {
		for (const RobotInfo& robotInfo : team.robots) {
			Robot robot;
			robot.index = robotIndex;
			robot.info = robotInfo;
			robot.rootBodyId = -1;  // will be set later

			robot.leftCam.type = mjCAMERA_FIXED;
			robot.leftCam.fixedcamid = mj_name2id(m, mjOBJ_CAMERA, (robot.info.name + "_left_cam").c_str());
			robot.rightCam.type = mjCAMERA_FIXED;
			robot.rightCam.fixedcamid = mj_name2id(m, mjOBJ_CAMERA, (robot.info.name + "_right_cam").c_str());

			robots.push_back(robot);
			robotIndex++;
		}
	}

	// Build body lists for each robot
	for (int b = 0; b < m->nbody; ++b) {
		const char* bodyName = mj_id2name(m, mjOBJ_BODY, b);
		int jntadr = m->body_jntadr[b];
		if (bodyName && jntadr >= 0) {
			std::string name(bodyName);
			if (name == "ball")
				ballBodyId = b;  // TODO: hardcoded string to "ball"
			for (Robot& robot : robots) {
				if (robot.ownsBody(name)) {
					robot.bodyIds.push_back(b);
					bodyToRobot[b] = robot.index;
					if (m->jnt_type[jntadr] == mjJNT_FREE) {
						robot.rootBodyId = b;
					}
				}
			}
		}
	}

	// Build geom lists for each robot
	for (int g = 0; g < m->ngeom; ++g) {
		int bodyId = m->geom_bodyid[g];
		int robotIdx = robotIndexByBody(bodyId);
		if (robotIdx >= 0) {
			robots[robotIdx].geomIds.push_back(g);
		}
	}
}

int RobotManager::robotIndexByBody(int bodyId) const {
	if (bodyId < 0 || bodyId >= bodyToRobot.size()) {
		return -1;
	}
	return bodyToRobot[bodyId];
}

int RobotManager::robotIndexByGeom(int geomId, const mjModel* m) const {
	if (geomId < 0 || geomId >= bodyToRobot.size()) {
		return -1;
	}
	int bodyId = m->geom_bodyid[geomId];
	return robotIndexByBody(bodyId);
}

int RobotManager::rootBodyIndex(int bodyId) const {
	if (bodyId < 0 || bodyId >= bodyToRobot.size()) {
		return -1;
	}
	int robotIndex = robotIndexByBody(bodyId);
	if (robotIndex != -1) {
		return get(robotIndex)->rootBodyId;
	}
	return -1;
}

const Robot* RobotManager::get(int idx) const {
	if (idx < 0 || idx >= robots.size()) {
		return nullptr;
	}
	return &robots[idx];
}

void RobotManager::highlightRobot(int bodyId, mjvScene* scene) const {
	const Robot* robot = get(robotIndexByBody(bodyId));

	if (!robot || !scene) {
		std::cout << "NO ROBOT SELECTED" << std::endl;
		return;
	}

	for (int geomid : robot->geomIds) {
		for (int i = 0; i < scene->ngeom; ++i) {
			if (scene->geoms[i].objid == geomid) {
				mjvGeom& gw = scene->geoms[i];
				gw.rgba[0] = std::min(1.0f, gw.rgba[0] + 0.2f);
				gw.rgba[1] = std::max(0.0f, gw.rgba[1] - 0.05f);
				gw.rgba[2] = std::max(0.0f, gw.rgba[2] - 0.05f);
				gw.rgba[3] = 0.7f;
				break;
			}
		}
	}
}
}  // namespace spqr
