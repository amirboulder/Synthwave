#pragma once

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"

#include "glm/glm.hpp"

#include "../ecs/components.hpp"

#include "../common.hpp"

#include "../state/stateManager.hpp"


class InputManager {

	flecs::world& ecs;

public:

	uint16_t forward = SDL_SCANCODE_W;
	uint16_t left = SDL_SCANCODE_A;
	uint16_t right = SDL_SCANCODE_D;
	uint16_t backward = SDL_SCANCODE_S;

	uint16_t escapeMenu = SDL_SCANCODE_ESCAPE;

	uint16_t closeWindow = SDL_SCANCODE_END;

	StateManager& stateManager;

	InputManager(flecs::world& ecs, StateManager& stateManager)
		: ecs(ecs), stateManager(stateManager)
	{

	}


	void handleInput() {

		switch (stateManager.appContext) {
		case AppContext::freeCam:

			processFreeCamKBMInput();
			break;

		case AppContext::player:

			handlePlayerKBMInput();
			break;

		case AppContext::menu:

			break;

		case AppContext::editor:

			break;

		}


	}

	void processFreeCamKBMInput() {

		Camera& camera = stateManager.freeCam.get_mut<Camera>();

		const bool* keystates = SDL_GetKeyboardState(NULL);

		
		// Handle WASD movement
		if (keystates[SDL_SCANCODE_W]) {
			camera.position += camera.front  * camera.movementSpeed;
		}
		if (keystates[SDL_SCANCODE_S]) {
			camera.position -= camera.front * camera.movementSpeed;
		}
		if (keystates[SDL_SCANCODE_D]) {
			camera.position += camera.right * camera.movementSpeed;
		}
		if (keystates[SDL_SCANCODE_A]) {
			camera.position -= camera.right * camera.movementSpeed;
		}

		// Handle mouse input for rotation
		float deltaX, deltaY;
		SDL_GetRelativeMouseState(&deltaX, &deltaY);

		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + deltaY * smoothingFactor;

		camera.rotateCamera(smoothedXOffset, smoothedYOffset);

		camera.updateVectors();

	}

	void handleEvents(bool& running,SDL_Event event,RendererConfig& config) {

		//Camera& camera = cameraManager.freeCam;

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F1) {

			stateManager.handleSavingRenderConfig();
			stateManager.save();
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == closeWindow) {
			running = false;
		}

		if (event.type == SDL_EVENT_QUIT) {
			running = false;
		}

		//used for switching between menu and game
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_ESCAPE) {

			stateManager.handleGamePause();

		}

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			stateManager.handleCameraSwitch();

		}

	}

	void handlePlayerKBMInput() {

		Camera& camera = stateManager.playerCam.get_mut<Camera>();

		PlayerInput & input =  ecs.lookup("player").get_mut<Player>().input;

		const bool* keystates = SDL_GetKeyboardState(NULL);

		// Reset input
		input.direction = glm::vec3(0);
		input.jump = false;
		input.offsetX = 0.0f;
		input.offsetY = 0.0f;


		// Handle WASD movement
		if (keystates[SDL_SCANCODE_W]) {
			input.direction += camera.front;
		}
		if (keystates[SDL_SCANCODE_S]) {
			input.direction -= camera.front;
		}
		if (keystates[SDL_SCANCODE_D]) {
			input.direction += camera.right;
		}
		if (keystates[SDL_SCANCODE_A]) {
			input.direction -= camera.right;
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
		input.offsetY = smoothedYOffset; 

	}


};