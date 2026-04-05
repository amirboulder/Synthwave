#pragma once

// Does both serialization and deserialization
class Serializer {

    flecs::world& ecs;
    flecs::query<> activeGameQuery;

public:

	bool gameLoaded = false;

    Serializer(flecs::world& ecs)
        : ecs(ecs)
    {
        registerQuery();

		LogSuccess(LOG_APP, "Serializer Initialized");
    }

    void registerQuery() {

        activeGameQuery = ecs.query_builder()
            .with<Game>()
            .term_at(0).self()
            .cascade(flecs::ChildOf)
            .build();
    }

    //TODO make sure everything is unloaded
	void unload() {
		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

		// We must collect entities to delete first then delete them outside the query for two reasons
		// 1. Normally the table is locked during iteration so this would cause an error unless its run as a part of a system which will defer table modifications
		// 2. This may be called from a defer_suspend() block such as when load game is called from pause menu. which will cause an error if we delete while iterating

		std::vector<flecs::entity> entitiesToDelete;

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
				if (e.has<Player>()) {
					ecs.set<PlayerRef>({ flecs::entity::null() });
					ecs.set<PlayerCamRef>({ flecs::entity::null() });
				}
				entitiesToDelete.push_back(e);
			}
		});

		// Now delete all entities after the query iteration is complete
		for (flecs::entity e : entitiesToDelete) {
			e.destruct();
		}

		gameLoaded = false;
	}

    //Currently does not check if the file already exist. it will override it
    bool saveGameToJson(const std::string& path) {
        rapidjson::Document jsonArray;
        jsonArray.SetArray();
        rapidjson::Document::AllocatorType& allocator = jsonArray.GetAllocator();

        // Build a fresh query on each save to avoid stale cache
        activeGameQuery = ecs.query_builder()
            .with<Game>()
            .term_at(0).self()
            .cascade(flecs::ChildOf)
            .build();

        activeGameQuery.each([&](flecs::entity e) {

            if (!e.is_alive()) return false;

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

		LogSynth(LOG_APP, "💾 saved to  %s", path.c_str());

		return true;
    }

	bool loadGameFromJson(const std::string& path) {

		std::ifstream file(path);
		if (!file.is_open()) {
			LogError(LOG_APP, "Failed to open Game file %s", path.c_str());
			return false;
		}

		rapidjson::IStreamWrapper isw(file);
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if (doc.HasParseError()) {
			LogError(LOG_APP, "JSON parse error: %s at offset %s in file", doc.GetParseError(), doc.GetErrorOffset(), path.c_str());
			return false;
		}

		if (doc.IsObject()) {
			LogDebug(LOG_APP, "Successfully loaded JSON object from file %s", path.c_str());
		}

		if (!doc.IsArray()) {
			LogError(LOG_APP, "Root value is not a JSON array in file %s", path.c_str());
			return false;
		}


		for (const auto& item : doc.GetArray()) {

			if (!item.IsObject()) {

				std::string itemName;
				if (item.IsString()) {
					itemName = item.GetString();
				}

				LogError(LOG_APP, "ERROR item %s in json is not object in file %s", itemName.c_str(), path.c_str());

			}

			if (item.HasMember("components") && item["components"].IsObject()) {
				const auto& components = item["components"];

				if (components.HasMember("EntityType") && components["EntityType"].IsString()) {

					string entType = item["components"]["EntityType"].GetString();

					if (entType == "Game") {

						createGameEntFromJson(item, path);
					}
					else if (entType == "Scene") {

						createSceneEntFromJson(item, path);
					}
					else if (entType == "Player") {

						createPlayerEntFromJson(item, path);
					}
					else if (entType == "Capsule") {

						createCapsuleEntFromJson(item, path);
					}
					else if (entType == "Actor") {

						createActorEntFromJson(item, path);
					}
					else if (entType == "Sensor") {

						createSensorEntFromJson(item, path);
					}
					else if (entType == "Grid") {

						createGridEntFromJson(item, path);
					}
					else if (entType == "StaticMesh") {

						createStaticMeshEntFromJson(item, path);
					}
					else if (entType == "Camera") {

						//PlayerCam is created with the player for now.
					}
					else {

						LogWarn(LOG_APP, "Entity type %s exists in Game File %s", entType.c_str(), path.c_str());
					}
				}
			}
		}

		gameLoaded = true;
		return true;
	}

	//TODO create entityFactory function
	bool createGameEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;

		if (!validateName(item, filename)) return false;
		name = item["name"].GetString();

		//TODO create Factory function
		flecs::entity game = ecs.entity(name.c_str())
			.add<Game>()
			.set<EntityType>({ EntityType::Game })
			.add<IsActive>().add(flecs::CanToggle);

		if(!EntityFactory::validateEntityCreation(game, name)) return false;

		return true;
	}

	//TODO create entityFactory function
	bool createSceneEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;

		if (!validateName(item, filename)) return false;
		name = item["name"].GetString();

		if (!validateParent(item, filename)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		//TODO create Factory function
		flecs::entity scene = ecs.entity(name.c_str())
			.add<_Scene>()
			.set<EntityType>({ EntityType::Scene })
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(parentEnt);

		if (!EntityFactory::validateEntityCreation(scene, name)) return false;

		return true;
	}

	bool createPlayerEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;

		if (!validateParent(item, filename)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");


		//Transforms does nothing yet
		Transform playerTransform;
		playerTransform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		playerTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		playerTransform.scale = glm::vec3(1.0f);

		if (!EntityFactory::createPlayerEntity(ecs, parentEnt, playerTransform, "pipelineUnlit")) {
			return false;
		}

		return true;
	}

	bool createCapsuleEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item, filename)) return false;
		name = item["name"].GetString();

		if (!validateTransform(item, filename)) return false;
		auto optTransform = deserTransform(item["components"]["Transform"]);

		if (!optTransform) return false;
		transform = optTransform.value();

		//validate modelSrc
		if (!validateModelSrc(item, filename)) return false;
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();

		if (!validateParent(item, filename)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		if (!EntityFactory::createCapsuleEntity(ecs, parentEnt, name, "capsule4", transform, "pipelineUnlit")) {
			return false;
		}

		return true;
	}

	bool createActorEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;
		Transform transform;

		//By the time we get here We know the item already has components

		if (!validateName(item, filename)) return false;
		name = item["name"].GetString();

		if (!validateTransform(item, filename)) return false;
		auto optTransform = deserTransform(item["components"]["Transform"]);

		if (!optTransform) return false;
		transform = optTransform.value();

		if (!validateParent(item, filename)) return false;
		parentName = item["parent"].GetString();
		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		if (!EntityFactory::createActorEntity(ecs, parentEnt, name, "enemy1", transform, settings, actor1Update,"pipelineUnlit")) {
			return false;
		}

		return true;
	}

	bool createSensorEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;
		Transform transform;

		if (!validateName(item, filename)) return false;
		if (!validateTransform(item, filename)) return false;
		if (!validateParent(item, filename)) return false;

		name = item["name"].GetString();
		parentName = item["parent"].GetString();

		std::optional<Transform> optTransform = deserTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		JPH::Vec3 boxSensorSize = JPH::Vec3(15.0f, 15.0f, 15.0f);
		if (!EntityFactory::createBoxSensorEntity(ecs, parentEnt, name, transform, boxSensorSize, sensor1Behavior)) {
			return false;
		}

		return true;
	}

	bool createGridEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item, filename)) return false;
		if (!validateTransform(item, filename)) return false;
		if (!validateParent(item, filename)) return false;
		if (!validateModelSrc(item, filename)) return false;

		//TODO entityFactory validate Name 

		name = item["name"].GetString();
		parentName = item["parent"].GetString();
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();


		std::optional<Transform> optTransform = deserTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		if (!EntityFactory::createGridEntity(ecs, parentEnt, name, transform, "pipelineGrid-Wireframe", 256)) {
			return false;
		}

		return true;
	}

	bool createStaticMeshEntFromJson(const rapidjson::Value& item, const std::string& filename) {

		std::string name;
		std::string parentName;
		std::string modelSrcName;
		Transform transform;

		if (!validateName(item, filename)) return false;
		if (!validateTransform(item, filename)) return false;
		if (!validateParent(item, filename)) return false;
		if (!validateModelSrc(item, filename)) return false;

		//TODO entityFactory validate Name 

		name = item["name"].GetString();
		parentName = item["parent"].GetString();
		modelSrcName = item["components"]["ModelSourceRef"]["name"].GetString();


		std::optional<Transform> optTransform = deserTransform(item["components"]["Transform"]);
		if (!optTransform) return false;
		transform = optTransform.value();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");
		if (!EntityFactory::createStaticMeshEntity(ecs, parentEnt, name, modelSrcName, transform, "pipelineSolid-Wireframe")) {
			return false;
		}

		return true;
	}


	bool validateParent(const rapidjson::Value& item, const std::string filename) const {

		if (!item.HasMember("parent") || !item["parent"].IsString()) {

			LogError(LOG_APP, "Json entity does not have parent in file %s" , filename.c_str());
			return false;
		}

		std::string parentName = item["parent"].GetString();

		flecs::entity parentEnt = ecs.lookup(parentName.c_str(), ".");

		if (!parentEnt.is_valid()) {

			LogError(LOG_APP, " parentEnt named %s read from json is invalid entity in file %s", parentName.c_str(), filename.c_str());
			return false;
		}

		return true;

	}

	// does not check if components is valid because we assume that has already been checked
	bool validateName(const rapidjson::Value& item, const std::string filename) {

		if (!item.HasMember("name") || !item["name"].IsString()) {

			LogError(LOG_APP, "this poor entity doesn't even have a name! in file %s", filename.c_str());
			return false;
		}
		return true;
	}

	
	bool validateModelSrc(const rapidjson::Value& item, const std::string filename) {

		if (!item["components"].HasMember("ModelSourceRef")) {

			LogError(LOG_APP, "components section does not have ModelSourceRef in file %s", filename.c_str());
			return false;
		}

		if (!item["components"]["ModelSourceRef"].HasMember("name")) {

			LogError(LOG_APP, "ERROR components does not have ModelSourceRef name in file %s", filename.c_str());
			return false;
		}

		if (!item["components"]["ModelSourceRef"]["name"].IsString()) {

			LogError(LOG_APP, "ERROR components does not have ModelSourceRef name string in file %s", filename.c_str());
			return false;
		}
		return true;
	}

	//validates and gets transform from json
	bool validateTransform(const rapidjson::Value& item, const std::string filename) {

		if (!item["components"].HasMember("Transform")) {

			LogError(LOG_APP, "ERROR components section does not have Transform in file %s", filename.c_str());
			return false;
		}
		return true;
	}


	std::optional<glm::vec3> deserializeGLMVec3(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		if (!obj.HasMember("x") || !obj.HasMember("y") || !obj.HasMember("z"))
			return std::nullopt;

		return glm::vec3(
			obj["x"].GetFloat(),
			obj["y"].GetFloat(),
			obj["z"].GetFloat()
		);
	}

	std::optional<glm::quat> deserializeGLMQuat(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		if (!obj.HasMember("w") || !obj.HasMember("x") ||
			!obj.HasMember("y") || !obj.HasMember("z"))
			return std::nullopt;

		// glm::quat constructor is (w, x, y, z)
		return glm::quat(
			obj["w"].GetFloat(),
			obj["x"].GetFloat(),
			obj["y"].GetFloat(),
			obj["z"].GetFloat()
		);
	}

	std::optional<Transform> deserTransform(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		Transform transform;

		// Deserialize position
		if (obj.HasMember("position")) {
			auto pos = deserializeGLMVec3(obj["position"]);
			if (pos) transform.position = *pos;
		}

		// Deserialize rotation
		if (obj.HasMember("rotation")) {
			auto rot = deserializeGLMQuat(obj["rotation"]);
			if (rot) transform.rotation = *rot;
		}

		// Deserialize scale
		if (obj.HasMember("scale")) {
			auto scl = deserializeGLMVec3(obj["scale"]);
			if (scl) transform.scale = *scl;
		}

		return transform;
	}
};
