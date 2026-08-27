#pragma once

#include "../ecs/components.hpp"
#include "../ecs/eventComponents.hpp"

#include "../common.hpp"

enum class MouseButtons {

	BUTTON_INVALID,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_X1,
	BUTTON_X2,
};


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

	uint16_t escapeMenuKey = SDL_SCANCODE_ESCAPE;

	uint16_t closeWindowKey = SDL_SCANCODE_END;

	uint16_t testGamePadButton = SDL_GAMEPAD_BUTTON_SOUTH;           /**< Bottom face button (e.g. Xbox A button) */

	uint16_t leftClickKey = SDL_BUTTON_LEFT;
	uint16_t rightClickKey = SDL_BUTTON_RIGHT;

	//Maps the key/button to the Event that is mapped to it
	std::vector<std::pair<SDL_Scancode, uint64_t>> keyboardMappings;
	std::vector<std::pair<MouseButtons, uint64_t>> MouseMappings;
	std::vector<std::pair<SDL_GamepadButton, uint64_t>> gamepadMappings;

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

		ecs.component<InteractEvent>().add(flecs::Singleton);
		ecs.set<InteractEvent>({});

		ecs.component<InteractEvent>().add(flecs::Singleton);
		ecs.set<InteractEvent>({});
		flecs::id_t id = ecs.id<InteractEvent>();
		bindEventToKeyboard(SDL_SCANCODE_E, id);

		LogSuccess(LOG_APP, "InputManager Initialized");
	}


	bool bindEventToKeyboard(SDL_Scancode key, uint64_t eventID) {

		flecs::entity e = ecs.entity(eventID);
		if (!e.is_valid()) {
			LogError(LOG_APP, "Entity with ID %d is invalid cannot bind to key", eventID);
			return false;
		}

		const char* eventName = e.name();

		//If the key is already mapped then reassign it
		for (std::pair<SDL_Scancode, uint64_t> & map : keyboardMappings) {

			if (map.first == key) {
				 
				LogWarn(LOG_APP, " Key %s is already bound to event %s rebinding it!", magic_enum::enum_name(key), eventName);
				map.second = eventID;
			}
		}

		//If the Event is mapped to a Mouse button remove that mapping 
		for (int i = 0; i < MouseMappings.size(); i++) {

			std::pair<MouseButtons, uint64_t>& map = MouseMappings[i];

			if (map.second == eventID) {

				LogWarn(LOG_APP, "Event %s is already mapped to Mouse Button", eventName, magic_enum::enum_name(map.first));
				MouseMappings.erase(MouseMappings.begin() + i);
			}
		}

		//If not bound at all simply bind it
		keyboardMappings.emplace_back(key, eventID);
	}

	void handleInput() {

		//TODO implement switching based on InputDeviceState as well once we get there
		captureInputKeyboardMouse();
	}

	void handleEvents(SDL_Event& event) {

		ImGui_ImplSDL3_ProcessEvent(&event);
		ImGuiIO& io = ImGui::GetIO();

		// We handle quit and escape BEFORE yielding to ImGui,
		// so escape can close the pause menu even when ImGui has focus
		if (event.type == SDL_EVENT_QUIT) {
			ecs.set<ExitEvent>({ true });
		}
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0
			&& event.key.scancode == escapeMenuKey) {
			ecs.set<GamePauseEvent>({ true });
		}

		// The rest of the input is now gated
		if (io.WantTextInput || io.WantCaptureMouse) {
			return;
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
			&& event.button.button == leftClickKey) {
			ecs.set<MouseClickLeftEvent>({ event.button.x, event.button.y });
		}

		handleEditorEvents(event);
	}

	void handleEvents2(SDL_Event& event) {


	}


	// Put things in here that we don't want in distribution mode.
	void handleEditorEvents(SDL_Event& event) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == closeWindowKey) {
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