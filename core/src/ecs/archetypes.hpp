#pragma once

#include "../renderer/Model.hpp"
#include "../physics/physics.hpp"

#include "components.hpp"

using std::vector;

// entities have a transfrom a physics body and a model
struct Entities {

	vector<Transform> transforms;
	vector<ModelInstance> models;
	vector<PhysicsData> physicsComponents;

};

