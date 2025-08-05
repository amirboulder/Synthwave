#include "Synthwave.h"

using std::cout;

int main(int argc, char* argv[])
{
	bool running = true;

	Renderer renderer(1920, 1080);

	//renderer init
	running = renderer.createWindow();
	running = renderer.createAndClaimGPU();
	running = renderer.createRenderTargets();

	running = renderer.createSampler();

	Fisiks fisiks;

	Scene scene(fisiks, renderer);


	FreeCam camera(glm::vec3(-2.0f, -2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f));
	camera.aspectRatio = static_cast<float>(renderer.winWidth) / static_cast<float>(renderer.winHeight);
	camera.mouseSensitivity = 0.1;

	InputManager inputManager;

	PlayerInput input;
	Player player;

	
	
	//Shader triangleShader("shaders/physicsDebug/triangleShader.vs", "shaders/physicsDebug/triangleShader.fs");
	MyDebugRenderer fisiksRender(renderer.device,renderer.window,renderer.resolveTarget,renderer.msaaColorTarget,renderer.depthTexture);

	shader::generateSpirvShaders("shaders/slang/physicsRender.slang", "shaders/compiled/physicsRenderVS.spv", "shaders/compiled/physicsRenderFS.spv");
	PL::loadVertexShader(renderer.device, fisiksRender.vertexShader, "shaders/compiled/physicsRenderVS.spv", 0, 2, 0, 0);
	PL::loadFragmentShader(renderer.device, fisiksRender.fragmentShader, "shaders/compiled/physicsRenderFS.spv", 0, 0, 0, 0);

	fisiksRender.createPipeline();

	BodyManager::DrawSettings fiskisDrawSettings;
	//fiskisDrawSettings.mDrawBoundingBox = true;
	//fiskisDrawSettings.mDrawShapeWireframe = false;
	//fiskisDrawSettings.mDrawShape = true;
	//fiskisDrawSettings.mDrawCenterOfMassTransform = true;
	//fiskisDrawSettings.mDrawVelocity = true;
	

	

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
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
		}

	
		float deltaTime = (SDL_GetTicks() - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		accumulator += deltaTime;
		while (accumulator >= timeStep) {

			
			inputManager.processFreeCamInput(running, camera);
			camera.updateVectors();
			

			
			fisiks.update(timeStep);
			fisiks.syncTransfroms(scene.dynamicEnts.transforms, scene.dynamicEnts.physicsComponents);
			//scene.player.update(input,renderer.camera);
			accumulator -= timeStep;
			//scene.LVL1Script(fisiks.physics_system, scene.player.JoltCharacter->GetPosition());
		}

		//TODO INTERPOLATE ?????
		renderer.draw(camera.generateview(),camera.generateProj());

		RVec3Arg camPos(camera.position.x, camera.position.y, camera.position.z);
		fisiksRender.setCameraUnifroms(camPos, camera.generateview(), camera.generateProj());
		fisiksRender.beginRenderPass(renderer.commandBuffer,renderer.swapchainTexture,renderer.swapchainWidth,renderer.swapchainHeight);
		fisiks.physics_system.DrawBodies(fiskisDrawSettings, &fisiksRender);
		fisiksRender.endRenderPass();
		renderer.submitCommandBuffer();
		
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
