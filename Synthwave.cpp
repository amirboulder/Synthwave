#include "Synthwave.h"
#include <chrono>
using std::cout;

int main()
{
	UserSettings settings = loadUserSettings();

	Window window(settings.windowWidth, settings.windowHeight);

	
	Renderer renderer(window.width,window.height);
	window.addResizeCallback([&renderer](int width, int height) {
		renderer.handleWindowResize(width, height);
	});


	Fisiks fisiks;

	Scene scene(1);
	scene.constructLVL1(fisiks);


	double timeStep = 1.0f / 120.0f;
	double accumulator = 0.0f;
	double lastTime = glfwGetTime();
	int frameCount = 0;
	int fps = 0;
	double fpsTimer = glfwGetTime();

	cout << "Welcome to the simulation!\n";
	while (!glfwWindowShouldClose(window.windowPtr)) {




		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		accumulator += deltaTime;
		while (accumulator >= timeStep) {

			ProcessInput(window.windowPtr, renderer.camera, settings);


			fisiks.update(timeStep);
			fisiks.updateTransforms(scene.transforms, scene.physicsCompoments);

			accumulator -= timeStep;
		}

		//TODO INTERPOLATE ?????

		renderer.draw(scene.models, scene.transforms);

		renderer.drawText("Hello Synthwave", { 50.0f,50.0f }, 1, { 1.0f,1.0f,1.0f });

		frameCount++;
		if (currentTime - fpsTimer >= 1.0) {
			fps = frameCount;
			frameCount = 0;
			fpsTimer = currentTime;
		}
		renderer.drawFps(fps);
		
		glfwSwapBuffers(window.windowPtr);
		glfwPollEvents();

	}

	glfwTerminate();
	cout << "Goodbye!\n";
	return 0;
}
