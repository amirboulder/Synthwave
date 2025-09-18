#pragma once

class TimeManager {

public:

	float timeStep = 1.0f / 120.0f;
	float accumulator = 0.0f;

	uint64_t lastTime = 0;
	int frameCount = 0;
	int fps = 0;
	uint64_t fpsTimer = 0;

	float deltaTime = 0.0f;

	bool paused = false;

	void tick() {

		if (paused) return;

		deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;

		frameCount++;
		if (SDL_GetTicks() - fpsTimer >= 1000.0) {
			fps = frameCount;
			frameCount = 0;
			fpsTimer = SDL_GetTicks();
		}
	}

	void pauseGame() {

		paused = true;
	}

	void unPauseGame() {

		paused = false;

		// setting this to zero so there is no time accumulated
		lastTime = SDL_GetTicks(); 

	}


};