#include "Synthwave.h"

using std::cout;

int main(int argc, char* argv[])
{
	bool running = true;

	StateManager stateManager;

	Fisiks fisiks;

	RendererConfig renderConfig("config/renderConfig.ini");

	Renderer renderer(renderConfig, fisiks.physics_system);
	
	CameraManager cameraManager(renderer,renderConfig, stateManager.appContext);

	Player player(cameraManager.playerCam);

	Scene scene(fisiks, renderer, stateManager, player);

	InputManager inputManager(stateManager.appContext,cameraManager,scene.stateManager);

	PlayerInput input;
	
	
	
	float timeStep = 1.0f / 120.0f;
	float accumulator = 0.0f;
	uint64_t lastTime = SDL_GetTicks();
	int frameCount = 0;
	int fps = 0;
	uint64_t fpsTimer = SDL_GetTicks();

	printf("\033[35mWelcome to the simulation!\033[0m\n");
	SDL_Event event;
	while (running) {

		//OPTICK_FRAME("MainThread");

		while (SDL_PollEvent(&event)) {
			
			inputManager.handleEvents(running,event,renderConfig);
		}

	
		float deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;
		while (accumulator >= timeStep) {

			inputManager.handleInput(input);


			fisiks.update(timeStep);
			fisiks.syncTransfroms(scene.dynamicEnts.transforms, scene.dynamicEnts.physicsComponents);

			scene.player.update(input);
			//scene.LVL1Script(fisiks.physics_system, scene.player.JoltCharacter->GetPosition());

			accumulator -= timeStep;
		}

			//TODO INTERPOLATE to account for physics and rendering happening at diffrent rates
			renderer.draw();

		//JPH::Vec3 playerPos =  scene.player.JoltCharacter->GetPosition();
		//string posText = std::to_string(playerPos.GetX()) + " " + std::to_string(playerPos.GetY()) + " " + std::to_string(playerPos.GetZ());
		//renderer.drawText(posText, { 50.0f,50.0f }, 1, { 1.0f,1.0f,1.0f });

		frameCount++;
		if (SDL_GetTicks() - fpsTimer >= 1000.0) {
			fps = frameCount;
			frameCount = 0;
			fpsTimer = SDL_GetTicks();
		}
		//printf("FPS: %d\n", fps);
		//renderer.drawFps(fps);
	}

	printf("\033[35mGoodbye!\033[0m\n");
	return 0;
}
