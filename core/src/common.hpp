#pragma once

struct AppContext {
	enum Type { Menu, Player, Editor, FreeCam } value = Menu;
};