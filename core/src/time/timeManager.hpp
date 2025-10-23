#pragma once

class TimeManager {

public:

	float timeStep = 1.0f / 60.0f;
	float accumulator = 0.0f;

	uint64_t lastTime = 0;

	float deltaTime = 0.0f;

	uint64_t appStartTime = 0;

	
	float maxAccumulator = timeStep * 5.0f; 

	bool paused = true;

	TimeManager(float timeStep)
		: timeStep(timeStep) 
	{
		appStartTime = SDL_GetTicks();
	}

	void startGameTime()
		
	{
		lastTime = SDL_GetTicks();
		paused = false;
	}

	void tick() {

		if (paused) return;

		deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;

		// Clamp to prevent spiral of death
		if (accumulator > maxAccumulator) {
			accumulator = maxAccumulator;
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game is running too slow - spiral of death!");
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