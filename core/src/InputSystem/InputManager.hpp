#pragma once

#include "../ecs/components.hpp"
#include "../ecs/eventComponents.hpp"

#include "../common.hpp"


/// <summary>
/// Processes input and emits events when appropriate.
/// This class is responsible for creating input components
/// </summary>
class InputManager {

	flecs::world& ecs;

public:

	//TODO Key mappings should be loaded from a config file.
	uint16_t forwardKey = SDL_SCANCODE_W;
	uint16_t leftKey = SDL_SCANCODE_A;
	uint16_t rightKey = SDL_SCANCODE_D;
	uint16_t backwardKey = SDL_SCANCODE_S;

	uint16_t jumpKey = SDL_SCANCODE_SPACE;

	uint16_t escapeMenu = SDL_SCANCODE_ESCAPE;

	uint16_t closeWindow = SDL_SCANCODE_END;


	InputManager(flecs::world& ecs)
		: ecs(ecs)
	{

		ecs.component<UserInput>().add(flecs::Singleton);
		ecs.set<UserInput>({});

		ecs.component<Direction>().add(flecs::Singleton);
		ecs.set<Direction>({ Direction::forward });

		ecs.component<MouseClickLeftEvent>().add(flecs::Singleton);
		//ecs.set<MouseClickEvent>({});

		ecs.component<ExitEvent>().add(flecs::Singleton);
		ecs.set<ExitEvent>({});

		ecs.component<WindowLostFocusEvent>().add(flecs::Singleton);
		ecs.set<WindowLostFocusEvent>({});

		ecs.component<GamePauseEvent>().add(flecs::Singleton);
		ecs.set<GamePauseEvent>({});

		ecs.component<EditorToggleEvent>().add(flecs::Singleton);
		ecs.set<EditorToggleEvent>({});

		ecs.component<CameraSwitchEvent>().add(flecs::Singleton);
		ecs.set<CameraSwitchEvent>({});

		ecs.component<PhysicsRenderToggleEvent>().add(flecs::Singleton);
		ecs.set<PhysicsRenderToggleEvent>({});
		
		ecs.component<SaveGameSrcEvent>().add(flecs::Singleton);
		ecs.set<SaveGameSrcEvent>({});

		ecs.component<PrintSystemsEvent>().add(flecs::Singleton);
		ecs.set<PrintSystemsEvent>({});

		LogSuccess(LOG_APP, "InputManager Initialized");
	}

	/// <summary>
	/// Resolves input based on the current input device
	/// </summary>
	void handleInput() {

		//TODO implement switching based on InputDeviceState as well once we get there
		captureInputKeyboardMouse();
	}


	void handleEvents(SDL_Event& event) {

		if (event.type == SDL_EVENT_QUIT) {

			ecs.set<ExitEvent>({ true });
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_ESCAPE) {

			ecs.set<GamePauseEvent>({true});
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {

			ecs.set<MouseClickLeftEvent>({ event.button.x,event.button.y });
		}

		handleEditorEvents(event);
	}


	// Put things in here that we don't want in distribution mode.
	void handleEditorEvents(SDL_Event& event) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == closeWindow) {
			ecs.set<ExitEvent>({true});
		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F1) {

			ecs.set<EditorToggleEvent>({ true });
		}

		if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {

			ecs.set<WindowLostFocusEvent>({ true });
		}
		if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
			LogDebug(LOG_INPUT, "Window gained focus!");
		}

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			ecs.set<CameraSwitchEvent>({ true });

		}

		//TODO change these to buttons in the editor
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F3) {

			//stateManager.printSystems();
			ecs.set<PrintSystemsEvent>({ true });

		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F4) {

			//stateManager.printPhases();
		}

		// disable physics Renderer Phase
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F6) {

			ecs.set<PhysicsRenderToggleEvent>({ true });
		}

		EditorState state = ecs.get<EditorState>();

		//All the things that should only happen when editor is enabled
		if (state == EditorState::Enabled) {

			if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F5) {

				ecs.set<SaveGameSrcEvent>({ true });
			}

			//FreeCam is created with the editor so we assume it exists (not checking for null)
			flecs::entity cameraEnt = ecs.lookup("FreeCam");
			CameraMVMTState* state = cameraEnt.try_get_mut<CameraMVMTState>();
			if (!state) {

				LogError(LOG_INPUT, "CameraMVMTState does not exist!");
				return;
			}

			if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_MIDDLE) {
				
				state->locked = false;
				SDL_SetWindowRelativeMouseMode(renderContext.window, true);
				CMN::flushMouseMovement();
			
			}
			else if(event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_MIDDLE) {
				
				state->locked = true;
				SDL_SetWindowRelativeMouseMode(renderContext.window, false);

			}

		}

	}


	/// <summary>
	/// captures all relevant Keyboard and Mouse input and updates the ECS.
	/// </summary>
	void captureInputKeyboardMouse() {

		const bool* keystates = SDL_GetKeyboardState(NULL);

		UserInput& input = ecs.get_mut<UserInput>();

		// Reset each frame before accumulating
		input.direction = glm::vec2(0);
		input.jump = false;

		if (keystates[forwardKey]) {

			input.direction.y += 1;
		}
		if (keystates[backwardKey]) {

			input.direction.y -= 1;
		}
		if (keystates[leftKey]) {

			input.direction.x -= 1;
		}
		if (keystates[rightKey]) {

			input.direction.x += 1;
		}

		if (keystates[jumpKey] && input.jumpConsumed) {
			input.jump = true;
			input.jumpConsumed = false;
		}
		if (!keystates[jumpKey])
			input.jumpConsumed = true; // ready to jump again

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(input.direction) > 0.0f) {
			input.direction = glm::normalize(input.direction);
		}


		// Handle mouse input for rotation
		float deltaX, deltaY;
		SDL_GetRelativeMouseState(&deltaX, &deltaY);

		//TODO parameterize
		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + deltaY * smoothingFactor;

		input.offsetX = smoothedXOffset;
		input.offsetY = smoothedYOffset;
	}


};