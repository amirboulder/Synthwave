#pragma once

#include "SDL3/SDL_keyboard.h"

#include "SDL3/SDL_mouse.h"

#include "glm/glm.hpp"

#include "../renderer/Camera.hpp"

#include "../components.hpp"


enum InputContext
{
	game,
	menu,

};


class InputManager {

public:


	InputContext context = InputContext::game;

	uint16_t forward = SDL_SCANCODE_W;
	uint16_t left = SDL_SCANCODE_A;
	uint16_t right = SDL_SCANCODE_D;
	uint16_t backward = SDL_SCANCODE_S;

	uint16_t escapeMenu = SDL_SCANCODE_ESCAPE;

	uint16_t closeWindow = SDL_SCANCODE_END;



	int prevMouseX = 1920/2, prevMouseY = 1080/2;

	



	void ProccesInput(bool & running,Camera& camera, PlayerInput& input) {


		processKeyboardGameInput(running, camera, input);

		//handlePlayerInput(running, input);

	}


	void processKeyboardGameInput(bool& running, Camera& camera, PlayerInput& input) {

		const bool* keystates = SDL_GetKeyboardState(NULL);

		// Handle quit condition
		if (keystates[SDL_SCANCODE_END]) {
			running = false;
		}

		// Reset input
		input.direction = glm::vec3(0);
		input.jump = false;
		input.offsetX = 0.0f;
		input.offsetY = 0.0f;

		// Use camera orientation for movement direction
		glm::vec3 camForward = camera.front;
		glm::vec3 camRight = camera.right;

		// Handle WASD movement
		if (keystates[SDL_SCANCODE_W]) {
			input.direction += camForward;
		}
		if (keystates[SDL_SCANCODE_S]) {
			input.direction -= camForward;
		}
		if (keystates[SDL_SCANCODE_D]) {
			input.direction += camRight;
		}
		if (keystates[SDL_SCANCODE_A]) {
			input.direction -= camRight;
		}

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(input.direction) > 0.0f) {
			input.direction = glm::normalize(input.direction);
		}

		// Handle jump (spacebar)
		if (keystates[SDL_SCANCODE_SPACE]) {
			input.jump = true;
		}

		// Handle mouse input for rotation
		float deltaX, deltaY;
		SDL_GetRelativeMouseState(&deltaX, &deltaY);

		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + deltaY * smoothingFactor;

		input.offsetX = smoothedXOffset;
		input.offsetY = -smoothedYOffset; // Negate Y for consistent pitch
	}


	void handlePlayerInput(bool& running,PlayerInput& input) {
		const bool* keystates = SDL_GetKeyboardState(NULL);

		if (keystates[SDL_SCANCODE_END]) {

			running = false;

		}

		// Reset input
		input.direction = glm::vec3(0);
		input.jump = false;

		if (keystates[SDL_SCANCODE_UP]) {
			input.direction.z -= 1.0f; 
		}
		if (keystates[SDL_SCANCODE_DOWN]) {
			input.direction.z += 1.0f;
		}
		if (keystates[SDL_SCANCODE_LEFT]) {
			input.direction.x -= 1.0f;
		}
		if (keystates[SDL_SCANCODE_RIGHT]) {
			input.direction.x += 1.0f;
		}
		
		if (keystates[SDL_SCANCODE_SPACE]) {
			input.jump = true;
		}

	}

	void processMouseGameInput() {


	}

	void processGamepadInput() {

	}

	void procesKeyboardsMenuInput(bool& running, Camera& camera, PlayerInput& input) {

		const bool* keystates = SDL_GetKeyboardState(NULL);

		if (keystates[SDL_SCANCODE_ESCAPE] ) {

			context = InputContext::game;
			cout << "JEET!\n";
		}

	}


};