#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	Renderer renderer(ecs);

	Fisiks fisiks(ecs);

	AssetLibrary assetLib(ecs);

	MenuSystem menuSys(ecs);

	TimeManager time(fisiks.timeStep);

	Scene scene(ecs, fisiks, renderer);

	Serializer serde(ecs);

	Editor editor(ecs);

	StateManager stateManager(ecs, renderer, fisiks, serde, menuSys, editor, time, scene, running);

	InputManager inputManager(ecs, stateManager);

	stateManager.init();

	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, Synth "Initializing simulation 🤖" RESET);
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

			ecs.progress();

			time.accumulator -= time.timeStep;
		}
		//TODO INTERPOLATE to account for physics and rendering happening at different rates
		renderer.drawAll();

	}
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, Synth "Goodbye!" RESET);
	return 0;
}
