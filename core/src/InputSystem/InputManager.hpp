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
		//TODO keep a ref instead of looking up by name
		Camera& camera = ecs.lookup("FreeCam").get_mut<Camera>();

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

	void handleEvents(SDL_Event event) {


		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F1) {

			stateManager.save();
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == closeWindow) {
			stateManager.exitCallback();
		}

		if (event.type == SDL_EVENT_QUIT) {
			stateManager.exitCallback();
		}

		//used for switching between menu and game
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_ESCAPE) {

			stateManager.handleGamePause();

		}

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			stateManager.switchAppContext();

		}

	}

	void handlePlayerKBMInput() {
		//TODO keep a ref instead of looking up by name
		Camera& camera = ecs.lookup("PlayerCam").get_mut<Camera>();

		Player & player2 = ecs.lookup("player2").get_mut<Player>();

		const bool* keystates = SDL_GetKeyboardState(NULL);

		glm::vec3 playerInput = glm::vec3(0);

		// Handle WASD movement
		if (keystates[SDL_SCANCODE_W]) {
			playerInput += camera.front;
		}
		if (keystates[SDL_SCANCODE_S]) {
			playerInput -= camera.front;
		}
		if (keystates[SDL_SCANCODE_D]) {
			playerInput += camera.right;
		}
		if (keystates[SDL_SCANCODE_A]) {
			playerInput -= camera.right;
		}

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(playerInput) > 0.0f) {
			playerInput = glm::normalize(playerInput);
		}

		player2.mMoveInput.SetX(playerInput.x);
		player2.mMoveInput.SetY(playerInput.y);
		player2.mMoveInput.SetZ(playerInput.z);

		// Handle jump (spacebar)
		if (keystates[SDL_SCANCODE_SPACE]) {
			
			player2.Jump();

		}

		// Handle mouse input for rotation
		float deltaX, deltaY;
		SDL_GetRelativeMouseState(&deltaX, &deltaY);

		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + deltaY * smoothingFactor;

		player2.offsetX = smoothedXOffset;
		player2.offsetY = smoothedYOffset;

	}


};