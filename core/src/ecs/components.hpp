#pragma once

//All general components should live here
//All components that rely on third-party dependencies should not go here. (STL is ok)

struct FrameCounter {
	uint64_t count = 0;
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

struct HasScript {};

/// <summary>
/// A Tag attached to objects that should only be rendered in while editor is enabled.
/// Used by renderer queries.
/// </summary>
struct EditorMesh {};



struct IsActive {};



//////////////////////////////////////////////

struct Game {};
struct _Scene {};





/// <summary>
/// Used for locking the camera
/// </summary>
struct CameraMVMTState {
	bool locked = false;
};

struct ObjectType {
	std::string name; 
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
	RagdollKinematic,
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


enum class EnemyState {

	SLEEP, //UnAware the player exits Standing Pose
	IDLE,
	SEARCH, //Know the player exists but unaware of the location looks/walks around randomly
	CHASE, //know player location and is running towards out of punching range
	FIGHT, //Withing punching range of player engaging in combat
	DISABLED, //The AI system for this robot is temporarily turned off
	CRAWLING, //(MAYBE) The robots legs are blown of but it will still chase slowly using arms
	PARALYSED, // The robot cannot chase but can still spot the player
	CORPSE, // Enough damage is taken to destroy the Robot

};

