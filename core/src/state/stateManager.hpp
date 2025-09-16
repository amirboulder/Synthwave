#pragma once

#include "PlayerState.hpp"



class StateManager {

private:

	Player* player = nullptr;

public:

	AppContext appContext = AppContext::player;

	StateManager() = default;







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

};