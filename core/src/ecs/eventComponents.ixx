module;

#include <cstdint>

export module EventComponents;

export struct ActionState {
	int64_t frameStamp = 0;
	float heldTime = 0.0f;   // accumulates while down, useful for charge-up
	bool occurred = false;   // currently held
	bool latch = false;   //  triggered by sdlEvents,used for missed events
	bool occurredLast = false;   // held last frame / persisted
	bool justPressed = false;  // true for exactly one frame
	bool justReleased = false;  // true for exactly one frame
};

export struct MouseMovementState {

	float deltaX, deltaY;
};

export struct MouseClickLeftEvent {

	float x = 0;
	float y = 0;
};

export struct ExitEvent {
	bool occurred = false;
};

export struct WindowLostFocusEvent {
	bool occurred = false;
};

export struct GamePauseEvent {
	bool occurred = false;
};

export struct EditorToggleEvent{
	bool occurred = false;
};

export struct CameraSwitchEvent {
	bool occurred = false;
};

export struct PhysicsRenderToggleEvent {
	bool occurred = false;
};

export struct SaveGameSrcEvent {
	bool occurred = false;
};

export struct RagdollSavedEvent {
	bool occurred = false;
};

export struct PrintActiveSystemsEvent {
	bool occurred = false;
};

export struct PrintAllSystemsEvent {
	bool occurred = false;
};