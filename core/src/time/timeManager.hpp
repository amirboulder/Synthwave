#pragma once

/// <summary>
/// Keeps track of how much times is consumed in every frame.
/// If enough time accumulated (>= timeStep) then game loop will progress
/// Note: When game is paused we DO NOT pause time but rather disable game systems.
/// Note: deltaTime has been capped 50ms to avoid doing too many updates when debugging.
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
		uint64_t now = SDL_GetTicks();
		deltaTime = (now - lastTime) / 1000.0f;
		lastTime = now;
		deltaTime = std::min(deltaTime, 0.05f); // clamp before accumulating
		accumulator += deltaTime;
	}

};