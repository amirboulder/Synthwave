#pragma once

#include "core/src/pch.h"

#include "../common.hpp"

struct MouseClickEvent {

	float x = 0;
	float y = 0;
};


struct ExitEvent {
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
