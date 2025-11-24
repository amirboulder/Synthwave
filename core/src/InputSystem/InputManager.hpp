#pragma once

#include "core/src/pch.h"

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
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "InputManager Initialized");
	}


	void handleInput() {

		//TODO implement switching based on InputDeviceState as well once we get there


		CameraState camState = ecs.get<CameraState>();

		switch (camState) {
		case CameraState::FREECAM:

			processFreeCamKBMInput();
			break;

		case CameraState::PLAYER:

			// If game is paused don't handle player input
			if (ecs.get<PlayState>() == PlayState::PAUSE) {
				return;
			}

			handlePlayerKBMInput();
			break;


		}


	}

	void processFreeCamKBMInput() {
		//TODO keep a ref instead of looking up by name

		flecs::entity cameraEnt = ecs.lookup("FreeCam");
		if (!cameraEnt ) {
			return;
		}

		Camera& camera = cameraEnt.get_mut<Camera>();

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

	void handleEvents(SDL_Event& event) {



		if (event.type == SDL_EVENT_QUIT) {
			stateManager.exitCallback();
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_ESCAPE) {

			stateManager.gamePauseHandler();

		}

		handleEditorEvents(event);

	}

	void handleAlwaysEvents(SDL_Event& event) {

	}

	void handlePlayerEvents(SDL_Event& event) {

	}

	void handleMenuEvents(SDL_Event& event) {

	}

	// Put things in here that we don't want in distribution mode.
	void handleEditorEvents(SDL_Event& event) {

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == closeWindow) {
			stateManager.exitCallback();
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F1) {

			stateManager.toggleEditor();
		}

		if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
			std::cout << "Window lost focus!" << std::endl;
		}
		if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
			std::cout << "Window gained focus!" << std::endl;
		}

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			stateManager.cameraSwitchHandler();

		}

		//TODO change these to buttons in the editor
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F3) {

			stateManager.printSystems();

		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F4) {

			stateManager.printPhases();
		}

		// disable physics Renderer Phase
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F6) {

			stateManager.togglePhysicsRenderer();
		}

		EditorState state = ecs.get<EditorState>();

		//All the things that should only happen when editor is enabled
		if (state == EditorState::Enabled) {

			if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F5) {

				stateManager.saveGameData();

			}

		}

	}

	void handleEditorInputs() {

	}
	
	//TODO look into decoupling inputManager and Player/PlayerCam
	void handlePlayerKBMInput() {

		flecs::entity playerEnt;
		flecs::entity cameraEnt;
		
		if (!ecs.try_get<PlayerRef>() || !ecs.try_get<PlayerCamRef>()) {
			return;
		}

		playerEnt = ecs.get<PlayerRef>().value;
		cameraEnt = ecs.get<PlayerCamRef>().value;


		if (!cameraEnt || !playerEnt) {
			return;
		}

		Camera& camera = cameraEnt.get_mut<Camera>();
		Player & player = playerEnt.get_mut<Player>();

		const bool* keystates = SDL_GetKeyboardState(NULL);

		// Project camera vectors onto horizontal plane for movement
		// This takes prevents Y from contribution to toward the vector's length 
		// which will cause slower movement when camera is pointing down
		//Assumes Y is up , works because camera PITCH_LIMIT = 89.0f
		glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
		glm::vec3 right = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));

		glm::vec3 playerInput = glm::vec3(0);

		// Handle WASD movement
		if (keystates[SDL_SCANCODE_W]) {
			playerInput += forward;
		}
		if (keystates[SDL_SCANCODE_S]) {
			playerInput -= forward;
		}
		if (keystates[SDL_SCANCODE_D]) {
			playerInput += right;
		}
		if (keystates[SDL_SCANCODE_A]) {
			playerInput -= right;
		}

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(playerInput) > 0.0f) {
			playerInput = glm::normalize(playerInput);
		}

		player.mMoveInput.SetX(playerInput.x);
		player.mMoveInput.SetY(playerInput.y);
		player.mMoveInput.SetZ(playerInput.z);

		// Handle jump (spacebar)
		if (keystates[SDL_SCANCODE_SPACE]) {
			
			player.Jump();

		}

		// Handle mouse input for rotation
		float deltaX, deltaY;
		SDL_GetRelativeMouseState(&deltaX, &deltaY);

		//TODO parameterlize
		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + deltaY * smoothingFactor;

		player.offsetX = smoothedXOffset;
		player.offsetY = smoothedYOffset;

	}


};