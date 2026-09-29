module;

#include <functional>
#include <string>

export module Components;

import Flecs;

export struct FrameCounter {
	uint64_t count = 0;
};



export struct Draw {
	std::function<void()> draw;
};


//TODO rename CallbackComponent
export struct Callback {
	std::function<void()> callbackFunction;
};

export enum class GameLoadedState { NotLoaded, Loaded, Failed };
export enum class MenuState { MAIN, OPTIONS, PAUSE, NONE };
export enum class CameraState { PLAYER, FREECAM, NONE };
export enum class PlayState {PLAY,PAUSE, NONE};
export enum class EditorState {Enabled,Disabled, NONE};
export enum class InputDeviceState {KBM,CONTROLLER};

export enum class UICommandType {
	NewGame,
	SaveGame,
	LoadGame,
	RestartLevel,
	ResumeGame,
	GameOptions,
	MainMenu,
	ExitGame
};

export struct UICommand {
	UICommandType type;
};

// Tags 
export struct DynamicEnt {};
export struct StaticEnt {};
export struct Sensor {};

export struct MenuComponent {};
export struct HudComponent {};
export struct EditorUIComponent {};
export struct OverlayComponent {};
export struct Active{};

export struct HasScript {};

/// <summary>
/// A Tag attached to objects that should only be rendered in while editor is enabled.
/// Used by renderer queries.
/// </summary>
export struct EditorMesh {};



export struct IsActive {};



//////////////////////////////////////////////

export struct Game {};
export struct _Scene {};





/// <summary>
/// Used for locking the camera
/// </summary>
export struct CameraMVMTState {
	bool locked = false;
};

export struct ObjectType {
	std::string name; 
};


/// <summary>
/// Used for Serialization
/// </summary>
export enum class EntityType {
	Empty,
	Generic,

	Game,
	Scene,
	Player,
	Camera,

	//Generated Meshes
	Cube,
	Capsule,
	Sphere,
	Cylinder,

	ProgrammaticRagdoll,
	BuiltRagdoll,

	Ragdoll,
	RagdollStatic,
	RagdollDynamic,
	RagdollKinematic,

	RagdollForce,
	RagdollCharacterController,

	RobotArm,
	Snake,

	Actor,
	
	Grid,
	StaticMesh,
	BoxCar, // For testing Mesh hierarchy 
	Mountain,

	Sensor,
	
	Light,

	COUNT
};



export enum class EnemyState {

	SLEEP, 
	IDLE,
	SEARCH, //Know the player exists but unaware of the location looks/walks around randomly
	CHASE, //knows player location and is running towards but out of punching range
	FIGHT, //Withing punching range of player engaging in combat
	DISABLED, //The AI system for this robot is temporarily turned off
	BROKEN, //The robot is disconnected from it character controller but still performing its animation.
	CRAWLING, //(MAYBE) The robots legs are blown of but it will still chase slowly using arms
	PARALYSED, // The robot cannot chase but can still spot the player
	CORPSE, // Enough damage is taken to destroy the Robot

};


export struct ActorBehavior {

	std::function<void(flecs::world& ecs, flecs::entity self)> actorUpdate;

};

//These two are the same thing get rid of one
//TODO find a better name for this
export struct HudRender {
	std::function<void(flecs::world& ecs)> draw;
};
export struct Render {
	std::function<void(flecs::world& ecs)> draw;
};


//TODO MOVE THIS
export struct HighlightedEntRef {
	flecs::entity ent;
};


// Added to a component's entity (not to game entities) to keep that component out of save files.
// For runtime-only or derived data that the factory rebuilds on load.
export struct DontSerialize {};