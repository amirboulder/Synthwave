#include "Synthwave.h"
 

using std::cout;

int main(int argc, char* argv[])
{

	UserSettings settings = loadUserSettings();

	SDLWindow sdlWindow(settings.windowWidth, settings.windowHeight);

	InputManager inputManager;
	
	PlayerInput input;
	
	Renderer renderer(settings.windowWidth, settings.windowHeight);

	Fisiks fisiks;

	Scene scene(1);
	scene.constructLVL1(fisiks);


	float timeStep = 1.0f / 120.0f;
	float accumulator = 0.0f;
	uint64_t lastTime = SDL_GetTicks();
	int frameCount = 0;
	int fps = 0;
	uint64_t fpsTimer = SDL_GetTicks();

	cout << "Welcome to the simulation!\n";
	
	bool running = true;
	SDL_Event event;
	while (running) {

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
		}

		float deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;
		while (accumulator >= timeStep) {

			
			inputManager.ProccesInput(running, renderer.camera, input);
			
			fisiks.update(timeStep);
			fisiks.updateTransforms(scene.transforms, scene.physicsCompoments);

			scene.player.update(input,renderer.camera);
			
			accumulator -= timeStep;
		}

		//TODO INTERPOLATE ?????

		renderer.draw(scene.models, scene.transforms);

		renderer.drawText("Hello Synthwave", { 50.0f,50.0f }, 1, { 1.0f,1.0f,1.0f });

		//RVec3Arg camPos(renderer.camera.position.x, renderer.camera.position.y, renderer.camera.position.z);
		//fisiksRender.SetCameraPos(camPos);
		//fisiks.physics_system.DrawBodies(fiskisDrawSettings, &fisiksRender);
		//fisiksRender.drawAllLines();
		

		frameCount++;
		if (SDL_GetTicks() - fpsTimer >= 1000.0) {
			fps = frameCount;
			frameCount = 0;
			fpsTimer = SDL_GetTicks();
		}
		renderer.drawFps(fps);
		
		
		SDL_GL_SwapWindow(sdlWindow.window);

	}


	sdlWindow.destroy();
	cout << "Goodbye!\n";
	return 0;
}
