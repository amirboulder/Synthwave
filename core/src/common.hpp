#pragma once

enum class AppContext {
	player,
	freeCam,
	menu,
	editor
};

//TODO decide to use or remove

struct ApplicationContext {

	void updateContext(AppContext newContext) {

		context = newContext;
	}

private:

	AppContext context;

};


enum class PlayState {
	play,
	pause
};

