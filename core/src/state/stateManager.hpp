#pragma once

#include "PlayerState.hpp"

#include "../renderer/renderer.hpp"
#include "../renderer/CameraManager.hpp"

#include"../renderer/RendererConfig.hpp"

#include "../time/timeManager.hpp"


class StateManager {

private:

	Player* player = nullptr;

	RendererConfig& renderConfig;

	Renderer& renderer;
	CameraManager& cameraManager;
	TimeManager & time;

public:

	AppContext appContext = AppContext::player;
	PlayState playState = PlayState::play;

	StateManager(RendererConfig & renderConfig,Renderer& renderer, CameraManager& cameraManager, TimeManager & time)
		: renderConfig(renderConfig), renderer(renderer), cameraManager(cameraManager), time(time)
	{
		applicationSetup();
	}


	void applicationSetup() {

		setActiveCamera(appContext);
	}



	void setPlayer(Player& ps) {
		player = &ps;
	}

	Player& getPlayer() {
		if (!player) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error Player not set in StateManager! ");
		}
		return *player;
	}

	bool save() {

		
		PlayerState::savePlayerState(getPlayer(), "data/playerState.json");


		cout << "game saved sucessfully\n";
		return true;
	}

	bool load() {


		PlayerState::loadPlayerState(getPlayer(), "data/playerState.json");




		cout << "game loaded sucessfully\n";
		return true;
	}

	void handleGamePause() {

		if (time.paused) {
			time.unPauseGame();
			playState = PlayState::play;

			
			SDL_SetWindowRelativeMouseMode(renderer.window, true);

			// flushing all the mouse movement accumulated during pause to avoid camera jerk
			float dx, dy;
			SDL_GetRelativeMouseState(&dx, &dy);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "play");

		}
		else {
			time.pauseGame();
			playState = PlayState::pause;
			SDL_SetWindowRelativeMouseMode(renderer.window, false);

			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "pause");
		}
	}

	void setActiveCamera(AppContext context) {

		switch (context) {

		case AppContext::player:
			renderer.activeCamera = &cameraManager.playerCam;
			break;

		case AppContext::freeCam:
			renderer.activeCamera = &cameraManager.freeCam;
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

		Camera& camera = cameraManager.freeCam;

		renderConfig.FreeCamFront = camera.front;
		renderConfig.FreeCamPos = camera.position;

		renderConfig.FreeCamYaw = camera.yaw;
		renderConfig.FreeCamPitch = camera.pitch;

		//TODO modify function so the path is not hardcoded
		renderConfig.saveRendererConfigINI("config/renderConfig.ini");

	}

};