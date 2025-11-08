#pragma once

#include "core/src/pch.h"

#include "../../core/src/EntityFactory.hpp"

#include "EditorItems.hpp"

class Editor {

	flecs::entity editorToggle;

public:
	
	flecs::world& ecs;

	Editor(flecs::world& ecs)
		:	ecs(ecs)
	{
		// Everything about the editor should be in init because 
	}

	//This is called by StateManager
	void init() {

		registerPrefab();

		registerEditorItems();

		RegisterSampleGame();

		// Disabled by default
		disable();
	}

	// All editor Items are created using the prefab editorComponent this allows us to disable all of them by disabling editorComponent
	void registerPrefab() {

		editorToggle = ecs.prefab("editorComponent");
	}

	void registerEditorItems() {

		EntityFactory::createEditorItemEntity(ecs, "SceneTree", editorToggle,EditorNS::SceneTreeDraw);
	}

	void disable() {

		editorToggle.disable();

	}

	void enable() {

		editorToggle.enable();

	}

	void RegisterSampleGame() {

		flecs::entity sampleGame = ecs.entity("Sample Game")
			.add<Game>()
			.add<IsActive>().add(flecs::CanToggle);

		flecs::entity sampleScene1 = ecs.entity("Sample Scene1")
			.add<_Scene>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleGame);

		flecs::entity sampleObject1 = ecs.entity("Sample Object1")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleObject2 = ecs.entity("Sample Object2")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleObject3 = ecs.entity("Sample Object3")
			.add<DynamicEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleScene2 = ecs.entity("Sample Scene2")
			.add<_Scene>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleGame);

		flecs::entity sampleObject4 = ecs.entity("Sample Object4")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene2);
	}

};
