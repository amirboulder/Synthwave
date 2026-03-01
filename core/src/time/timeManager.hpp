#pragma once

/// <summary>
/// Keeps track of how much times is consumed in every frame.
/// If enough time accumulated (>= timeStep) then game loop will progress
/// Note: When game is paused we DO NOT pause time but rather disable game systems.
/// </summary>
class TimeManager {

public:

	float timeStep = 1.0f / 60.0f;
	float accumulator = 0.0f;

	uint64_t lastTime = 0;

	float deltaTime = 0.0f;

	uint64_t appStartTime = 0;


	TimeManager(float timeStep)
		: timeStep(timeStep) 
	{
		appStartTime = SDL_GetTicks();

		LogSuccess(LOG_APP, "TimeManager Initialized");
	}

	void startGameTime()
		
	{
		lastTime = SDL_GetTicks();
	}

	void tick() {

		deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;

	}

};