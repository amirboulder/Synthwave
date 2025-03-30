#pragma once
#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include "Camera.hpp"

using std::cout;

unsigned int windowWidth = 1920;
unsigned int windowHeight = 1080;


float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;
bool firstMouse = true;
bool mouseMoved = false;

float xoffset = 0.0f;
float yoffset = 0.0f;

//TODO make window part of the renderer

void frameBuffer_size_callback(GLFWwindow* window, int width, int height)
{
	//camera.SetAspectRatio((float)width / (float)height);
	//projection = camera.getProjectionMatrix();
	glViewport(0, 0, width, height);

}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	// Get window dimensions
	int width, height;
	glfwGetWindowSize(window, &width, &height);


	// Check if mouse is at window edges and wrap if needed
	//bool wrapped = false;
	
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

	// If we wrapped, skip this frame's movement to avoid camera jump
	//if (!wrapped) {
		mouseMoved = true;
	//}

	// Update last positions
	lastX = xpos;
	lastY = ypos;
}

void ProcessInput(GLFWwindow* window,Camera & camera) {

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
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


GLFWwindow* createWindow()
{
	//create WINDOOW object
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Simulation", NULL, NULL);
	if (!window) {
		cout << "failed to create Window!\n";
		glfwTerminate();
		return nullptr;
	}

	glfwSetWindowPos(window, 800, 200);
	// make the window the main context on the current thread
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, frameBuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

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

