#pragma once

#include <mujoco/mujoco.h>

#include "SceneParser.h"

namespace spqr {

struct Robot {
	int index = -1;            // Index in the list of robots
	RobotInfo info;            // Information from sceneParser
	int rootBodyId;            // body if of the root (for mjData)
	std::vector<int> bodyIds;  // all body ids belonging to this robot
	std::vector<int> geomIds;  // all geom ids belonging to this robot

	mjvCamera leftCam{};
	mjvCamera rightCam{};

	const mjtNum* worldPos(const mjData* d) const {
		return d->xpos + 3 * rootBodyId;
	}

	bool ownsGeom(int geomId) const {
		return std::find(geomIds.begin(), geomIds.end(), geomId) != geomIds.end();
	}

	bool ownsBody(int bodyId) const {
		return std::find(bodyIds.begin(), bodyIds.end(), bodyId) != bodyIds.end();
	}

	bool ownsBody(const std::string& name) const {
		std::string prefix = info.name + "_";
		return name.compare(0, prefix.size(), prefix) == 0;
	}
};

}  // namespace spqr
