#pragma once
#include <iostream>
#include <vector>
#include <functional>

#include <glad/glad.h>

#include "GLFW/glfw3.h"
#include "../../game/src/userSettings.hpp"
#include "Camera.hpp"

using std::cout;

float xoffset = 0.0f;
float yoffset = 0.0f;

float lastX =  0.0f;
float lastY =  0.0f;

bool firstMouse = true;
bool mouseMoved = false;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	// Get window dimensions
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	
		// Initialize if it's the first movement
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
		return;  // Skip processing first frame to avoid jumps
	}
	firstMouse = false;

	// Calculate offsets
	xoffset = xpos - lastX;
	yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	mouseMoved = true;

	// Update last positions
	lastX = xpos;
	lastY = ypos;
}


//TODO create a proper inpute system
// create an event based design via callback function
void ProcessInput(GLFWwindow* window,Camera & camera, UserSettings settings) {

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	//This get called a bunch of times everytime its pressed
	if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {

		int width, height;
		glfwGetWindowSize(window,&width, &height);
		settings.windowWidth = width;
		settings.windowHeight = height;
		saveUserSettings(settings);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.processKeyboard(FORWARD);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.processKeyboard(BACKWARD);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.processKeyboard(LEFT);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.processKeyboard(RIGHT);
	}

	float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
	float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

	if (mouseMoved) {

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + xoffset * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + yoffset * smoothingFactor;


		camera.processMouseMovement(smoothedXOffset, smoothedYOffset);
		mouseMoved = false;
	}


}

void initGLFW()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}


int initGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

		cout << "failed to initlize glad!" << '\n';
		return -1;
	}
	return 0;
}

class Window {
public:
	unsigned int width = 1280;
	unsigned int height = 720;

	GLFWwindow* windowPtr;
	std::vector<std::function<void(int, int)>> resizeCallbacks;

	Window(unsigned int width, unsigned int height)
	:	width(width),height(height)
	{
		

		initGLFW();
		windowPtr = createWindow(width, height);
		initGLAD();

		// hide cursor while controlling camera - allows for mouse to wrap around
		glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	}

	void addResizeCallback(std::function<void(int, int)> callback) {
		resizeCallbacks.push_back(callback);
	}

	void handleResize(int width, int height) {
		for (auto& callback : resizeCallbacks) {
			callback(width, height);
		}
	}

	//TODO is it ok for this to be a part of the window struct
	static void frameBuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
		// get curret window
		Window* windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(window));
		windowInstance->handleResize(width, height);
		
	
	}

	GLFWwindow* createWindow(unsigned int width, unsigned int height)
	{
		//create WINDOOW object
		GLFWwindow* window = glfwCreateWindow(width, height, "Simulation", NULL, NULL);
		if (!window) {
			cout << "failed to create Window!\n";
			glfwTerminate();
			return nullptr;
		}

		glfwSetWindowPos(window, 400, 50);
		// make the window the main context on the current thread
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, frameBuffer_size_callback);
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetWindowUserPointer(window, this);
		return window;
	}

	void initGLFW()
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	}


	int initGLAD()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

			cout << "failed to initlize glad!" << '\n';
			return -1;
		}
		return 0;
	}

};


