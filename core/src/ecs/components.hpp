#pragma once


#include "../common.hpp"
#include "GraphicsComponents.hpp"



struct Position {
	glm::vec3 position = glm::vec3(1);
};



struct LinearVelocity {
	glm::vec3 position = glm::vec3(1);
};

/// <summary>
/// Created by InputManager and consumed by player and freeCam.
/// Data is reset when camera switches.
/// </summary>
struct UserInput {
	glm::vec2 direction = glm::vec2(0);
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	float magnitude = 0.0f;         // 0-1, for speed scaling
	bool jump = false;
	bool jumpConsumed = true; //TODO we can store multiple bools in an int if we have many
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



struct MenuComponent {};
struct HudComponent {};
struct EditorUIComponent {};
struct OverlayComponent {};
struct Active{};

/// <summary>
/// A Tag attached to objects that should only be rendered in while editor is enabled.
/// Used by renderer queries.
/// </summary>
struct EditorMesh {};



struct IsActive {};

//=============================================
// Physics
//=============================================

/// <summary>
/// Reference to the physics system which allows other system query it from the ECS
/// instead of having to pass around references.
/// </summary>
struct PhysicsSystemRef {
	JPH::PhysicsSystem & physicsSystem;
};

struct PhysicsBody {
	JPH::BodyID ID;
};

struct PhysicsBodyGroup {
	std::vector<JPH::BodyID> IDs;
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

//////////////////////////////////////////////

struct Game {};
struct _Scene {};

struct PlayerRef { flecs::entity value = flecs::entity::null(); };
struct PlayerCamRef { flecs::entity value = flecs::entity::null(); };


struct ModelSourceName {
	std::string name;
};

/// <summary>
/// Used for locking the camera
/// </summary>
struct CameraMVMTState {
	bool locked = false;
};

struct ObjectType {
	std::string name; 
};

struct HighlightedEntRef {
	flecs::entity ent;
};

/// <summary>
/// Used for Serialization
/// </summary>
enum class EntityType {
	Empty,
	Game,
	Scene,
	Car,
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


struct EntityTypeComponent {
	EntityType type;
};