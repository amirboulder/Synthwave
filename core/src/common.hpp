#pragma once


enum class PlayState {
	play,
	pause
};

struct AppContext {
	enum Type { Menu, Player, Editor, FreeCam } value = Menu;
};