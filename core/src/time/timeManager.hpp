#pragma once

class TimeManager {

public:

	float timeStep = 1.0f / 60.0f;
	float accumulator = 0.0f;

	uint64_t lastTime = 0;

	float deltaTime = 0.0f;
	//float maxAccumulator = timeStep * 60.0f;

	uint64_t appStartTime = 0;


	TimeManager(float timeStep)
		: timeStep(timeStep) 
	{
		appStartTime = SDL_GetTicks();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "TimeManager Initialized" RESET);
	}

	void startGameTime()
		
	{
		lastTime = SDL_GetTicks();
	}

	void tick() {


		deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;

		// Clamp to prevent spiral of death
		/*if (accumulator > maxAccumulator) {
			accumulator = maxAccumulator;
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Game is running too slow - spiral of death!");
		}*/
	}


	void flushAccumulatedTime() {
		lastTime = SDL_GetTicks();
	}


};