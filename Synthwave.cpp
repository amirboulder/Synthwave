#include "Synthwave.h"

int main(int argc, char* argv[])
{
	bool running = true;

	SDL_SetAppMetadata("Synthwave", "0.0.1", "SynthID");

	flecs::world ecs;

	Logger logger; //sets all log categories to SDL_LOG_PRIORITY_INFO.

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

	//Keeps track of all rendered frames (separate from ecs frame)
	ecs.component<FrameCounter>().add(flecs::Singleton);
	ecs.set<FrameCounter>({});
	uint64_t& frameCounter = ecs.get_mut<FrameCounter>().count;

	LogSynth(LOG_APP,"Initializing Simulation 🤖");

	while (running) {

		//OPTICK_FRAME("MainThread");
		inputManager.accumulateInput();

		time.tick();
		while (time.accumulator >= time.timeStep) {

			
			ecs.progress(); //All systems except rendering happen here.

			time.accumulator -= time.timeStep;
		}
		
		frameCounter++;
		renderer.drawAll();

	}
	LogSynth(LOG_APP, "Goodbye!");
	return 0;
}
