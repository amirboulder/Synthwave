#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	flecs::world ecs;

	InputManager inputManager(ecs);

	Renderer renderer(ecs);

	Physics physics(ecs);

	TransformPropagation transformPropagation(ecs);

	AssetLibrary assetLib(ecs);
	AssetManager assetManager(ecs, assetLib.manifest);

	Registrar registrar(ecs);

	MenuSystem menuSys(ecs);

	TimeManager time(ecs, physics.timeStep);

	Scene scene(ecs);

	Serializer serializer(ecs);

	Editor editor(ecs);

	StateManager stateManager(ecs, renderer, physics, serializer, menuSys, editor, time, scene, running);

	stateManager.init();

	LogSynth(LOG_APP,"Initializing Simulation 🤖");

	while (running) {

		//OPTICK_FRAME("MainThread");

		

		time.tick();
		while (time.accumulator >= time.timeStep) {

			inputManager.accumulateInput();
			ecs.progress(); //All systems except rendering happen here.

			time.accumulator -= time.timeStep;
		}
		
		renderer.drawAll();

	}
	LogSynth(LOG_APP, "Goodbye!");
	return 0;
}
