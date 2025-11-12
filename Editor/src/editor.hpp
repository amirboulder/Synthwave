#pragma once

#include "core/src/pch.h"

#include "../../core/src/EntityFactory.hpp"

#include "../../core/src/ecs/RegisterReflectionData.hpp"

#include "EditorItems.hpp"
#include "SceneTree.hpp"

class Editor {

	flecs::entity editorToggle;

	flecs::query<> activeGameQuery;

public:
	
	flecs::world& ecs;

	Editor(flecs::world& ecs)
		:	ecs(ecs)
	{
		registerReflectionData(ecs);
	}

	//This is called by StateManager
	void init() {

		registerPrefab();

		registerEditorItems();

		registerQuery();

		editorToggle.disable();
		
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

		EntityFactory::createEditorItemEntity(ecs, "SceneTree", editorToggle,SceneTree::SceneTreeDraw);
	}

	void disable() {

		editorToggle.disable();

	}

	void enable() {

		editorToggle.enable();

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

	
	void unload() {

		ecs.defer_begin();
		activeGameQuery.each([&](flecs::entity e) {

			e.destruct();

		});
		ecs.defer_end();

	}

	//using flecs to serialize for now
	void saveGameEntities2(const std::string& path = "data/game1.json") {

		
		std::ostringstream jsonArray;
		jsonArray << "[\n";

		bool first = true;
		activeGameQuery.each([&](flecs::entity e) {
			if (!first) {
				jsonArray << ",\n";
			}
			first = false;

			// Serialize each entity with its components and relationships
			flecs::string entityJson = e.to_json();
			jsonArray << "  " << entityJson.c_str();
		});

		jsonArray << "\n]";

		std::ofstream out(path);
		out << jsonArray.str();
		out.close();
	}

	//Deprecated
	/*
	void loadGameEntities2(const std::string& path = "data/game.json") {
		std::ifstream in(path);
		std::string content((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
		in.close();

		// Simple JSON array parser
		std::vector<std::string> entities;
		size_t pos = content.find('[');
		if (pos == std::string::npos) return;

		int braceCount = 0;
		size_t startPos = 0;
		bool inString = false;
		char prevChar = '\0';

		for (size_t i = pos + 1; i < content.length(); i++) {
			char c = content[i];

			// Track string boundaries (escaped quotes don't count)
			if (c == '"' && prevChar != '\\') {
				inString = !inString;
			}

			if (!inString) {
				if (c == '{') {
					if (braceCount == 0) {
						startPos = i;
					}
					braceCount++;
				}
				else if (c == '}') {
					braceCount--;
					if (braceCount == 0) {
						entities.push_back(content.substr(startPos, i - startPos + 1));
					}
				}
			}

			prevChar = c;
		}

		// Load each entity
		for (const auto& entityJson : entities) {
			//std::cout << entityJson << std::endl;
			flecs::entity newEnt = ecs.entity();
			const char* result = newEnt.from_json(entityJson.c_str());
		}
	}
	*/

	//Still needs work
	// only  works for capsules for now
	//TODO
	// GRID
	//MTN
	//ACtor
	void loadGameEntities(const std::string& path = "data/game.json") {

		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "Failed to open " << path << "\n";
			return;
		}

		nlohmann::json j;
		file >> j;

	
		for (auto& ent : j) {
			// Safely extract ObjectType name
			std::string name = ent.value("name", "Unknown");

			std::string type = "";

			if (ent.contains("components") && ent["components"].is_object()) {
				const auto& comps = ent["components"];
				if (comps.contains("ObjectType") && comps["ObjectType"].is_object()) {
					const auto& ot = comps["ObjectType"];
					if (ot.contains("name") && ot["name"].is_string()) {
						type = ot["name"].get<std::string>();
					}
				}
			}

			if (type == "Capsule") {
				std::string name = ent.value("name", "Unnamed");
				Transform transform = ent["components"]["Transform"].get<Transform>();

				std::string parentName = ent["parent"].get<std::string>();

				// Lookup the parent entity by its path
				flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

				// Debug print
				if (parentEnt.is_valid()) {
					std::cout << "Found parent: " << parentEnt.name() << std::endl;

					EntityFactory::createCapsuleEntity(ecs, parentEnt, name, "CapsuleModel", transform);
				}
				else {
					std::cout << "Parent not found: " << parentName << std::endl;
				}

				
			}
		}
	}

};
