#pragma once

enum class ControlMode {
	KBM,
	GAMEPAD
};

enum class PollingMode : std::uint16_t {
	DISCRETE,
	CONTINUOUS,
};

enum class InputContext : std::uint16_t {
	
	MOVEMENT,
	COMBAT,
};


enum class MouseButtons {

	BUTTON_INVALID,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_X1,
	BUTTON_X2,
};



struct KeyboardBinding {
	SDL_Scancode key;
	PollingMode pollingMode;
	flecs::entity entity;
};

struct MouseBinding {
	MouseButtons button;
	PollingMode pollingMode;
	flecs::entity entity;
};

struct GamepadBinding {
	SDL_Scancode button;
	PollingMode pollingMode;
	flecs::entity entity;
};



/// <summary>
///
/// </summary>
class InputManager {

	flecs::world& ecs;

public:

	SDL_Gamepad* gamepad;
	SDL_Gamepad* gamepad2; //Unused

	ControlMode controlMode = ControlMode::KBM;

	uint16_t escapeMenuKey = SDL_SCANCODE_ESCAPE;
	uint16_t closeWindowKey = SDL_SCANCODE_END;
	uint16_t testGamePadButton = SDL_GAMEPAD_BUTTON_SOUTH;           /**< Bottom face button (e.g. Xbox A button) */
	uint16_t leftClickKey = SDL_BUTTON_LEFT;
	uint16_t rightClickKey = SDL_BUTTON_RIGHT;

	flecs::entity inputPhase;
	flecs::system clearInputSys;

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
		//ecs.set<MouseClickEvent>({});

		flecs::entity interactEventEnt = ecs.entity("InteractEvent")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_E, interactEventEnt, PollingMode::DISCRETE);

		flecs::entity forwardMVMTEnt = ecs.entity("forwardMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_W, forwardMVMTEnt, PollingMode::CONTINUOUS);

		flecs::entity backwardMVMTEnt = ecs.entity("backwardMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_S, backwardMVMTEnt, PollingMode::CONTINUOUS);

		flecs::entity leftMVMTEnt = ecs.entity("leftMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_A, leftMVMTEnt, PollingMode::CONTINUOUS);

		flecs::entity rightMVMTEnt = ecs.entity("rightMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_D, rightMVMTEnt, PollingMode::CONTINUOUS);

		flecs::entity jumpMVMTEnt = ecs.entity("jumpMVMTEnt")
			.set<ActionState>({});
		bindInputToKeyboard(SDL_SCANCODE_SPACE, jumpMVMTEnt, PollingMode::CONTINUOUS);

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
	}


	/// <summary>
	/// Handles all input
	/// Note this runs for every draw ,
	/// but acts as a latch meaning it it runs multiple times before an ecs.progress() it will accumulate input
	/// meaning it will all be considered as a part of one frame until its cleared in clearInputSystem during flecs::post frame
	/// </summary>
	void accumulateInput() {

		const float dt = ecs.get<DeltaTime>().dt;

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
			//If imgui wants the input then it will go to it
			if (io.WantTextInput || io.WantCaptureMouse) {
				return;
			}

			if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN
				&& sdlEvent.button.button == leftClickKey) {
				ecs.set<MouseClickLeftEvent>({ sdlEvent.button.x, sdlEvent.button.y });
			}
			handleEditorEvents(sdlEvent);

			//Verify that this check is comprehensive
			if (sdlEvent.type == SDL_EVENT_KEY_DOWN || sdlEvent.type == SDL_EVENT_KEY_UP) {

				// Switch to KBM if needed
				if (controlMode != ControlMode::KBM) {
					controlMode = ControlMode::KBM;
					LogInfo(LOG_APP, "Switched to KBM mode!");
				}

				handleEventKeyboard(sdlEvent, dt);
			}
			
			else if (sdlEvent.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

				//controlMode = ControlMode::KBM;
				LogWarn(LOG_APP, "Switched to Gamepad mode NOT YET IMPLEMENTED!");
			}


		}

		pollInputKBM();

	}

	/// <summary>
	///This happens last in the frame and sets up input state for the next frame
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

	void handleEventKeyboard(SDL_Event& sdlEvent, const float dt) {


		for (KeyboardBinding& keyBinding : keyboardBindings) {

			if (keyBinding.pollingMode == PollingMode::CONTINUOUS) {
				continue;
			}

			if (keyBinding.key == sdlEvent.key.scancode) {

				ActionState& state = keyBinding.entity.get_mut<ActionState>();

				if (sdlEvent.type == SDL_EVENT_KEY_DOWN) {

					LogInfo(LOG_APP, "KeyDOWN ");

					state.occurred = true;

					if (sdlEvent.key.repeat == 0) {
						state.justPressed = true;
						LogInfo(LOG_APP, "KeyDOWN NEW");
					}
					else {
						state.justPressed = false;
						state.occurredLast = true;
						LogInfo(LOG_APP, "KeyDOWN repeat");
					}

					state.heldTime += dt;
				}

				if(sdlEvent.type == SDL_EVENT_KEY_UP){
					state.occurred = false;
					state.justReleased = true;
					state.occurredLast = true;
					//LogInfo(LOG_APP, "KeyUP");
				}
			}
		}
	}

	void pollInputKBM() {

		const bool* keyStates = SDL_GetKeyboardState(NULL);

		MouseMovementState& mouseMovement = ecs.get_mut<MouseMovementState>();

		float deltaX, deltaY;
		SDL_MouseButtonFlags mouseState = SDL_GetRelativeMouseState(&deltaX, &deltaY);

		//Accumulate mouse movement because we handle input more often
		mouseMovement.deltaX += deltaX;
		mouseMovement.deltaY += deltaY;

		for (KeyboardBinding keyBinding : keyboardBindings) {

			if (keyBinding.pollingMode == PollingMode::DISCRETE) {
				continue;
			}

			if (keyStates[keyBinding.key]) {

				ActionState& state = keyBinding.entity.get_mut<ActionState>();
				state.occurred = true;
			}
			else {
				ActionState& state = keyBinding.entity.get_mut<ActionState>();
				state.occurred = false;
			}
		}

		for (MouseBinding mouseBinding : mouseBindings) {

			if (mouseBinding.pollingMode == PollingMode::DISCRETE) {
				continue;
			}

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

	bool bindInputToKeyboard(SDL_Scancode key, flecs::entity entity, PollingMode pollingMode) {

		//If the Event is mapped to a Mouse button remove that mapping 
		for (int i = 0; i < mouseBindings.size(); i++) {

			MouseBinding mouseBinding = mouseBindings[i];

			if (mouseBinding.entity == entity) {

				LogWarn(LOG_INPUT, "Event %s is already mapped to Mouse Button %s removing it", entity.name().c_str(), magic_enum::enum_name(mouseBinding.button));
				mouseBindings.erase(mouseBindings.begin() + i);
				break;
			}
		}

		//If the key is already mapped then reassign it
		for (KeyboardBinding& keyBinding : keyboardBindings) {

			if (keyBinding.key == key) {

				LogWarn(LOG_INPUT, " Key %s is already bound to event %s, rebinding it!", magic_enum::enum_name(key), entity.name().c_str());
				keyBinding.entity = entity;
				return true;
			}
		}

		//If not bound at all simply bind it
		keyboardBindings.emplace_back(key, pollingMode, entity);

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