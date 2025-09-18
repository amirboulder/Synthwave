#include "Synthwave.h"

using std::cout;

int main(int argc, char* argv[])
{
	bool running = true;

	Fisiks fisiks;

	RendererConfig renderConfig("config/renderConfig.ini");

	Renderer renderer(renderConfig, fisiks.physics_system);

	TimeManager time;
	
	CameraManager cameraManager(renderConfig);

	StateManager stateManager(renderer, cameraManager,time);

	Scene scene(fisiks, renderer, stateManager, cameraManager.playerCam);

	InputManager inputManager(stateManager.appContext,cameraManager,stateManager);

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


			fisiks.update(time.timeStep);
			fisiks.syncTransfroms(scene.dynamicEnts.transforms, scene.dynamicEnts.physicsComponents);

			scene.player.update(input);
			//scene.LVL1Script(fisiks.physics_system, scene.player.JoltCharacter->GetPosition());

			time.accumulator -= time.timeStep;
		}

		//TODO INTERPOLATE to account for physics and rendering happening at diffrent rates
		renderer.draw();

		//JPH::Vec3 playerPos =  scene.player.JoltCharacter->GetPosition();
		//string posText = std::to_string(playerPos.GetX()) + " " + std::to_string(playerPos.GetY()) + " " + std::to_string(playerPos.GetZ());
		//renderer.drawText(posText, { 50.0f,50.0f }, 1, { 1.0f,1.0f,1.0f });


		printf("FPS: %d\n", time.fps);
		//renderer.drawFps(fps);
	}

	printf("\033[35mGoodbye!\033[0m\n");
	return 0;
}
