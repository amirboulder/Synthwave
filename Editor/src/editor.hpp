#pragma once

#include "core/src/pch.h"

#include "../../core/src/EntityFactory.hpp"
#include "../../core/src/Serialization/serialization.hpp"
#include "../../core/src/ecs/RegisterReflectionData.hpp"

#include "EditorItems.hpp"
#include "SceneTree.hpp"

class Editor {

	flecs::entity editorToggle;

	flecs::query<> activeGameQuery;

public:

	flecs::world& ecs;

	Editor(flecs::world& ecs)
		: ecs(ecs)
	{
		registerReflectionData(ecs);

		const RendererConfig& config = ecs.get<RendererConfig>();
		ecs.entity("FreeCam")
			.emplace<Camera>(config);
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

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

		// Build a fresh query on each unload to avoid stale cache
		activeGameQuery = ecs.query_builder()
			.with<Game>()
			.term_at(0).self()
			.cascade(flecs::ChildOf)
			.build();

		activeGameQuery.each([&](flecs::entity e) {

			if (e.is_alive()) {

				if (e.has<JPH::BodyID>()) {

					const JPH::BodyID bodyID = e.get<JPH::BodyID>();

					if (bodyInterface.IsAdded(bodyID))
						bodyInterface.RemoveBody(bodyID);

					bodyInterface.DestroyBody(bodyID);
				}

				//Maybe not needed
				if (e.has<Player>()) {

					ecs.set<PlayerRef>({ flecs::entity::null()});
					ecs.set<PlayerCamRef>({ flecs::entity::null()});
				}

				e.destruct();
			}
		});

	}

	void saveGameToJson(const std::string& path) {
		rapidjson::Document jsonArray;
		jsonArray.SetArray();
		rapidjson::Document::AllocatorType& allocator = jsonArray.GetAllocator();

		// Build a fresh query on each unload to avoid stale cache
		activeGameQuery = ecs.query_builder()
			.with<Game>()
			.term_at(0).self()
			.cascade(flecs::ChildOf)
			.build();

		activeGameQuery.each([&](flecs::entity e) {

			if (!e.is_alive()) return;

			flecs::string entityJson = e.to_json();

			// Parse directly into a new Document
			rapidjson::Document entityDoc(&allocator);
			entityDoc.Parse(entityJson.c_str());

			// Move it into the array
			jsonArray.PushBack(entityDoc, allocator);
		});

		// Write to file with pretty formatting
		std::ofstream file(path);
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		writer.SetIndent(' ', 2);
		jsonArray.Accept(writer);

		file << buffer.GetString();
	}

	
	void loadGameFromJson(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "Failed to open " << path << "\n";
			return;
		}

		rapidjson::IStreamWrapper isw(file);
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if (doc.HasParseError()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "JSON parse error: %s at offset %s", doc.GetParseError(), doc.GetErrorOffset());
			return;
		}

		if (doc.IsObject()) {
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Successfully loaded JSON object");
		}

		if (!doc.IsArray()) {
			SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Root value is not a JSON array");
			return;
		}

	
		for (const auto& item : doc.GetArray()) {


			if (!item.IsObject()) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR item in json is not object!!!");

			}

			if (item.HasMember("components") && item["components"].IsObject()) {
				const auto& components = item["components"];

				if (components.HasMember("EntityType") && components["EntityType"].IsString()) {

					string entType = item["components"]["EntityType"].GetString();

					if (entType == "Game") {

						createGameEntFromJson(item);
					}
					else if (entType == "Scene") {

						createSceneEntFromJson(item);
					}
					else if (entType == "Player") {

						createPlayerEntFromJson(item);
					}
					else if (entType == "Capsule") {

						createCapsuleEntFromJson(item);
					}
					else if (entType == "Actor") {

						createActorEntFromJson(item);
					}
					else if (entType == "Sensor") {

						createSensorEntFromJson(item);
					}
					else if (entType == "Grid") {

						createGridEntFromJson(item);
					}
					else if (entType == "StaticMesh") {

						createStaticMeshEntFromJson(item);
					}
					else {

						SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Entity type %s exists in Game File", entType.c_str());
					}
				}
			}
		}
	}

	//TODO create entityFactory function
	bool createGameEntFromJson(const rapidjson::Value& item) {

		std::string name;

		if (!validateName(item)) return false;
		name = item["name"].GetString();

		//TODO create Factory function
			ecs.entity(name.c_str())
			.add<Game>()
			.set<EntityType>({ EntityType::Game })
			.add<IsActive>().add(flecs::CanToggle);

		return true;
	}

	//TODO create entityFactory function
	bool createSceneEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;

		if (!validateName(item)) return false;
		name = item["name"].GetString();

		if (!validateParent(item)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		//TODO create Factory function
		ecs.entity(name.c_str())
			.add<_Scene>()
			.set<EntityType>({ EntityType::Scene })
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(parentEnt);

		return true;
	}

	bool createPlayerEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;

		if (!validateParent(item)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");


		//Transforms does nothing yet
		Transform playerTransform;
		playerTransform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		playerTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		playerTransform.scale = glm::vec3(1.0f);

		if (!EntityFactory::createPlayerEntity(ecs, parentEnt, playerTransform)) {
			return false;
		}

		return true;
	}

	bool createCapsuleEntFromJson(const rapidjson::Value & item) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item)) return false;
		name = item["name"].GetString();
			
		if (!validateTransform(item)) return false;
		auto optTransform = serde::deserializeTransform(item["components"]["Transform"]);

		if (!optTransform) return false;
		transform = optTransform.value();

		//validate modelSrc
		if (!validateModelSrc(item)) return false;
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();
		
		if (!validateParent(item)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		if (!EntityFactory::createCapsuleEntity(ecs, parentEnt, name, "CapsuleModel", transform)) {
			return false;
		}

		return true;
	}

	bool createActorEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;
		Transform transform;

		//By the time we get here We know the item already has components

		if (!validateName(item)) return false;
		name = item["name"].GetString();

		if (!validateTransform(item)) return false;
		auto optTransform = serde::deserializeTransform(item["components"]["Transform"]);

		if (!optTransform) return false;
		transform = optTransform.value();

		if (!validateParent(item)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		if (!EntityFactory::createActorEntity(ecs, parentEnt, name, "ActorModel", transform, settings, actor1Update)) {
			return false;
		}

		return true;
	}

	bool createSensorEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;
		Transform transform;

		if (!validateName(item)) return false;
		if (!validateTransform(item)) return false;
		if (!validateParent(item)) return false;

		name = item["name"].GetString();
		parentName = item["parent"].GetString();

		std::optional<Transform> optTransform = serde::deserializeTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		JPH::Vec3 boxSensorSize = JPH::Vec3(15.0f, 15.0f, 15.0f);
		if (!EntityFactory::createBoxSensorEntity(ecs, parentEnt, name, transform, boxSensorSize, sensor1Behavior)) {
			return false;
		}

		return true;
	}

	bool createGridEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item)) return false;
		if (!validateTransform(item)) return false;
		if (!validateParent(item)) return false;
		if (!validateModelSrc(item)) return false;

		//TODO entityFactory validate Name 

		name = item["name"].GetString();
		parentName = item["parent"].GetString();
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();


		std::optional<Transform> optTransform = serde::deserializeTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		if (!EntityFactory::createGridEntity(ecs, parentEnt, name, modelSrcName,transform, "pipelineGrid", 256, 256)) {
			return false;
		}

		return true;
	}

	bool createStaticMeshEntFromJson(const rapidjson::Value& item) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item)) return false;
		if (!validateTransform(item)) return false;
		if (!validateParent(item)) return false;
		if (!validateModelSrc(item)) return false;

		//TODO entityFactory validate Name 

		name = item["name"].GetString();
		parentName = item["parent"].GetString();
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();


		std::optional<Transform> optTransform = serde::deserializeTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		if (!EntityFactory::createStaticMeshEntity(ecs, parentEnt, name, modelSrcName, transform, "pipelineMtn")) {
			return false;
		}

		return true;
	}


	bool validateParent(const rapidjson::Value& item) const {

		if (!item.HasMember("parent") || !item["parent"].IsString()) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR Json entity does not have parent");
			return false;
		}

		std::string parentName = item["parent"].GetString();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		if (!parentEnt.is_valid()) {
			
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR parentEnt name read from json is invalid");
			return false;
		}

		return true;

	}

	// does not check if components is valid because we assume that has already been checked
	bool validateName(const rapidjson::Value& item) {

		if (!item.HasMember("name") || !item["name"].IsString()) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR this poor entity doesn't even have a name!");
			return false;
		}
		return true;
	}

	bool validateComponents(const rapidjson::Value& item) {

	}

	bool validateModelSrc(const rapidjson::Value& item) {

		if (!item["components"].HasMember("ModelSourceRef")) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR components does not have ModelSourceRef");
			return false;
		}

		if (!item["components"]["ModelSourceRef"].HasMember("name")) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR components does not have ModelSourceRef name");
			return false;
		}

		if (!item["components"]["ModelSourceRef"]["name"].IsString()) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR components does not have ModelSourceRef name string");
			return false;
		}
		return true;
	}

	//validates and gets transform from json
	bool validateTransform(const rapidjson::Value& item) {

		if (!item["components"].HasMember("Transform")) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "ERROR components does not have Transform");
			return false;
		}
		return true;		
	}
};


