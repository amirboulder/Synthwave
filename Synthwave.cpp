#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	RendererConfig renderConfig("config/renderConfig.ini");

	Fisiks fisiks;

	Renderer renderer(ecs,renderConfig, fisiks.physics_system);

	TimeManager time;

	StateManager stateManager(ecs,renderConfig,renderer,time);

	Scene scene(ecs, fisiks, renderer, stateManager);

	InputManager inputManager(stateManager);

	PlayerInput input;
	
	printf("\033[35mWelcome to the simulation!\033[0m\n");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {
			
			inputManager.handleEvents(running,event,renderConfig);
		}

		time.tick();
		while (time.accumulator >= time.timeStep) {

			inputManager.handleInput(input);

			fisiks.update(time.timeStep, ecs);
			
			scene.update(input);

			time.accumulator -= time.timeStep;
		}
		//TODO INTERPOLATE to account for physics and rendering happening at diffrent rates

		renderer.setFPSText(time.fps);

		renderer.drawAll();

	}

	printf("\033[35mGoodbye!\033[0m\n");
	return 0;
}
