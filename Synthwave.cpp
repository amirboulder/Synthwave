#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	Fisiks fisiks(ecs);

	Renderer renderer(ecs, fisiks.physics_system);

	TimeManager time;

	StateManager stateManager(ecs,renderer,time);

	Scene scene(ecs, fisiks, renderer, stateManager);

	InputManager inputManager(ecs,stateManager);

	printf("\033[35mWelcome to the simulation!\033[0m\n");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {
			
			inputManager.handleEvents(running,event);

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		time.tick();
		while (time.accumulator >= time.timeStep) {

			inputManager.handleInput();

			fisiks.update(time.timeStep, ecs);
			
			scene.update();

			time.accumulator -= time.timeStep;
		}
		//TODO INTERPOLATE to account for physics and rendering happening at diffrent rates

		renderer.drawAll();

	}

	printf("\033[35mGoodbye!\033[0m\n");
	return 0;
}
