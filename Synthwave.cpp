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

	Scene scene(ecs);

	Serializer serializer(ecs);

	Editor editor(ecs);

	StateManager stateManager(ecs, renderer, fisiks, serializer, menuSys, editor, time, scene, running);

	InputManager inputManager(ecs);

	stateManager.init();

	LogSynth(LOG_APP,"Initializing Simulation 🤖");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {

			inputManager.handleEvents(event);

			ImGui_ImplSDL3_ProcessEvent(&event);
		}

		time.tick();
		while (time.accumulator >= time.timeStep) {

			//TODO input can be handled at a faster rate which would enable faster camera movement, which would then require interpolation.
			inputManager.handleInput(); 

			ecs.progress(); //All systems except rendering happen here.

			time.accumulator -= time.timeStep;
		}
		
		renderer.drawAll();

	}
	LogSynth(LOG_APP, "Goodbye!");
	return 0;
}
