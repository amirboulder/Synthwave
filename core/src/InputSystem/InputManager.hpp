#pragma once

enum class ControlMode {
	KBM,
	GAMEPAD
};

enum class InputContext : std::uint16_t {
	
	MOVEMENT,
	COMBAT,
};

enum class MouseButtons {

	BUTTON_INVALID,
	BUTTON_LEFT,
	BUTTON_MIDDLE,
	BUTTON_RIGHT,
	BUTTON_X1,
	BUTTON_X2,
};

struct KeyboardBinding {
	flecs::entity entity;
	SDL_Scancode key;
};

struct MouseBinding {
	flecs::entity entity;
	MouseButtons button;

};

struct GamepadBinding {
	flecs::entity entity;
	SDL_GamepadButton button;
};


/// TODO handle gamepad input
/// TODO translate mouseMovement and gamepad stick movement into movement intent for player class to consume,
/// that way we won't need to publish raw mouse movement data.

/// <summary>
/// Polls for input every render frame by reading keyboard and mouse state.
/// Also polls the event queue for missed input using a latch mechanism and miscellaneous events such as losing window focus.
/// Comprised of three main parts
/// 1.accumulateInput this is where raw input is read non gameplay events are triggered. Runs every render frame
/// 2.postInputSystem runs fist thing every ECS frame and sets ActionState for every occurred/ latched event.
/// 3.clearInputSystem runs last thing every ECS frame and clears mouse input.
/// Allows for input to be remapped to any key/button
/// An input cannot be mapped to a keyboard key and mouse button at the same time.
/// </summary>
class InputManager {

	flecs::world& ecs;

public:

	SDL_Gamepad* gamepad = nullptr;
	SDL_Gamepad* gamepad2 = nullptr; //Unused for now

	ControlMode controlMode = ControlMode::KBM;

	uint16_t escapeMenuKey = SDL_SCANCODE_ESCAPE;
	uint16_t closeWindowKey = SDL_SCANCODE_END;
	uint16_t testGamePadButton = SDL_GAMEPAD_BUTTON_SOUTH;           /**< Bottom face button (e.g. Xbox A button) */
	uint16_t leftClickKey = SDL_BUTTON_LEFT;
	uint16_t rightClickKey = SDL_BUTTON_RIGHT;

	flecs::entity inputPhase;
	flecs::system clearInputSys;
	flecs::system postInputSys;

	std::vector<KeyboardBinding> keyboardBindings;
	std::vector<MouseBinding> mouseBindings;
	std::vector<GamepadBinding> gamepadBindings;

	SDL_Event sdlEvent;

	InputManager(flecs::world& ecs)
		: ecs(ecs)
	{

		registerPhase();
		registerSystems();

		ecs.component<Direction>().add(flecs::Singleton);
		ecs.set<Direction>({ Direction::forward });

		ecs.component<MouseClickLeftEvent>().add(flecs::Singleton);

		flecs::entity interactEventEnt = ecs.entity("InteractEvent")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_E, interactEventEnt);

		flecs::entity Attack1EventEnt = ecs.entity("Attack1EventEnt")
			.set<ActionState>({});
		bindInputToMouse(MouseButtons::BUTTON_LEFT, Attack1EventEnt);

		flecs::entity forwardMVMTEnt = ecs.entity("forwardMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_W, forwardMVMTEnt);

		flecs::entity backwardMVMTEnt = ecs.entity("backwardMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_S, backwardMVMTEnt);

		flecs::entity leftMVMTEnt = ecs.entity("leftMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_A, leftMVMTEnt);

		flecs::entity rightMVMTEnt = ecs.entity("rightMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_D, rightMVMTEnt);

		flecs::entity jumpMVMTEnt = ecs.entity("jumpMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_SPACE, jumpMVMTEnt);

		ecs.component<MouseMovementState>().add(flecs::Singleton);
		ecs.set<MouseMovementState>({});

		LogSuccess(LOG_APP, "InputManager Initialized");
	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity inputPhaseDependency = ecs.entity("InputPhaseDependency");
		inputPhase = ecs.entity("InputPhase").add(flecs::Phase).depends_on(inputPhaseDependency);

	}

	void registerSystems() {

		clearInputSystem();
		postInputSystem();
	}


	/// <summary>
	/// accumulates all input data, Runs every Render frame
	/// Note the events triggered in this function should be non-gameplay events such as WindowLostFocusEvent,
	/// since Flecs observers/hook are called immediately.
	/// </summary>
	void accumulateInput() {


		while (SDL_PollEvent(&sdlEvent)) {

			ImGui_ImplSDL3_ProcessEvent(&sdlEvent);
			ImGuiIO& io = ImGui::GetIO();

			// We handle quit and escape BEFORE yielding to ImGui,
			// so escape can close the pause menu even when ImGui has focus
			if (sdlEvent.type == SDL_EVENT_QUIT) {
				ecs.set<ExitEvent>({ true });
			}
			if (sdlEvent.type == SDL_EVENT_KEY_DOWN && sdlEvent.key.repeat == 0
				&& sdlEvent.key.scancode == escapeMenuKey) {
				ecs.set<GamePauseEvent>({ true });
			}

			if (sdlEvent.type == SDL_EVENT_WINDOW_FOCUS_LOST) {

				ecs.set<WindowLostFocusEvent>({ true });
				LogDebug(LOG_INPUT, "Window lost focus!");
			}
			if (sdlEvent.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
				LogDebug(LOG_INPUT, "Window gained focus!");
			}

			if (sdlEvent.type == SDL_EVENT_GAMEPAD_ADDED) {

				if (gamepad != NULL) {
					LogWarn(LOG_APP, "A gamepad is already connected, adding a new gamepad will override the old one fix this!!!");
				}

				gamepad = SDL_OpenGamepad(sdlEvent.gdevice.which);
				SDL_GamepadType gamepadType = SDL_GetGamepadType(gamepad);
				LogInfo(LOG_APP, "Gamepad %s added", magic_enum::enum_name(gamepadType).data());
			}

			if (sdlEvent.type == SDL_EVENT_GAMEPAD_REMOVED && SDL_GetGamepadID(gamepad) == sdlEvent.gdevice.which) {

				SDL_GamepadType gamepadType = SDL_GetGamepadType(gamepad);
				LogInfo(LOG_APP, "Gamepad %s removed", magic_enum::enum_name(gamepadType).data());

				SDL_CloseGamepad(gamepad);
				gamepad = NULL;
			}

			// The rest of the input is now gated
			//If IMGUI wants the input then it will go to it
			if (io.WantTextInput || io.WantCaptureMouse) {
				continue;
			}

			if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
				&& sdlEvent.button.button == leftClickKey) {
				ecs.set<MouseClickLeftEvent>({ sdlEvent.button.x, sdlEvent.button.y });
			}
			handleEditorEvents(sdlEvent);


			if (sdlEvent.type == SDL_EVENT_KEY_DOWN || sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

				// Switch to KBM if needed
				if (controlMode != ControlMode::KBM) {
					controlMode = ControlMode::KBM;
					LogInfo(LOG_APP, "Switched to KBM mode!");
				}

				handleInput(sdlEvent);
			}
			
			else if (sdlEvent.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

				//controlMode = ControlMode::GAMEPAD;
				LogWarn(LOG_APP, "Switched to Gamepad mode NOT YET IMPLEMENTED!");
			}

		}

		if (controlMode == ControlMode::KBM) {
			pollInputKBM();

		}
		else {
			//TODO pollGamepad input.
		}


	}

	/// <summary>
	///This clears mouse movement before the start of the next ecs frame
	/// </summary>
	void clearInputSystem() {

		//Clear last frames input input
		clearInputSys = ecs.system("ClearInputSys")
			.kind(flecs::PostFrame)
			.run([&](flecs::iter& it) {

			//clear the accumulated mouse input
			MouseMovementState& mouseMovement = ecs.get_mut<MouseMovementState>();
			mouseMovement.deltaX = 0.0f;
			mouseMovement.deltaY = 0.0f;

		});

	}

	void postInputSystem() {

		//Happens first thing in the frame after input has been polled
		postInputSys = ecs.system("PostInputSys")
			.kind(inputPhase)
			.run([&](flecs::iter& it) {

			const float timeStep = ecs.get<TimeStep>().step;

			const ecs_world_info_t* info = ecs.get_info();
			int64_t currentFrame = info->frame_count_total;

			for (KeyboardBinding& keyBinding : keyboardBindings) {

				ActionState& state = keyBinding.entity.get_mut<ActionState>();

				setActionState(state, currentFrame, timeStep);
			}

			for (MouseBinding& mouseBinding : mouseBindings) {

				ActionState& state = mouseBinding.entity.get_mut<ActionState>();

				setActionState(state, currentFrame, timeStep);
			}

			//TODO gamepad input
		});

	}

	void setActionState(ActionState& state, const int64_t& currentFrame, const float& timeStep) {

		if (state.occurred || state.latch) {

			if (state.frameStamp == currentFrame - 1) {
				state.justPressed = false;
				state.occurredLast = true;
			}
			else {
				state.justPressed = true;
				state.occurredLast = false;
			}

			state.heldTime += timeStep;
			state.frameStamp = currentFrame;
			state.justReleased = false;
		}
		else {

			if (state.frameStamp == currentFrame - 1) {
				state.justReleased = true;
				state.occurredLast = true;
			}
			else {
				state.occurredLast = false;
				state.justReleased = false;
			}

			state.heldTime = 0.0f;
			state.justPressed = false;
		}

		state.latch = false;

	}

	void pollInputKBM() {

		const bool* keyStates = SDL_GetKeyboardState(NULL);

		MouseMovementState& mouseMovement = ecs.get_mut<MouseMovementState>();

		float deltaX, deltaY;
		SDL_MouseButtonFlags mouseState = SDL_GetRelativeMouseState(&deltaX, &deltaY);

		//Accumulate mouse movement because we may handle input more often than read it.
		mouseMovement.deltaX += deltaX;
		mouseMovement.deltaY += deltaY;

		for (const KeyboardBinding& keyBinding : keyboardBindings) {

			if (keyStates[keyBinding.key]) {

				ActionState& state = keyBinding.entity.get_mut<ActionState>();
				state.occurred = true;
			}
			else {
				ActionState& state = keyBinding.entity.get_mut<ActionState>();
				state.occurred = false;
			}
		}

		for (const MouseBinding& mouseBinding : mouseBindings) {

			if (mouseState & SDL_BUTTON_MASK(static_cast<int>(mouseBinding.button))) {

				ActionState& state = mouseBinding.entity.get_mut<ActionState>();
				state.occurred = true;
			}
			else {
				ActionState& state = mouseBinding.entity.get_mut<ActionState>();
				state.occurred = false;
			}
		}
	}

	void handleInput(SDL_Event& sdlEvent) {

		for (const KeyboardBinding& keyBinding : keyboardBindings) {

			if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {
				if (sdlEvent.key.scancode == keyBinding.key && !sdlEvent.key.repeat) {

					ActionState& state = keyBinding.entity.get_mut<ActionState>();
					state.latch = true;
				}
			}
			
		}

		for (const MouseBinding& mouseBinding : mouseBindings) {

			if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {

				//Do we need to check for repeat ?
				if (sdlEvent.button.button == static_cast<int>(mouseBinding.button)) {

					ActionState& state = mouseBinding.entity.get_mut<ActionState>();
					state.latch = true;
				}
			}
		}
	}

	bool bindInputToKeyboard(SDL_Scancode key, flecs::entity entity) {

		//If the Event is mapped to a Mouse button remove that mapping 
		for (size_t i = 0; i < mouseBindings.size(); i++) {

			MouseBinding& mouseBinding = mouseBindings[i];

			if (mouseBinding.entity == entity) {

				LogWarn(LOG_INPUT, "Event %s is already mapped to Mouse Button %s removing it", entity.name().c_str(), magic_enum::enum_name(mouseBinding.button).data());
				mouseBindings.erase(mouseBindings.begin() + i);
				break;
			}
		}

		//If the key is already mapped then reassign it
		for (KeyboardBinding& keyBinding : keyboardBindings) {

			if (keyBinding.key == key) {

				LogWarn(LOG_INPUT, " Key %s is already bound to event %s, rebinding it to %s", magic_enum::enum_name(keyBinding.key).data(), keyBinding.entity.name().c_str(), entity.name().c_str());
				keyBinding.entity = entity;
				return true;
			}
		}

		//If not bound at all simply bind it
		keyboardBindings.emplace_back(entity, key);
		LogVerbose(LOG_INPUT, "Key %s bound to event %s", magic_enum::enum_name(key).data(), entity.name().c_str());


		return true;
	}

	bool bindInputToMouse(const MouseButtons& button, flecs::entity entity) {

		//If the Event is mapped to a keyboard Key remove that mapping 
		for (size_t i = 0; i < keyboardBindings.size(); i++) {

			KeyboardBinding& keyBinding = keyboardBindings[i];

			if (keyBinding.entity == entity) {

				LogWarn(LOG_INPUT, "Event %s is already mapped to Keyboard key %s removing it", entity.name().c_str(), magic_enum::enum_name(keyBinding.key).data());
				keyboardBindings.erase(keyboardBindings.begin() + i);
				break;
			}
		}

		//If the key is already mapped then reassign it
		for (MouseBinding& mouseBinding : mouseBindings) {

			if (mouseBinding.button == button) {

				LogWarn(LOG_INPUT, "Button %s is already bound to event %s, rebinding it to %s", magic_enum::enum_name(mouseBinding.button).data(), mouseBinding.entity.name().c_str(), entity.name().c_str());
				mouseBinding.entity = entity;
				return true;
			}
		}

		//If not bound at all simply bind it
		mouseBindings.emplace_back(entity, button);
		LogVerbose(LOG_INPUT, "Button %s bound to event %s", magic_enum::enum_name(button).data(), entity.name().c_str());

		return true;
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

		// Switch between playerCam and freeCam
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F2) {

			ecs.set<CameraSwitchEvent>({ true });

		}

		//TODO change these to buttons in the editor
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F3) {

			ecs.set<PrintSystemsEvent>({ true });

		}

		if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0 && event.key.scancode == SDL_SCANCODE_F4) {

			ecs.set<PrintPhasesEvent>({ true });
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

};