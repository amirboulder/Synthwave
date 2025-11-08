#pragma once

#include "core/src/pch.h"

#include "MenuFunctions.hpp"

class MenuSystem {

public:

	flecs::world& ecs;
	flecs::entity mainMenu;
	flecs::entity pauseMenu;



	MenuSystem(flecs::world& ecs)
		:ecs(ecs)
	{
		registerEntities();
	}

	void registerEntities() {

		
		mainMenu = EntityFactory::createMenuItemEntity(ecs, "MainMenu", Menu::mainMenuDraw);
	
		pauseMenu = EntityFactory::createMenuItemEntity(ecs, "PauseMenu", Menu::pauseMenuDraw);

		//TODO options menu

		//load Game submenu

		//new Game submenu
	}

};