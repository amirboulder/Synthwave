#pragma once

#include "core/src/pch.h"

#include "../common.hpp"

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

//These two are the same thing get rid of one
//TODO find a better name for this
struct HudRender {
	std::function<void(flecs::world& ecs)> draw;
};
struct Render {
	std::function<void(flecs::world& ecs)> draw;
};


struct JoltCharacter {
	JPH::Character* characterPtr = nullptr;
};
//TODO rename
struct Callback {
	std::function<void()> callbackFunction;
};

struct stateChangeRequest {
	AppContext::Type newContext;
};

struct PlayState { bool play = false; };

enum class MenuState { MAIN, OPTIONS, PAUSE };

enum class UICommandType {
	NewGame,
	LoadGame,
	GameOptions,
	MainMenu,
	ExitGame
};

struct UICommand {
	UICommandType type;
};

// Tags 
struct DynamicEnt {};
struct StaticEnt {};
struct Sensor {};

struct CustomPipeline {};

struct ActiveCamera {};


struct MenuItem {};
struct Active{};