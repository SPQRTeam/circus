#include "MujocoContext.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>

namespace spqr {
MujocoContext::MujocoContext(const std::string& xmlString) {
	char error[1024] = {0};
	std::filesystem::current_path(PROJECT_ROOT);

	std::unique_ptr<mjSpec, std::function<void(mjSpec*)> > spec(
	    mj_parseXMLString(xmlString.c_str(), nullptr, error, sizeof(error)),
	    [](mjSpec* s) { mj_deleteSpec(s); });

	if (!spec) {
		throw std::runtime_error(std::string("Failed to parse the generated XML. ") + error);
	}

	model = mj_compile(spec.get(), nullptr);
	if (!model) {
		throw std::runtime_error(std::string("Failed to compile mujoco specs. ") + mjs_getError(spec.get()));
	}

	data = mj_makeData(model);

	mjv_defaultOption(&opt);
	mjv_defaultCamera(&camField);
	mjv_makeScene(model, &scene, 10000);

	// Set camera to a good initial position to view the scene
	camField.azimuth = 90;     // Side view
	camField.elevation = -15;  // Slightly from above
	camField.distance = 8;     // Good distance to see the field
	camField.lookat[0] = 0;    // Center of field
	camField.lookat[1] = 0;
	camField.lookat[2] = 0.5;  // Slightly above ground
}

MujocoContext::~MujocoContext() {
	if (data)
		mj_deleteData(data);
	if (model)
		mj_deleteModel(model);
	mjv_freeScene(&scene);
}

MujocoContext& MujocoContext::operator=(MujocoContext&& other) noexcept {
	if (this != &other) {
		if (data)
			mj_deleteData(data);
		if (model)
			mj_deleteModel(model);
		mjv_freeScene(&scene);

		model = other.model;
		data = other.data;
		camField = other.camField;
		opt = other.opt;
		scene = other.scene;

		other.model = nullptr;
		other.data = nullptr;
	}
	return *this;
}
}  // namespace spqr
