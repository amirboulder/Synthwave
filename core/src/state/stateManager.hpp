#pragma once

#include <fstream>
#include <iostream>

#include <flecs.h>

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"

class StateManager {


public:

	AppContext appContext = AppContext::menu;
	PlayState playState = PlayState::pause;

	Renderer& renderer;
	Scene& scene;
	TimeManager& time;

	flecs::world& ecs;

	flecs::entity mainMenu;
	flecs::entity playerCam;
	flecs::entity freeCam;
	flecs::entity player;


	bool & running ;

	StateManager(flecs::world& ecs,Renderer& renderer, TimeManager & time,Scene& scene ,bool& running)
		: ecs(ecs), renderer(renderer), time(time), running(running), scene(scene)
	{

		createEntities();

	}


	void init() {

		//TODO See if there is a benefit to initilizing systems like physics here instead of in their constructor
		
		createfisiksRenderer();

		getEntityHandles();

	}

	
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


	bool save() {


		handleSavingRenderConfig();

		std::string path = "data/save1.json";

		json j;

		ecs.lookup("player2").get<Player>().save(j);
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

		ecs.lookup("player2").get_mut<Player>().load(j);
		playerCam.get_mut<Camera>().loadTransform(j);

		//TODO change to game once we start loading game state
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "game saved successfully");
		return true;
	}

	void handleGamePause() {

		RenderConxtext& rendercontext = ecs.get_mut<RenderConxtext>();

		if (time.paused) {
			time.unPauseGame();
			playState = PlayState::play;

			
			SDL_SetWindowRelativeMouseMode(rendercontext.window, true);

			// flushing all the mouse movement accumulated during pause to avoid camera jerk
			float dx, dy;
			SDL_GetRelativeMouseState(&dx, &dy);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "play");

		}
		else {
			time.pauseGame();
			playState = PlayState::pause;
			SDL_SetWindowRelativeMouseMode(rendercontext.window, false);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "pause");
		}
	}

	void setApplicationState(AppContext newContext) {

		appContext = newContext;
		
	}

	

	void setActiveCamera() {



		assert(playerCam && "PlayerCam entity missing!");
		assert(freeCam && "FreeCam entity missing!");

		switch (appContext) {

		case AppContext::player:

			playerCam.add<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to player");
			break;

		case AppContext::freeCam:
			playerCam.remove<ActiveCamera>();
			freeCam.add<ActiveCamera>();
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to Menu");
			break;
		case AppContext::menu:
			playerCam.remove<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to freeCam");
			break;
		case AppContext::editor:
			playerCam.remove<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AppContext set to editor but its not implemented yet");
			break;

		}

	}


	void switchAppContext() {
		if (appContext == AppContext::player) {
			appContext = AppContext::freeCam;

			setActiveCamera();
		}
		else if (appContext == AppContext::freeCam) {
			appContext = AppContext::player;
			setActiveCamera();
		}
	}

	// Create ECS system to handle changs so we don't have to check this everyframe
	void updateState() {
		
		
		switch (appContext) {

		case AppContext::player:

			mainMenu.remove<Active>();
			//SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to player");
			break;

		case AppContext::freeCam:
		
			mainMenu.remove<Active>();

			//SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to Menu");
			break;
		case AppContext::menu:
			
			mainMenu.add<Active>();
			//DL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "AppContext set to freeCam");
			break;
		case AppContext::editor:
			
			mainMenu.remove<Active>();
			//SDL_LogError(SDL_LOG_CATEGORY_ERROR, "AppContext set to editor but its not implemented yet");
			break;

		}
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

	void newGameCallback() {

		const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

		setApplicationState(AppContext::player);
		setActiveCamera();

		SDL_SetWindowRelativeMouseMode(rendercontext.window, true);

		scene.constructLevel();

		time.startGameTime();
	}

	void loadGameCallback() {

		const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

		//TODO maybe lock cursor during load so the camera does not move 
		
		setApplicationState(AppContext::player);
		setActiveCamera();

		load();

		SDL_SetWindowRelativeMouseMode(rendercontext.window, true);

		scene.constructLevel();

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