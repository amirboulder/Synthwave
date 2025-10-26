#include "Synthwave.h"


int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	Renderer renderer(ecs);

	Fisiks fisiks(ecs);

	TimeManager time(fisiks.timeStep);

	Scene scene(ecs, fisiks, renderer);

	StateManager stateManager(ecs, renderer, fisiks, time, scene, running);

	InputManager inputManager(ecs,stateManager);

	stateManager.init();

	printf("\033[35mInitializing simulation\033[0m\n");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {
			
			inputManager.handleEvents(event);

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		time.tick();
		while (time.accumulator >= time.timeStep) {

			inputManager.handleInput();

			stateManager.update();

			ecs.progress();

			time.accumulator -= time.timeStep;
		}
		//TODO INTERPOLATE to account for physics and rendering happening at different rates
		renderer.drawAll();

	}

	printf("\033[35mGoodbye!\033[0m\n");
	return 0;
}
