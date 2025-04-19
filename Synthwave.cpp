#include "Synthwave.h"

using std::cout;

int main()
{
	UserSettings settings = loadUserSettings();

	Window window(settings.windowWidth, settings.windowHeight);

	Scene scene(1);
	scene.constructLevel1();

	Renderer renderer(window.width,window.height);

	window.addResizeCallback([&renderer](int width, int height) {
		renderer.handleWindowResize(width,height);
	});

	Grid grid("shaders/grid2Shader.vs", "shaders/grid2Shader.fs", 1600, 1600);
	
	int tickRate = 144;
	double timeStep = 1.0f / static_cast<double>(tickRate);
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

			//Do Physics and AI here

			accumulator -= timeStep;
		}

		//TODO INTERPOLATE ?????

		renderer.draw(scene.entities);
		//TODO make grid an entity like the rest
		grid.draw();

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
}
