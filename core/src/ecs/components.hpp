#pragma once

#include "core/src/pch.h"

#include "../common.hpp"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};

struct LineVertex {
	glm::vec3 position;
	glm::vec4 color;
};

//TODO stop using as a component use it only for entity creation
struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);

};

struct Position {
	glm::vec3 position = glm::vec3(1);
};

enum class PipelineType {
	Vertex,
	PhysicsDebug,
	LineVertex,

};

struct LinearVelocity {
	glm::vec3 position = glm::vec3(1);
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
struct Draw {
	std::function<void()> draw;
};


struct JoltCharacter {
	JPH::Character* characterPtr = nullptr;
};

struct JoltRagdoll {
	JPH::Ragdoll* ragdollPtr = nullptr;
};

struct JoltAnimation {
	JPH::SkeletalAnimation* animationPtr = nullptr;
};

struct JoltPose {
	JPH::SkeletonPose* posePtr = nullptr;
};

struct AnimationTime {
	float time = 0.0f;
};

//TODO rename CallbackComponent
struct Callback {
	std::function<void()> callbackFunction;
};

enum class GameLoadedState { NotLoaded, Loaded, Failed };
enum class MenuState { MAIN, OPTIONS, PAUSE, NONE };
enum class CameraState { PLAYER, FREECAM, NONE };
enum class PlayState {PLAY,PAUSE, NONE};
enum class EditorState {Enabled,Disabled, NONE};
enum class InputDeviceState {KBM,CONTROLLER};

enum class UICommandType {
	NewGame,
	SaveGame,
	LoadGame,
	RestartLevel,
	ResumeGame,
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

struct RenderPipeline {};

struct ActiveCamera {};

struct MenuComponent {};
struct HudComponent {};
struct EditorComponent {};
struct OverlayComponent {};
struct Active{};

struct IsActive {};

struct PhysicsSystemRef {
	JPH::PhysicsSystem & physicsSystem;
};

struct Game {};
struct _Scene {};

struct PlayerRef { flecs::entity value = flecs::entity::null(); };
struct PlayerCamRef { flecs::entity value = flecs::entity::null(); };

struct ModelSourceRef {
	std::string name;
};

struct CameraMVMTState {
	bool locked = false;
};

struct ObjectType {
	std::string name; 
};

enum class EntityType {
	Empty,
	Game,
	Scene,
	Player,
	Humanoid,
	Ragdoll,
	RobotArm,
	Snake,
	Actor,
	Capsule,
	Grid,
	StaticMesh,
	Sphere,
	Sensor,
	Cube,
	Light,
	Camera,
	COUNT
};