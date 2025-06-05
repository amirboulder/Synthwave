#pragma once

#include "renderer/Model.hpp"
#include "physics/physics.hpp"

#include "components.hpp"

using std::vector;

// Dynamic entities have a transfrom a physics body and a model
struct DynamicEntities {

	vector<TransformData2> transforms;
	vector<Model> models;
	vector<PhysicsData> physicsCompoments;

};

struct StaticEntities {

	vector<TransformData2> transforms;
	vector<Model> models;
	vector<PhysicsData> physicsCompoments;

};


