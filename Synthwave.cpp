// Synthwave.cpp : Defines the entry point for the application.
//

#include "Synthwave.h"

using namespace std;



int main()
{

	

	//initGLFW();
	//initGLAD();
	//GLFWwindow* window = createWindow();
	
	Window window(1280, 720);



	

	Scene scene(1);
	scene.constructLevel1();

	Renderer renderer(scene.entities);

	renderer.setup();

	

	int fpsCap = 144;
	double timeStep = 1.0f / static_cast<double>(fpsCap);
	double accumulator = 0.0f;
	double lastTime = glfwGetTime();
	int frameCount = 0;
	double fpsTimer = glfwGetTime();

	cout << "Welcome to the simulation!\n";
	while (!glfwWindowShouldClose(window.windowPtr)) {

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		accumulator += deltaTime;
		while (accumulator >= timeStep) {

			ProcessInput(window.windowPtr, renderer.camera);

			//Do Physics and AI here

			accumulator -= timeStep;
		}

		//TODO INTERPOLATE ?????

		renderer.draw();


		frameCount++;
		if (currentTime - fpsTimer >= 1.0) {
			//std::cout << "FPS: " << frameCount << '\n';
			frameCount = 0;
			fpsTimer = currentTime;
		}

		glfwSwapBuffers(window.windowPtr);
		glfwPollEvents();

	}

	glfwTerminate();
	cout << "Goodbye!\n";
}
