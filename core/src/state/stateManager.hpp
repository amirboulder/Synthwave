#pragma once

#include "core/src/pch.h"

#include "../renderer/renderer.hpp"
#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../MenuSystem/MenuSystem.hpp"

#include "../../../Editor/src/editor.hpp"

#include "../Serialization/serialization.hpp"

#include "../common.hpp"
#include "../ecs/eventComponents.hpp"

//TODO seperete all event based systems into their own class

class StateManager {

public:

	flecs::world& ecs;

	Renderer& renderer;
	Fisiks& fisiks;
	Serializer& serde;
	MenuSystem& menuSys;
	Editor& editor;
	Scene& scene;
	TimeManager& time;


	flecs::entity inputPhase;
	flecs::system processUICommandsSys;

	bool & running ;

	StateManager(flecs::world& ecs,Renderer& renderer,Fisiks & fisiks,Serializer & serde ,MenuSystem & menuSys,Editor & editor ,TimeManager & time,Scene& scene ,bool& running)
		: ecs(ecs), renderer(renderer),fisiks(fisiks), serde(serde), menuSys(menuSys), editor(editor), time(time), scene(scene), running(running)
	{
		registerHooks();

		createComponents();

		registerObservers();

		createEntities();

		registerPhase();

		RegisterSystems();
		
		RegisterCustomPhaseDeps();

		disableDefaultPhases();


		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "StateManager Initialized" RESET);

	}

	// Setting state here to make sure that all entity references such as player and mainMenu are valid
	// not strictly necessary because this class is constructed after most systems
	void init() {

		renderer.initSubSystems();

		editor.init();

		SetDefaultApplicationState();
	}


	void createEntities() {

		std::function<void()> newGameFunction = [this]() { this->newGameCallback(); };
		ecs.entity("newGameHandler").emplace<Callback>(newGameFunction);

		std::function<void()> loadGameFunction = [this]() { this->loadGameCallback(); };
		ecs.entity("loadGameHandler").emplace<Callback>(loadGameFunction);

		std::function<void()> saveGameFunction = [this]() { this->saveGameCallback(); };
		ecs.entity("saveGameHandler").emplace<Callback>(saveGameFunction);

		std::function<void()> restartLevelFunction = [this]() { this->restartLevelCallback(); };
		ecs.entity("restartLevelHandler").emplace<Callback>(restartLevelFunction);

		std::function<void()> resumeGameFunction = [this]() { this->resumeGameCallback(); };
		ecs.entity("ResumeGameHandler").emplace<Callback>(resumeGameFunction);

		std::function<void()> gameOptionsFunction = [this]() { this->gameOptionsCallback(); };
		ecs.entity("gameOptionsHandler").emplace<Callback>(gameOptionsFunction);

		std::function<void()> toMainMenuFunction = [this]() { this->toMainMenuCallback(); };
		ecs.entity("MainMenuHandler").emplace<Callback>(toMainMenuFunction);

		std::function<void()> exitFunction = [this]() { this->exitCallback(); };
		ecs.entity("ExitHandler").emplace<Callback>(exitFunction);

	}



	void registerHooks() {

		gameLoadedOnSetHook();
		playStateOnSetHook();
		menuStateOnSetHook();
		cameraStateOnSetHook();

		InputStateOnSetHook();
		EditorStateOnSetHook();

		//Event hooks
		mouseClickEventHook();
		exitEventHook();
		windowLostFocusEvent();
		gamePauseEventHook();
		editorToggleHook();
		cameraSwitchEventHook();
		physicsRenderToggleEventHook();
		saveGameSrcEventHook();
		ragdollSavedEventHook();

	}

	void createComponents() {

		ecs.component<GameLoadedState>().add(flecs::Singleton);
		ecs.set<GameLoadedState>({ GameLoadedState::NotLoaded });

		ecs.component<PlayState>().add(flecs::Singleton);
		ecs.set<PlayState>({ PlayState::NONE });

		ecs.component<MenuState>().add(flecs::Singleton);
		ecs.set<MenuState>({ MenuState::MAIN });

		ecs.component<CameraState>().add(flecs::Singleton);
		ecs.set<CameraState>({ CameraState::NONE });

		ecs.component<EditorState>().add(flecs::Singleton);
		ecs.set<EditorState>({ EditorState::NONE });

		ecs.component<InputDeviceState>().add(flecs::Singleton);
		ecs.set<InputDeviceState>({ InputDeviceState::KBM });


	}

	void registerObservers() {

		MenuObserver();
		gameLoadedStateObserver();
	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, thats handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity inputPhaseDependency = ecs.entity("InputPhaseDependency");
		inputPhase = ecs.entity("InputPhase").add(flecs::Phase).depends_on(inputPhaseDependency);

	}

	//sets the order of execution for systems
	void RegisterCustomPhaseDeps() {

		flecs::entity inputPhaseDependency = ecs.lookup("InputPhaseDependency");
		flecs::entity physicsPhaseDependency = ecs.lookup("PhysicsPhaseDependency").depends_on(inputPhaseDependency);
		flecs::entity aiPhaseDependency = ecs.lookup("AIPhaseDependency").depends_on(physicsPhaseDependency);
		//TODO Audio phase
		//TODO maybe gameState phase
		flecs::entity playerPhaseDependency = ecs.lookup("PlayerPhaseDependency").depends_on(aiPhaseDependency);

		flecs::entity physicsRenderPhaseDependency = ecs.entity("PhysicsRenderPhaseDependency").depends_on(flecs::PostFrame);

		/* This still works with flecs builtin pipeline query :
		world.pipeline()
		  .with(flecs::System)
		  .with(flecs::Phase).cascade(flecs::DependsOn)
		  .without(flecs::Disabled).up(flecs::DependsOn)
		  .without(flecs::Disabled).up(flecs::ChildOf)
		  .build();
		*/
	}

	void disableDefaultPhases() {

		// Disable most the default phases so we don't see them

		//ecs.entity(flecs::PreFrame).disable();
		ecs.entity(flecs::OnLoad).disable();
		ecs.entity(flecs::PostLoad).disable();
		ecs.entity(flecs::PreUpdate).disable();
		ecs.entity(flecs::OnUpdate).disable();
		ecs.entity(flecs::OnValidate).disable();
		ecs.entity(flecs::PostUpdate).disable();
		ecs.entity(flecs::PreStore).disable();
		ecs.entity(flecs::OnStore).disable();
		//ecs.entity(flecs::PostFrame).disable();

	}


	
	void RegisterSystems() {

		processUICommandsSystem();
	}

	void processUICommandsSystem() {

		processUICommandsSys = ecs.system<UICommand>("processUICommandsSys")
			.kind(inputPhase)
			.immediate() // disable read-only mode for this system
			.each([&](flecs::entity e, UICommand command) {
			
			switch (command.type) {
			case UICommandType::NewGame:
				ecs.lookup("newGameHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::SaveGame:
				ecs.lookup("saveGameHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::LoadGame:
				ecs.lookup("loadGameHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::RestartLevel:
				ecs.lookup("restartLevelHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::ResumeGame:
				ecs.lookup("ResumeGameHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::GameOptions:
				ecs.lookup("gameOptionsHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::MainMenu:
				ecs.lookup("MainMenuHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::ExitGame:
				ecs.lookup("ExitHandler").get<Callback>().callbackFunction();
				break;

			}
			e.destruct();
		});
	}




	void SetDefaultApplicationState() {

		// When application opens just show main menu

		ecs.set<GameLoadedState>({ GameLoadedState::NotLoaded });
		ecs.set<PlayState>({ PlayState::NONE });
		ecs.set<MenuState>({ MenuState::MAIN });
		ecs.set<CameraState>({ CameraState::NONE });
		ecs.set<InputDeviceState>({ InputDeviceState::KBM });
		ecs.set<EditorState>({ EditorState::NONE });

	}

	// PlayState and EditorState react to GameLoadedState and MenuState reacts to them
	void startGame() {

		CMN::flushMouseMovement();

		time.startGameTime();

		ecs.set<GameLoadedState>({ GameLoadedState::Loaded });
		ecs.set<CameraState>({ CameraState::PLAYER });

		fisiks.physicsPhase.enable();
		scene.aiUpdatePhase.enable();
		scene.playerPhase.enable();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,GOOD "🚀 Game started" RESET);
	}


	//Switch between play and pause by triggering PlayState onSet hooks.
	void gamePauseHandler() {

		PlayState state = ecs.get<PlayState>();

		if (state == PlayState::NONE) {

			return;
		}
		else if (state == PlayState::PLAY) {

			ecs.set<PlayState>(PlayState::PAUSE);
		}
		else if (state == PlayState::PAUSE) {

			ecs.set<PlayState>(PlayState::PLAY);
		}
	}


	// Switch between player and freeCam by triggering CameraState hooks
	void cameraSwitchHandler() {

		CameraState state = ecs.get<CameraState>();

		if (state == CameraState::PLAYER) {
			ecs.set<CameraState>({ CameraState::FREECAM });
		}
		else if (state == CameraState::FREECAM) {
			ecs.set<CameraState>({ CameraState::PLAYER });
		}
	}


	void togglePhysicsRenderer() {

		if (ecs.entity<fisiksDebugRenderer>().enabled<fisiksDebugRenderer>()) {

			ecs.entity<fisiksDebugRenderer>().disable<fisiksDebugRenderer>();
		}
		else {

			ecs.entity<fisiksDebugRenderer>().enable<fisiksDebugRenderer>();
		}
	}

	// if GameLoadedState is set to loaded it sets EditorState to Disabled which allows it to be toggled
	// if GameLoadedState is NotLoaded Failed it set EditorState to none which prevents things like turning on the editor in main menu/
	void toggleEditor() {

		EditorState state = ecs.get<EditorState>();

		if (state == EditorState::Enabled) {

			ecs.set<EditorState>({ EditorState::Disabled });
			ecs.set<PlayState>({ PlayState::PLAY });

		}
		else if (state == EditorState::Disabled) {

			//this is here to prevent opening the editor when in pause Menu
			PlayState playState = ecs.get<PlayState>();
			if (playState == PlayState::PAUSE) {
				return; // Can't open editor from paused game
			}

			ecs.set<EditorState>({ EditorState::Enabled });
			ecs.set<PlayState>({ PlayState::PAUSE });

		}
	}

	// Empty for now but might be used later
	void gameLoadedOnSetHook() {

		ecs.component<GameLoadedState>()
			.on_set([&](GameLoadedState& newState) {
			switch (newState) {
			case GameLoadedState::NotLoaded:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GameLoadedState::NotLoaded");
				break;

			case GameLoadedState::Loaded:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GameLoadedState::Loaded");
				break;

			case GameLoadedState::Failed:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GameLoadedState::Failed");
				break;
			}
		});

	}

	
	void menuStateOnSetHook() {
		
		//TODO create disable all menus using prefabs once there are more menus
		ecs.component<MenuState>()
			.on_set([&](MenuState& newState) {

			switch (newState) {
			case MenuState::MAIN:

				menuSys.mainMenu.enable();
				menuSys.pauseMenu.disable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::MAIN");
				break;

			case MenuState::PAUSE:

				menuSys.mainMenu.disable();
				menuSys.pauseMenu.enable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::PAUSE");
				break;

			case MenuState::OPTIONS:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::OPTIONS");
				break;
			case MenuState::NONE:
				
				//Disable all menus

				menuSys.mainMenu.disable();
				menuSys.pauseMenu.disable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::NONE");
				break;
			}
		});
	}

	//Note: When game is paused we DO NOT pause time but rather disable game systems.
	void playStateOnSetHook() {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		

		ecs.component<PlayState>()
			.on_set([&](PlayState& newState) {
			switch (newState) {
			case PlayState::PLAY:

				fisiks.physicsPhase.enable();
				scene.aiUpdatePhase.enable();
				scene.playerPhase.enable();

				CMN::flushMouseMovement();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::play");

				SDL_SetWindowRelativeMouseMode(renderContext.window, true);
				break;
			case PlayState::PAUSE:

				fisiks.physicsPhase.disable();
				scene.aiUpdatePhase.disable();
				scene.playerPhase.disable();
				
				SDL_SetWindowRelativeMouseMode(renderContext.window, false);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::pause");
				break;
			case PlayState::NONE:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::None");

				break;

			}
		});
	}

	//PlayerCam may not be created yet which is why we have all the if statements here
	void cameraStateOnSetHook() {

		ecs.component<CameraState>()
			.on_set([&](CameraState& newState) {

			flecs::entity playerCam = flecs::entity::null();

			if (auto ref = ecs.try_get<PlayerCamRef>()) {         
				flecs::entity cam = ref->value;               

				if (cam.is_alive()) {                        
					playerCam = cam;
				}
			}

			//TODO change to use FreeCam ref for this too!
			flecs::entity freeCam = ecs.lookup("FreeCam");

			switch (newState) {
			case CameraState::PLAYER:

				if (playerCam.is_valid()) {
					playerCam.add<ActiveCamera>();
				};

				if (freeCam.is_valid()) {
					freeCam.remove<ActiveCamera>();
				};

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::PLAYER");

				break;
			case CameraState::FREECAM:

				if (playerCam.is_valid()) {
					playerCam.remove<ActiveCamera>();
				};

				if (freeCam.is_valid()) {
					freeCam.add<ActiveCamera>();
				};

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::FREECAM");

				break;
			case CameraState::NONE:

				if (playerCam.is_valid()) {
					playerCam.remove<ActiveCamera>();
				};

				if (freeCam.is_valid()) {
					freeCam.remove<ActiveCamera>();
				};
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::NONE");

				break;
			}

		});
	}

	void InputStateOnSetHook() {

		ecs.component<InputDeviceState>()
			.on_set([&](InputDeviceState& newState) {
			switch (newState) {
			case InputDeviceState::KBM:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "InputState::KBM");
				break;

			case InputDeviceState::CONTROLLER:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, " InputState::CONTROLLER");
				break;
			}
		});
	}

	void EditorStateOnSetHook() {

		ecs.component<EditorState>()
			.on_set([&](EditorState& newState) {
			switch (newState) {

			case EditorState::Enabled:

				editor.enable();

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EditorState::Enabled");
				break;

			case EditorState::Disabled:

				editor.disable();

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EditorState::Disabled");
				break;

			case EditorState::NONE:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "EditorState::NONE");
				break;
			}

		});
	}

	//If mouse click event returns a validEnt then an entitySelected event is triggered
	void mouseClickEventHook() {

		ecs.component<MouseClickEvent>()
			.on_set([&](MouseClickEvent& event) {

			uint32_t entID = renderer.readFromTexture(event.x, event.y);
			flecs::entity ent = ecs.entity(entID);
			
			editor.entitySelected(ent);
			

			});
	}


	void exitEventHook() {

		ecs.component<ExitEvent>()
			.on_set([&](ExitEvent& event) {

			if (event.occurred == true) {
				//we could just set running to false here but calling exitCallback for consistency.
				exitCallback();
			}
		});
	}

	void windowLostFocusEvent() {

		ecs.component<WindowLostFocusEvent>()
			.on_set([&](WindowLostFocusEvent& event) {

			if (event.occurred == true) {

				//If ediotor is enabled don't do anything
				const EditorState* editorState = ecs.try_get<EditorState>();
				if (editorState) {
					if (*editorState == EditorState::Enabled) {
						return;
					}
				}
				
				gamePauseHandler();
			}
		});
	}

	void gamePauseEventHook() {

		ecs.component<GamePauseEvent>()
			.on_set([&](GamePauseEvent& event) {

			if (event.occurred == true) {

				gamePauseHandler();
			}
		});
	}

	void editorToggleHook() {

		ecs.component<EditorToggleEvent>()
			.on_set([&](EditorToggleEvent& event) {

			if (event.occurred == true) {
				
				toggleEditor();
			}
		});
	}

	void cameraSwitchEventHook() {

		ecs.component<CameraSwitchEvent>()
			.on_set([&](CameraSwitchEvent& event) {

			if (event.occurred == true) {
				
				cameraSwitchHandler();
			}
		});
	}

	void physicsRenderToggleEventHook() {

		ecs.component<PhysicsRenderToggleEvent>()
			.on_set([&](PhysicsRenderToggleEvent& event) {

			if (event.occurred == true) {

				togglePhysicsRenderer();
			}
		});
	}


	void saveGameSrcEventHook() {

		ecs.component<SaveGameSrcEvent>()
			.on_set([&](SaveGameSrcEvent& event) {

			if (event.occurred == true) {

				saveGameSRCData();
			}
		});
	}

	void ragdollSavedEventHook() {

		ecs.component<RagdollSavedEvent>()
			.on_set([&](RagdollSavedEvent& event) {

			if (event.occurred == true) {

				AssetLibRef ref = ecs.get<AssetLibRef>();

				ref.assetLib->scanForRagdolls();
			}
		});
	}
	
	//TODO Maybe fix the redundant calls at some point
	void MenuObserver() {

		ecs.observer<PlayState, EditorState>()
			.term_at(0).src<PlayState>()
			.term_at(1).src<EditorState>()
			.event(flecs::OnSet)
			.each([](flecs::iter& it, size_t i, PlayState& play, EditorState& editor) {


			if (editor == EditorState::Enabled) {
				it.world().set<MenuState>({ MenuState::NONE });
				return;
			}

			switch (play) {
			case PlayState::PLAY:
				it.world().set<MenuState>({ MenuState::NONE });
				break;
			case PlayState::PAUSE:
				it.world().set<MenuState>({ MenuState::PAUSE });
				break;
			}
		});
	}

	// menuState reacts to editorState changes
	void menuEditorStateObserver() {

		ecs.observer<EditorState>()
			.term_at(0).src<EditorState>()
			.event(flecs::OnSet)
			.each([](flecs::iter& it, size_t i,EditorState& editor) {

			// If we're in a menu do
			if (it.world().get<MenuState>() != MenuState::NONE) {
				return;
			}
			if (editor == EditorState::Enabled) {
				it.world().set<MenuState>({ MenuState::NONE });
				return;
			}

			
		});

	}

	// PlayState and EditorState react to GameLoadedState
	void gameLoadedStateObserver() {

		ecs.observer<GameLoadedState>()
			.term_at(0).src<GameLoadedState>()
			.event(flecs::OnSet)
			.each([](flecs::iter& it, size_t i, GameLoadedState& gls) {

			switch (gls) {
			case GameLoadedState::NotLoaded:

				it.world().set<PlayState>({ PlayState::NONE });
				it.world().set<EditorState>({ EditorState::NONE });
				break;

			case GameLoadedState::Loaded:

				it.world().set<PlayState>({ PlayState::PLAY });
				it.world().set<EditorState>({ EditorState::Disabled });

				break;

			case GameLoadedState::Failed:

				it.world().set<PlayState>({ PlayState::NONE });
				it.world().set<EditorState>({ EditorState::NONE });

				break;
			}
		});		
	}

	void handleSavingRenderConfig() {

		RenderConfig& config = ecs.get_mut<RenderConfig>();

		const Camera& camera = ecs.lookup("FreeCam").get<Camera>();

		config.FreeCamFront = camera.front;
		config.FreeCamPos = camera.position;

		config.FreeCamYaw = camera.yaw;
		config.FreeCamPitch = camera.pitch;

		//TODO modify function so the path is not hardcoded
		RenderConfig::saveRendererConfigINIFile(ecs,"config/renderConfig.ini");
	}

	void saveGameSRCData() {


		fs::path path = fs::path(__FILE__).lexically_normal();
		fs::path repoRoot = path.parent_path().parent_path().parent_path().parent_path();
		fs::path jsonPath = repoRoot / "games" / "CrashTheSim" / "src" / "GameData.json";

		//Get current time using SDL
		SDL_Time now;
		SDL_GetCurrentTime(&now);    // ticks since epoch

		SDL_DateTime dt;
		if (!SDL_TimeToDateTime(now, &dt, true)) {   // true = local time
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to convert time.");
			return;
		}

		//Build timestamp
		char timestamp[64];
		SDL_snprintf(timestamp, sizeof(timestamp),
			"%04d-%02d-%02d_%02d-%02d-%02d",
			dt.year, dt.month, dt.day,
			dt.hour, dt.minute, dt.second);

		// Create backup folder path
		fs::path backupFolder = jsonPath.parent_path() / "backups";

		// Ensure the backup folder exists
		fs::create_directories(backupFolder);

		// Create backup file path inside the backups folder
		fs::path backupPath =
			backupFolder /
			(jsonPath.stem().string() +
				"_backup_" + timestamp +
				jsonPath.extension().string());

		//Copy original to backup
		if (fs::exists(jsonPath)) {
			fs::copy_file(jsonPath, backupPath, fs::copy_options::overwrite_existing);
		}

		//Save new game data
		serde.saveGameToJson(jsonPath.string());

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Saved GameData.json");
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Backup created: %s", backupPath.string().c_str());

	}

	
	//defer_suspend is needed for loadGameFromJson because each entity validates its parent because being created
	// and without defer_suspend the validation would fail because a parent ent such as scene won't fully exist before it children are created
	//Secondly unload needs defer_suspend to make sure it deletes ents before loadGameFromJson loads them again.
	//This run in processUICommandsSys and systems are ran while the ecs is in "read-only" mode
	void newGameCallback() {

		ecs.defer_suspend();

		if (serde.gameLoaded) {
			serde.unload();
		}


		fs::path repoRoot = util::getRepoRoot();
		fs::path jsonPath = repoRoot / "games" / "CrashTheSim" / "src" / "GameData.json";

		//Load from game src
		serde.loadGameFromJson(jsonPath.string());

		ecs.defer_resume();

		startGame();

	}

	//defer_suspend is needed for loadGameFromJson because each entity validates its parent because being created
	// and without defer_suspend the validation would fail because a parent ent such as scene won't fully exist before it children are created
	//Secondly unload needs defer_suspend to make sure it deletes ents before loadGameFromJson loads them again.
	//This run in processUICommandsSys and systems are ran while the ecs is in "read-only" mode
	void loadGameCallback(const std::string filePath = "data/GameSave1.json") {

		ecs.defer_suspend();
		if (serde.gameLoaded) {
			serde.unload();  
		}

		bool success = serde.loadGameFromJson(filePath);

		ecs.defer_resume();

		if (!success) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Failed to load game" RESET);
			ecs.entity().set<UICommand>({ UICommandType::MainMenu });
			return;
		}
		startGame();
	}

	// TODO receive the name of the save file
	void saveGameCallback(const std::string filePath = "data/GameSave1.json") {

		//This save just saves the render config 
		//TODO move that somewhere else
		handleSavingRenderConfig();
		
		serde.saveGameToJson(filePath);
	}

	void restartLevelCallback() {

	}

	void resumeGameCallback() {

		gamePauseHandler();
	}


	void gameOptionsCallback() {

	}

	void toMainMenuCallback () {


		serde.unload();
		
		ecs.set<MenuState>({ MenuState::MAIN });

		ecs.set<GameLoadedState>({ GameLoadedState::NotLoaded });


	}


	void printSystems() {

		cout << "Print systems in pipeline execution order\n";
		// Query systems in pipeline execution order
		auto pipeline_query = ecs.query_builder<>()
			.with(flecs::System)
			.with(flecs::Phase).cascade(flecs::DependsOn)
			.without(flecs::Disabled).up(flecs::DependsOn)
			.without(flecs::Disabled).up(flecs::ChildOf)
			.build();


		pipeline_query.each([&](flecs::iter& it, size_t i) {

			flecs::entity system = it.entity(i);

			cout << "  System: " << system.name() << std::endl;

		});
	}

	void printPhases() {

		cout << "Print phases in pipeline execution order\n";
		// Query systems in pipeline execution order
		auto pipeline_query = ecs.query_builder<>()
			.with(flecs::Phase)
			.without(flecs::Disabled).up(flecs::DependsOn)
			.without(flecs::Disabled).up(flecs::ChildOf)
			.build();


		pipeline_query.each([&](flecs::iter& it, size_t i) {

			flecs::entity phase = it.entity(i);

			cout << "  Phase: " << phase.name() << std::endl;

		});
	}

	void exitCallback() {
		running = false;
	}
};