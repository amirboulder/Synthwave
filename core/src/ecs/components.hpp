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

struct JoltRagdollFilter {
	JPH::IgnoreMultipleBodiesFilter* filter = nullptr;
};

struct JoltAnimation {
	JPH::SkeletalAnimation* animationPtr = nullptr;
};

struct JoltPose {
	JPH::SkeletonPose pose;
	JPH::Vec3 root_offset;
};

struct JoltPose2 {
	JPH::SkeletonPose pose;
	float hipsFromSoles = 0.0f;
};

struct AnimationTime {
	float time = 0.0f;
};


struct PhysicsConstraint {
	JPH::Ref<JPH::SixDOFConstraint> constraint;
};

struct JoltAnchorBody {
	JPH::BodyID bodyID;
	JPH::Ref<JPH::FixedConstraint> constraint;
};

//////////////////////////////////////////////

struct Game {};
struct _Scene {};

struct PlayerRef { flecs::entity value = flecs::entity::null(); };
struct PlayerCamRef { flecs::entity value = flecs::entity::null(); };



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
	Generic,
	Game,
	Scene,
	BoxCar,
	Player,
	Humanoid,
	Ragdoll,
	RagdollForce,
	JoltRagdollExample,
	RobotArm,
	Snake,
	Actor,
	Capsule,
	Grid,
	StaticMesh,
	Mountain,
	Sphere,
	Cylinder,
	Sensor,
	Cube,
	Light,
	Camera,
	COUNT
};


// This exists to keep entities of different EntityType in the same table ie prevent fragmentation,
// but is that even desirable ???
// TODO verify this behaves as expected using flecs api
struct EntityTypeComponent {
	EntityType type;
};