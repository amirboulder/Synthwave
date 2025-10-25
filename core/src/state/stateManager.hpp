#pragma once

#include "core/src/pch.h"

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"


class StateManager {

	PlayState playState = PlayState::pause;

public:


	Renderer& renderer;
	Scene& scene;
	TimeManager& time;

	flecs::world& ecs;

	flecs::entity mainMenu;
	flecs::entity playerCam;
	flecs::entity freeCam;
	flecs::entity player;

	flecs::system processContextChanges;

	bool & running ;

	StateManager(flecs::world& ecs,Renderer& renderer, TimeManager & time,Scene& scene ,bool& running)
		: ecs(ecs), renderer(renderer), time(time), running(running), scene(scene)
	{
		createHooks();

		createComponents();

		createEntities();

		createECSSystems();

	}


	void init() {

		createfisiksRenderer();

		getEntityHandles();

	}

	//TODO See if there is a benefit to initilizing systems like physics here instead of in their constructor
	void initSystems() {

		//init physics

		//init Time 

		// init Scene

	}


	void createEntities() {

		std::function<void()> newGameFunction = [this]() { this->newGameCallback(); };
		ecs.entity("newGame").emplace<Callback>(newGameFunction);

		std::function<void()> loadGameFunction = [this]() { this->loadGameCallback(); };
		ecs.entity("loadGame").emplace<Callback>(loadGameFunction);

		std::function<void()> gameOptionsFunction = [this]() { this->gameOptionsCallback(); };
		ecs.entity("gameOptions").emplace<Callback>(gameOptionsFunction);

		std::function<void()> exitFunction = [this]() { this->exitCallback(); };
		ecs.entity("Exit").emplace<Callback>(exitFunction);

	}



	void createHooks() {

		appContextHook();

	}

	void createComponents() {

		ecs.add<AppContext>();
	}


	void createECSSystems() {

		// This system is manually run on
		processContextChanges = ecs.system<stateChangeRequest>("processContextChanges")
			.each([&](flecs::entity e,stateChangeRequest request) {

			ecs.set<AppContext>({ request.newContext });

			e.destruct();
		});


		

	}

	void getEntityHandles() {


		playerCam = ecs.lookup("PlayerCam");
		freeCam = ecs.lookup("FreeCam");

		mainMenu = ecs.lookup("Main Menu");
	}


	void createfisiksRenderer() {
#ifdef JPH_DEBUG_RENDERER

		ecs.emplace<fisiksDebugRenderer>(ecs);

#endif
	}

	//Run all systems in here
	void update() {

		processContextChanges.run();
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

		RenderConxtext& rendercontext = ecs.get_mut<RenderConxtext>();

		if (time.paused) {
			time.unPauseGame();
			playState = PlayState::play;

			
			SDL_SetWindowRelativeMouseMode(rendercontext.window, true);

			flushMouseMovement();

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "play");

		}
		else {
			time.pauseGame();
			playState = PlayState::pause;
			SDL_SetWindowRelativeMouseMode(rendercontext.window, false);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "pause");
		}
	}



	
	void setActiveCamera() {

		assert(playerCam && "PlayerCam entity missing!");
		assert(freeCam && "FreeCam entity missing!");

		enum AppContext::Type appContext = ecs.get<AppContext>().value;

		switch (appContext) {

		case AppContext::Player:

			playerCam.add<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			break;

		case AppContext::FreeCam:
			playerCam.remove<ActiveCamera>();
			freeCam.add<ActiveCamera>();
			break;
		case AppContext::Menu:
			playerCam.remove<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			break;
		case AppContext::Editor:
			playerCam.remove<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			break;

		}

	}

	// Switch between player and freeCam
	void switchAppContext() {

		enum AppContext::Type appContext = ecs.get<AppContext>().value;

		if (appContext == AppContext::Player) {
			appContext = AppContext::FreeCam;
		}
		else if (appContext == AppContext::FreeCam) {
			appContext = AppContext::Player;
		}
	}

	// This exists to ensure that menu and Camera are set correctly
	void appContextHook() {

		ecs.component<AppContext>()
			.on_set([&](AppContext& ctx) {
			switch (ctx.value) {
			case AppContext::Menu:
				mainMenu.add<Active>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to menu");
				break;
			case AppContext::Player:
				mainMenu.remove<Active>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to player");
				break;
			case AppContext::FreeCam:
				mainMenu.remove<Active>();
				SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Context set to freeCam");
				break;
			case AppContext::Editor:
				mainMenu.remove<Active>();
				SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Context set to editor (not implemented)");
				break;
			}

			setActiveCamera();
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

		const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

		ecs.defer([&] {
			ecs.entity()
				.set<stateChangeRequest>({ AppContext::Player });

		});

		scene.constructLevel();

		SDL_SetWindowRelativeMouseMode(rendercontext.window, true);

		flushMouseMovement();

		time.startGameTime();
	}

	void loadGameCallback() {

		const RenderConxtext& renderContext = ecs.get<RenderConxtext>();

		ecs.defer([&] {
			ecs.entity()
				.set<stateChangeRequest>({ AppContext::Player });
		});

		//check if a level is loaded if so then destroy the level or reset it

		load();

		SDL_SetWindowRelativeMouseMode(renderContext.window, true);

		scene.constructLevel();

		flushMouseMovement();

		time.startGameTime();

	}

	void gameOptionsCallback() {

	}

	void toMainMenuCallback () {

	}

	void exitCallback() {
		running = false;
	}
};