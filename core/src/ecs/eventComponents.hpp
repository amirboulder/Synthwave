#pragma once

struct ActionState {
	float heldTime = 0.0f;   // accumulates while down, useful for charge-up
	bool occurred = false;   // currently held
	bool occurredLast = false;   // held last frame persisted
	bool justPressed = false;  // true for exactly one frame
	bool justReleased = false;  // true for exactly one frame
};

struct MouseMovementState {

	float deltaX, deltaY;
};

struct MouseClickLeftEvent {

	float x = 0;
	float y = 0;
};

struct ExitEvent {
	bool occurred = false;
};

struct WindowLostFocusEvent {
	bool occurred = false;
};

struct GamePauseEvent {
	bool occurred = false;
};

struct EditorToggleEvent{
	bool occurred = false;
};

struct CameraSwitchEvent {
	bool occurred = false;
};

struct PhysicsRenderToggleEvent {
	bool occurred = false;
};

struct SaveGameSrcEvent {
	bool occurred = false;
};

struct RagdollSavedEvent {
	bool occurred = false;
};

struct PrintSystemsEvent {
	bool occurred = false;
};

struct PrintPhasesEvent {
	bool occurred = false;
};