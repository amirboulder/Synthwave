#pragma once

#include "core/src/pch.h"

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"


class StateManager {

public:

	flecs::world& ecs;

	Renderer& renderer;
	Fisiks& fisiks;
	Scene& scene;
	TimeManager& time;

	flecs::entity mainMenu;
	flecs::entity pauseMenu;

	flecs::entity playerCam;
	flecs::entity freeCam;

	flecs::entity player;

	flecs::entity inputPhase;
	flecs::system processUICommandsSys;

	bool & running ;

	StateManager(flecs::world& ecs,Renderer& renderer,Fisiks & fisiks ,TimeManager & time,Scene& scene ,bool& running)
		: ecs(ecs), renderer(renderer), time(time), fisiks(fisiks), running(running), scene(scene)
	{
		registerHooks();

		createComponents();

		createEntities();

		registerPhase();

		RegisterSystems();
		
		RegisterCustomPhaseDeps();

		disableDefaultPhases();

		getEntityHandles();
	}

	// Setting state here to make sure that all entity references such as player and mainMenu are valid
	// not strictly necessary because this class is constructed after most systems
	void init() {

		renderer.init();

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

	
		playStateOnSetHook();
		menuStateOnSetHook();
		cameraStateOnSetHook();
		InputStateOnSetHook();

	}

	void createComponents() {

		ecs.component<PlayState>().add(flecs::Singleton);
		ecs.component<MenuState>().add(flecs::Singleton);
		ecs.component<CameraState>().add(flecs::Singleton);
		ecs.component<InputState>().add(flecs::Singleton);

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
			.immediate() // disable readonly mode for this system
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

	void getEntityHandles() {

		playerCam = ecs.lookup("PlayerCam");
		freeCam = ecs.lookup("FreeCam");

		mainMenu = ecs.lookup("MainMenu");
		pauseMenu = ecs.lookup("PauseMenu");
	}


	void SetDefaultApplicationState() {

		// When application opens just show main menu

		ecs.set<PlayState>({ PlayState::NONE });
		ecs.set<MenuState>({ MenuState::MAIN });
		ecs.set<CameraState>({ CameraState::NONE });
		ecs.set<InputState>({ InputState::KBM });

	}

	void startGame() {

		flushMouseMovement();

		time.startGameTime();

		ecs.set<PlayState>({ PlayState::PLAY });
		ecs.set<MenuState>({ MenuState::NONE });
		ecs.set<CameraState>({ CameraState::PLAYER });


		fisiks.physicsPhase.enable();
		scene.aiUpdatePhase.enable();
		scene.playerPhase.enable();

		//enable here for now but will be incorporated into render settings later
		fisiks.physicsRenderPhase.enable();

	}


	bool save() {


		handleSavingRenderConfig();

		std::string path = "data/save1.json";

		json j;

		ecs.lookup("player").get<Player>().save(j);
		playerCam.get<Camera>().saveTransform(j);

		std::ofstream file(path);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open file for writing: " + path);
		}
		file << j.dump(4); // pretty-print

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "game saved successfully");
		return true;
	}

	bool load() {

		std::string path = "data/save1.json";
		std::ifstream file(path);
		if (!file.is_open()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open save file");
			return false;
		}

		json j;
		file >> j;

		ecs.lookup("player").get_mut<Player>().load(j);
		playerCam.get_mut<Camera>().loadTransform(j);

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "game saved successfully");
		return true;
	}

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


	// Switch between player and freeCam
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

	void menuStateOnSetHook() {
		
		//TODO create disable all menus using prefabs once there are more menus
		ecs.component<MenuState>()
			.on_set([&](MenuState& newState) {
			switch (newState) {
			case MenuState::MAIN:

				mainMenu.enable();
				pauseMenu.disable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::MAIN");
				break;

			case MenuState::PAUSE:

				mainMenu.disable();
				pauseMenu.enable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::PAUSE");
				break;

			case MenuState::OPTIONS:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::OPTIONS");
				break;
			case MenuState::NONE:
				
				//Disable all menus

				mainMenu.disable();
				pauseMenu.disable();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "MenuState::NONE");
				break;
			}
		});
	}

	void playStateOnSetHook() {

		const RenderConxtext& renderContext = ecs.get<RenderConxtext>();
		

		ecs.component<PlayState>()
			.on_set([&](PlayState& newState) {
			switch (newState) {
			case PlayState::PLAY:

				fisiks.physicsPhase.enable();
				scene.aiUpdatePhase.enable();
				flushMouseMovement();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::play");

				SDL_SetWindowRelativeMouseMode(renderContext.window, true);
				ecs.set<MenuState>({ MenuState::NONE });

				break;
			case PlayState::PAUSE:

				fisiks.physicsPhase.disable();
				scene.aiUpdatePhase.disable();
				
				SDL_SetWindowRelativeMouseMode(renderContext.window, false);

				ecs.set<MenuState>({ MenuState::PAUSE });

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::pause");
				break;
			case PlayState::NONE:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::None");

				break;

			}
		});
	}
	
	void cameraStateOnSetHook() {

		ecs.component<CameraState>()
			.on_set([&](CameraState& newState) {
			switch (newState) {
			case CameraState::PLAYER:

				playerCam.add<ActiveCamera>();
				freeCam.remove<ActiveCamera>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::PLAYER");

				break;
			case CameraState::FREECAM:

				playerCam.remove<ActiveCamera>();
				freeCam.add<ActiveCamera>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::FREECAM");

				break;
			case CameraState::NONE:

				playerCam.remove<ActiveCamera>();
				freeCam.remove<ActiveCamera>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "CameraState::NONE");

				break;
			}
		});
	}

	void InputStateOnSetHook() {

		ecs.component<InputState>()
			.on_set([&](InputState& newState) {
			switch (newState) {
			case InputState::KBM:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "InputState::KBM");
				break;

			case InputState::CONTROLLER:

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, " InputState::CONTROLLER");
				break;
			}
		});
	}
	


	void handleSavingRenderConfig() {


		RendererConfig& config = ecs.get_mut<RendererConfig>();

		const Camera& camera = ecs.lookup("FreeCam").get<Camera>();

		config.FreeCamFront = camera.front;
		config.FreeCamPos = camera.position;

		config.FreeCamYaw = camera.yaw;
		config.FreeCamPitch = camera.pitch;

		//TODO modify function so the path is not hardcoded
		RendererConfig::saveRendererConfigINIFile(ecs,"config/renderConfig.ini");
	}

	//TODO maybe put in common
	void flushMouseMovement() {

		// flushing all the mouse movement accumulated during pause/load to avoid camera jerk
		float dx, dy;
		SDL_GetRelativeMouseState(&dx, &dy);
	}


	void newGameCallback() {

		//This is needed because this code is runs in processUICommandsSys and systems are ran while the ecs is in "readonly" mode
		ecs.defer_suspend();
		scene.constructLevel();
		ecs.defer_resume();

		startGame();

	}


	void loadGameCallback() {

		load();

		//This is needed because this code is runs in processUICommandsSys and systems are ran while the ecs is in "readonly" mode
		ecs.defer_suspend();
		scene.constructLevel();
		ecs.defer_resume();
		
		startGame();
	}

	void saveGameCallback() {
		save();
	}

	void restartLevelCallback() {

	}

	void resumeGameCallback() {

		gamePauseHandler();
	}


	void gameOptionsCallback() {

	}

	void toMainMenuCallback () {

		mainMenu.enable();
		pauseMenu.disable();

		ecs.set<MenuState>({ MenuState::MAIN });

		//TODO Unload level

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