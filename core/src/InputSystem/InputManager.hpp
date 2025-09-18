#pragma once

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"

#include "glm/glm.hpp"

#include "../renderer/CameraManager.hpp"

#include "../ecs/components.hpp"

#include "../common.hpp"

#include "../state/stateManager.hpp"

#include "../time/timeManager.hpp"


class InputManager {

public:


	AppContext & context;

	uint16_t forward = SDL_SCANCODE_W;
	uint16_t left = SDL_SCANCODE_A;
	uint16_t right = SDL_SCANCODE_D;
	uint16_t backward = SDL_SCANCODE_S;

	uint16_t escapeMenu = SDL_SCANCODE_ESCAPE;

	uint16_t closeWindow = SDL_SCANCODE_END;


	CameraManager& cameraManager;

	StateManager& stateManager;

	//TimeManager& time;


	InputManager(AppContext & context, CameraManager& cameraManager, StateManager& stateManager)
		: context(context), cameraManager(cameraManager), stateManager(stateManager)
	{

	}


	void handleInput(PlayerInput& input) {

		switch (context) {
		case AppContext::freeCam:

			processFreeCamKBMInput();
			break;

		case AppContext::player:

			handlePlayerKBMInput(input);
			break;

		case AppContext::menu:

			break;

		case AppContext::editor:

			break;

		}


	}

	void processFreeCamKBMInput() {

		Camera& camera = cameraManager.freeCam;

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

		Camera& camera = cameraManager.freeCam;

		//TODO move this out of here
		// trigger an event of some sort
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F1) {

			config.FreeCamFront = camera.front;
			config.FreeCamPos = camera.position;

			config.FreeCamYaw = camera.yaw;
			config.FreeCamPitch = camera.pitch;

			//TODO modify function so the path is not hardcoded
			config.saveRendererConfigINI("config/renderConfig.ini");

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

			stateManager.pauseHandler();

		}

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			if (context == AppContext::player) {

				context = AppContext::freeCam;

				stateManager.setActiveCamera(context);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "using free camera");

			}
			else if (context == AppContext::freeCam) {

				context = AppContext::player;

				stateManager.setActiveCamera(context);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "using player camera");

			}

		}

	}

	void handlePlayerKBMInput(PlayerInput& input) {


		Camera& camera = cameraManager.playerCam;

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