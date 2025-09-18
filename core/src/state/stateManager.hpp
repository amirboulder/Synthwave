#pragma once

#include "PlayerState.hpp"

#include "../renderer/renderer.hpp"
#include "../renderer/CameraManager.hpp"

#include "../time/timeManager.hpp"


class StateManager {

private:

	Player* player = nullptr;


	Renderer& renderer;
	CameraManager& cameraManager;
	TimeManager & time;

public:

	AppContext appContext = AppContext::player;
	PlayState playState = PlayState::play;

	StateManager(Renderer& renderer, CameraManager& cameraManager, TimeManager & time)
		: renderer(renderer), cameraManager(cameraManager), time(time)
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
			throw std::runtime_error("Player not set in StateManager!");
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

	void pauseHandler() {

		if (time.paused) {
			time.unPauseGame();
			playState = PlayState::play;

			
			SDL_SetWindowRelativeMouseMode(renderer.window, true);

			// flushing all the mouse movement accumulated during pause to avoid camera jerk
			float dx, dy;
			SDL_GetRelativeMouseState(&dx, &dy);

		}
		else {
			time.pauseGame();
			playState = PlayState::pause;
			SDL_SetWindowRelativeMouseMode(renderer.window, false);
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

};