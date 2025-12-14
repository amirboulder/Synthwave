#pragma once

#include "core/src/pch.h"

#include "../../core/src/EntityFactory.hpp"
#include "../../core/src/Serialization/serialization.hpp"
#include "../../core/src/ecs/RegisterReflectionData.hpp"

#include "SceneTree.hpp"
#include "RagdollBuilder.hpp"

class Editor {

	flecs::entity editorToggle;

	flecs::query<> activeGameQuery;

	flecs::entity freeCam;

public:

	flecs::world& ecs;

	Editor(flecs::world& ecs)
		: ecs(ecs)
	{
		registerReflectionData(ecs);

		const RendererConfig& config = ecs.get<RendererConfig>();
		freeCam = ecs.entity("FreeCam")
			.emplace<Camera>(config)
			.set<CameraMVMTState>({false});
	}

	//This is called by StateManager
	void init() {

		registerPrefab();

		registerEditorItems();

		registerQuery();

		editorToggle.disable();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Editor Initialized");
	}

	// All editor Items are created using the prefab editorComponent this allows us to disable all of them by disabling editorComponent
	void registerPrefab() {

		editorToggle = ecs.prefab("editorComponent");
	}

	void registerQuery() {

		activeGameQuery = ecs.query_builder()
			.with<Game>()
			.term_at(0).self()
			.cascade(flecs::ChildOf)
			.build();
	}

	void registerEditorItems() {

		EntityFactory::createEditorItemEntity(ecs, "SceneTree", editorToggle, SceneTree::SceneTreeDraw);
		EntityFactory::createEditorItemEntity(ecs, "RagdollCreator", editorToggle, RagdollBuilder::draw);
	}

	void disable() {

		editorToggle.disable();

	}

	void enable() {

		editorToggle.enable();

		freeCam.get_mut<CameraMVMTState>().locked = true;

	}

	// TODO Remove
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


	static void setEditorCamPos(flecs::world& ecs,glm::vec3 newPos) {

		flecs::entity freeCamEnt =  ecs.lookup("FreeCam");
		freeCamEnt.get_mut<Camera>().position = newPos;

	}

};


