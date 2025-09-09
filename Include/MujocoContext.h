#pragma once

#include <mujoco/mujoco.h>

#include <string>

#include "RobotManager.h"
#include "SceneParser.h"

namespace spqr {

struct MujocoContext {
	mjModel* model = nullptr;
	mjData* data = nullptr;
	mjvCamera camField{};
	mjvOption opt{};
	mjvScene scene{};

	MujocoContext(const std::string& xmlString);
	~MujocoContext();

	// Copying could potentially lead to freeing the model or data twice.
	// Deleting the copy constructors prevents this.
	MujocoContext(const MujocoContext&) = delete;
	MujocoContext& operator=(const MujocoContext&) = delete;

	MujocoContext& operator=(MujocoContext&& other) noexcept;
};
}  // namespace spqr
