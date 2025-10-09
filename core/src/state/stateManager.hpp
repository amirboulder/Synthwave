#pragma once

#include <flecs.h>

#include "PlayerState.hpp"

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"

class StateManager {

public:

	RendererConfig& renderConfig;

	Renderer& renderer;
	TimeManager & time;

	flecs::world& ecs;

	flecs::entity playerCam;
	flecs::entity freeCam;

	AppContext appContext = AppContext::player;
	PlayState playState = PlayState::play;

	StateManager(flecs::world& ecs,RendererConfig & renderConfig,Renderer& renderer, TimeManager & time)
		: ecs(ecs), renderConfig(renderConfig), renderer(renderer), time(time)
	{
		applicationSetup();
	}


	void applicationSetup() {

		createCameras();
		setActiveCamera(appContext);

	}

	void createCameras() {

		playerCam = ecs.entity("PlayerCam")
			.emplace<Camera>(renderConfig);

		freeCam = ecs.entity("FreeCam")
			.emplace<Camera>(renderConfig);

	}


	bool save() {

		PlayerState::savePlayerState(ecs ,"data/playerState.json");

		//TODO change to game once we start saving game state
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "player saved successfully");
		return true;
	}

	bool load() {

		PlayerState::loadPlayerState(ecs ,"data/playerState.json");

		//TODO change to game once we start loading game state
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "player loaded successfully");
		return true;
	}

	void handleGamePause() {

		if (time.paused) {
			time.unPauseGame();
			playState = PlayState::play;

			
			SDL_SetWindowRelativeMouseMode(renderer.context.window, true);

			// flushing all the mouse movement accumulated during pause to avoid camera jerk
			float dx, dy;
			SDL_GetRelativeMouseState(&dx, &dy);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "play");

		}
		else {
			time.pauseGame();
			playState = PlayState::pause;
			SDL_SetWindowRelativeMouseMode(renderer.context.window, false);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "pause");
		}
	}

	void setActiveCamera(AppContext context) {

		switch (context) {

		case AppContext::player:
			playerCam.add<ActiveCamera>();
			freeCam.remove<ActiveCamera>();
			break;

		case AppContext::freeCam:
			playerCam.remove<ActiveCamera>();
			freeCam.add<ActiveCamera>();

			break;
		}
	}


	void handleCameraSwitch() {

		if (appContext == AppContext::player) {

			appContext = AppContext::freeCam;

			setActiveCamera(appContext);
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "using free camera");
		}
		else if (appContext == AppContext::freeCam) {

			appContext = AppContext::player;

			setActiveCamera(appContext);
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "using player camera");

		}
	}

	void handleSavingRenderConfig() {

		// maybe use get_mut
		const Camera& camera = freeCam.get<Camera>();

		renderConfig.FreeCamFront = camera.front;
		renderConfig.FreeCamPos = camera.position;

		renderConfig.FreeCamYaw = camera.yaw;
		renderConfig.FreeCamPitch = camera.pitch;

		//TODO modify function so the path is not hardcoded
		renderConfig.saveRendererConfigINI("config/renderConfig.ini");

	}
};