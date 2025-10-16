#pragma once

#include <flecs.h>

#include "PlayerState.hpp"

#include "../renderer/renderer.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"

#include "../common.hpp"

class StateManager {

public:


	Renderer& renderer;
	TimeManager & time;

	flecs::world& ecs;

	flecs::entity playerCam;
	flecs::entity freeCam;

	AppContext appContext = AppContext::player;
	PlayState playState = PlayState::play;

	StateManager(flecs::world& ecs,Renderer& renderer, TimeManager & time)
		: ecs(ecs), renderer(renderer), time(time)
	{
		applicationSetup();
	}


	void applicationSetup() {

		createCameras();
		setActiveCamera(appContext);

	}

	void createCameras() {
		
		const RendererConfig& config = ecs.get<RendererConfig>();

		playerCam = ecs.entity("PlayerCam")
			.emplace<Camera>(config);

		freeCam = ecs.entity("FreeCam")
			.emplace<Camera>(config);

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

		//SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,"handleSavingRenderConfig FIX IT FIX IT FIX IT FIX IT!!!!");

		RendererConfig& config = ecs.get_mut<RendererConfig>();

		const Camera& camera = freeCam.get<Camera>();

		config.FreeCamFront = camera.front;
		config.FreeCamPos = camera.position;

		config.FreeCamYaw = camera.yaw;
		config.FreeCamPitch = camera.pitch;

		//TODO modify function so the path is not hardcoded
		RendererConfig::saveRendererConfigINIFile(ecs,"config/renderConfig.ini");
	}
};