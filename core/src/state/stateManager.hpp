#pragma once

#include "core/src/pch.h"

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"


class StateManager {

public:

	Renderer& renderer;
	Fisiks& fisiks;
	Scene& scene;
	TimeManager& time;

	flecs::world& ecs;

	flecs::entity mainMenu;
	flecs::entity pauseMenu;

	flecs::entity playerCam;
	flecs::entity freeCam;

	flecs::entity player;

	flecs::system processUICommandsSys;

	bool & running ;

	StateManager(flecs::world& ecs,Renderer& renderer,Fisiks & fisiks ,TimeManager & time,Scene& scene ,bool& running)
		: ecs(ecs), renderer(renderer), time(time), fisiks(fisiks), running(running), scene(scene)
	{
		createHooks();

		createComponents();

		createEntities();

		RegisterSystems();

	}


	void init() {

		getEntityHandles();

		mainMenu.enable();
		pauseMenu.disable();
	}


	void startGame() {


		flushMouseMovement();

		time.startGameTime();
		ecs.set<PlayState>({ true });

		//enable physics updates
		fisiks.physicsPhase.enable();

		// enable Scene updates
		scene.sceneUpdatePhase.enable();
		

	}


	void createEntities() {

		std::function<void()> newGameFunction = [this]() { this->newGameCallback(); };
		ecs.entity("newGameHandler").emplace<Callback>(newGameFunction);

		std::function<void()> loadGameFunction = [this]() { this->loadGameCallback(); };
		ecs.entity("loadGameHandler").emplace<Callback>(loadGameFunction);

		std::function<void()> gameOptionsFunction = [this]() { this->gameOptionsCallback(); };
		ecs.entity("gameOptionsHandler").emplace<Callback>(gameOptionsFunction);

		std::function<void()> toMainMenuFunction = [this]() { this->toMainMenuCallback(); };
		ecs.entity("MainMenuHandler").emplace<Callback>(toMainMenuFunction);

		std::function<void()> exitFunction = [this]() { this->exitCallback(); };
		ecs.entity("ExitHandler").emplace<Callback>(exitFunction);

	}



	void createHooks() {

		appContextHook();

	}

	void createComponents() {

		ecs.add<AppContext>();

		ecs.component<PlayState>().add(flecs::Singleton);
		ecs.set<PlayState>({});
	}


	void RegisterSystems() {


		processUICommandsSys = ecs.system<UICommand>("processUICommandsSys")
			.kind(flecs::PreUpdate)
			.immediate() // disable readonly mode for this system
			.each([&](flecs::entity e, UICommand command) {
			
			switch (command.type) {
			case UICommandType::NewGame:
				ecs.lookup("newGameHandler").get<Callback>().callbackFunction();
				break;
			case UICommandType::LoadGame:
				ecs.lookup("loadGameHandler").get<Callback>().callbackFunction();
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

	void handleGamePause() {

		enum AppContext::Type appContext = ecs.get<AppContext>().value;

		//TODO fix bug where game can be paused from main menu
			
		bool & play = ecs.get_mut<PlayState>().play;
		if (play) {
			
			play = false;

			pauseMenu.enable();

			ecs.set<AppContext>({ AppContext::Menu });

			fisiks.physicsPhase.disable();
			scene.sceneUpdatePhase.disable();
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::pause");

		}
		else {

			play = true;

			pauseMenu.disable();

			ecs.set<AppContext>({ AppContext::Player });

			fisiks.physicsPhase.enable();
			scene.sceneUpdatePhase.enable();
			flushMouseMovement();
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "PlayState::play");
		}

	}


	// Switch between player and freeCam
	void switchAppContext() {

		const enum AppContext::Type appContext = ecs.get_mut<AppContext>().value;

		if (appContext == AppContext::Player) {
			ecs.set<AppContext>({ AppContext::FreeCam });
		}
		else if (appContext == AppContext::FreeCam) {
			ecs.set<AppContext>({ AppContext::Player });
		}
	}

	// Runs whenever appContext is changed
	// Ensures that ActiveCamera Menu and RelativeMouseMode are set correctly
	void appContextHook() {

		//assert(playerCam && "PlayerCam entity missing!");
		//assert(freeCam && "FreeCam entity missing!");

		const RenderConxtext& renderContext = ecs.get<RenderConxtext>();

		ecs.component<AppContext>()
			.on_set([&](AppContext& ctx) {
			switch (ctx.value) {
			case AppContext::Menu:

			//	mainMenu.enable();

				playerCam.remove<ActiveCamera>();
				freeCam.remove<ActiveCamera>();
				SDL_SetWindowRelativeMouseMode(renderContext.window, false);

				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to menu");
				break;
			case AppContext::Player:

				mainMenu.disable();

				playerCam.add<ActiveCamera>();
				freeCam.remove<ActiveCamera>();

				SDL_SetWindowRelativeMouseMode(renderContext.window, true);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to player");
				break;
			case AppContext::FreeCam:

				mainMenu.disable();

				playerCam.remove<ActiveCamera>();
				freeCam.add<ActiveCamera>();

				SDL_SetWindowRelativeMouseMode(renderContext.window, true);
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to freeCam");
				break;
			case AppContext::Editor:

				mainMenu.disable();

				playerCam.remove<ActiveCamera>();
				freeCam.remove<ActiveCamera>();

				SDL_SetWindowRelativeMouseMode(renderContext.window, false);
				SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Context set to editor (not implemented)");
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

		ecs.set<AppContext>({ AppContext::Player });

		//This is needed because this code is runs in processUICommandsSys and systems are ran while the ecs is in "readonly" mode
		ecs.defer_suspend();
		scene.constructLevel();
		ecs.defer_resume();

		startGame();

	}

	void loadGameCallback() {

		ecs.set<AppContext>({ AppContext::Player });
		//TODO check if a level is loaded if so then destroy the level or reset it
		load();

		//This is needed because this code is runs in processUICommandsSys and systems are ran while the ecs is in "readonly" mode
		ecs.defer_suspend();
		scene.constructLevel();
		ecs.defer_resume();
		
		startGame();
	}

	void gameOptionsCallback() {

	}

	void toMainMenuCallback () {

		mainMenu.enable();
		pauseMenu.disable();

		ecs.set<AppContext>({ AppContext::Menu });

		//TODO Unload level

	}

	void exitCallback() {
		running = false;
	}
};