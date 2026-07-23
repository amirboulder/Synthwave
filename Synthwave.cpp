#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	Renderer renderer(ecs);

	Physics physics(ecs);

	TransformPropagation transformPropagation(ecs);

	AssetLibrary assetLib(ecs);
	AssetManager assetManager(ecs, assetLib.manifest);

	Registrar registrar(ecs);

	MenuSystem menuSys(ecs);

	TimeManager time(physics.timeStep);

	Scene scene(ecs);

	Serializer serializer(ecs);

	Editor editor(ecs);

	StateManager stateManager(ecs, renderer, physics, serializer, menuSys, editor, time, scene, running);

	InputManager inputManager(ecs);

	stateManager.init();

	LogSynth(LOG_APP,"Initializing Simulation 🤖");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {

			inputManager.handleEvents(event);
		}

		time.tick();
		while (time.accumulator >= time.timeStep) {

			//TODO input can be handled at a faster rate which would enable faster camera movement (less latency),
			// which would then require interpolation
			inputManager.handleInput(); 

			ecs.progress(); //All systems except rendering happen here.

			time.accumulator -= time.timeStep;
		}
		
		renderer.drawAll();

	}
	LogSynth(LOG_APP, "Goodbye!");
	return 0;
}
