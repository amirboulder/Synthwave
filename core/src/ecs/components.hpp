#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Character/Character.h>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};

//TODO stop using as a component use it only for entity creation
struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);

};


struct PlayerInput {
	glm::vec3 direction = glm::vec3(0);
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	bool jump = false;
};

//TODO use this
struct PlayerInput2 {
	JPH::Vec3 direction = JPH::Vec3::sZero();
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	bool jump = false;
};


struct RenderState {
	flecs::entity activePipeline;
};

struct PipelineRef {
	flecs::entity pipeline;
};


struct SensorBehavior {
	std::function<void(flecs::world& ecs,flecs::entity self, flecs::entity other)> onContactAdded;
};

struct ActorBehavior {

	std::function<void(flecs::world& ecs, flecs::entity self)> actorUpdate;

};

struct HudRender {
	std::function<void(flecs::world& ecs)> draw;
};

struct Render {
	std::function<void(flecs::world& ecs)> draw;
};

struct JoltCharacter {
	JPH::Character* characterPtr = nullptr;
};

struct Callback {
	std::function<void()> callbackFunction;
};


// Tags 
struct DynamicEnt {};
struct StaticEnt {};
struct Sensor {};

struct CustomPipeline {};

struct ActiveCamera {};


struct MenuItem {};
struct Active{};