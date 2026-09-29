module;

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <flecs.h>

//rapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"


// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif


export module Serialization;

import Logger;
import GLM;
import Jolt;
import Components;
import GraphicsComponents;
import PhysicsComponents;
import PlayerComponents;

//These should go away eventually
import EntityFactory; //We should be using entity Creator
import ActorBehaviors; // we should not have to refrence game functions directly here
import SensorBehaviors; // we should not have to refrence game functions directly here

import ReflectionDataECS;

namespace fs = std::filesystem;

// Does both serialization and deserialization
export class Serializer {

    flecs::world& ecs;
    flecs::query<> activeGameQuery;

	inline static ecs_entity_t s_dontSerializeId = 0;

public:

	bool gameLoaded = false;

    Serializer(flecs::world& ecs)
        : ecs(ecs)
    {

		registerReflectionData(ecs);
        registerQuery();

		s_dontSerializeId = ecs.component<DontSerialize>().id();


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
            .with<Game>().term_at(0).self().cascade(flecs::ChildOf)
			.without<DontSerialize>().self().up(flecs::ChildOf) //Filter Entites that we don't want to serialize
            .build();

		//Use this filter to filter out components with DontSerialize Trait
		flecs::entity_to_json_desc_t jsonDesc = ECS_ENTITY_TO_JSON_INIT;
		jsonDesc.component_filter = serializeComponentFilter;


		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

        activeGameQuery.each([&](flecs::entity e) {

            if (!e.is_alive()) return;

            flecs::string entityJson = e.to_json(&jsonDesc);

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

	// Called by flecs for each component on the entity, return false to leave it out of the json
	static bool serializeComponentFilter(const ecs_world_t* world, ecs_entity_t id) {

		// For data pairs like (HasContactScript, ContactFunction) the tag goes on the relationship
		const ecs_entity_t target = ECS_IS_PAIR(id) ? ecs_pair_first(world, id) : id;
		return !ecs_has_id(world, target, s_dontSerializeId);
	}

	bool loadGameFromJson(const std::string& path) {

		std::ifstream file(path);
		if (!file.is_open()) {
			LogError(LOG_APP, "Failed to open Game file %s", path.c_str());
			return false;
		}

		// Read the whole file and Parse() rather than ParseStream(IStreamWrapper).
		// IStreamWrapper instantiates std::istream::read here, and MSVC compiles
		// that body at end of TU -- after the `import`s in Synthwave.h -- where it
		// has lost the definition of basic_istream<char>::sentry (C2079 at
		// istream(535)). Same pattern as Manifest::Load.
		std::ostringstream ss;
		ss << file.rdbuf();
		const std::string raw = ss.str();

		rapidjson::Document doc;
		doc.Parse(raw.c_str());

		if (doc.HasParseError()) {
			LogError(LOG_APP, "JSON parse error %d at offset %zu in file %s", doc.GetParseError(), doc.GetErrorOffset(), path.c_str());
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

			// HasMember/FindMember assert on non-objects, so skip these entirely (e.g. null entries from a failed to_json)
			if (!item.IsObject()) {
				LogError(LOG_APP, "Item in json is not an object in file %s, skipping", path.c_str());
				continue;
			}

			const rapidjson::Value* components = findMember(item, "components");
			if (!components || !components->IsObject()) continue;

			const rapidjson::Value* entTypeVal = findMember(*components, "EntityType");
			if (!entTypeVal || !entTypeVal->IsString()) continue;

			const std::string entType = entTypeVal->GetString();

			if (entType == "Game") {

				createGameEntFromJson(item, *components, path);
			}
			else if (entType == "Scene") {

				createSceneEntFromJson(item, *components, path);
			}
			else if (entType == "Player") {

				createPlayerEntFromJson(item, *components, path);
			}
			else if (entType == "Capsule") {

				createCapsuleEntFromJson(item, *components, path);
			}
			else if (entType == "Cube") {

				createCubeEntFromJson(item, *components, path);
			}
			else if (entType == "Sphere") {

				createSphereEntFromJson(item, *components, path);
			}
			else if (entType == "Cylinder") {

				createCylinderEntFromJson(item, *components, path);
			}
			else if (entType == "Actor") {

				createActorEntFromJson(item, *components, path);
			}
			else if (entType == "Sensor") {

				createSensorEntFromJson(item, *components, path);
			}
			else if (entType == "Grid") {

				createGridEntFromJson(item, *components, path);
			}
			else if (entType == "StaticMesh") {

				createStaticMeshEntFromJson(item, *components, path);
			}
			else if (entType == "Mountain") {

				createMountainEntFromJson(item, *components, path);
			}
			else if (entType == "StaticEnt") {

				createStaticMountainEntFromJson(item, *components, path);
			}
			else if (entType == "Camera") {

				//PlayerCam is created with the player for now.
			}
			else {

				LogWarn(LOG_APP, "Entity type %s exists in Game File %s", entType.c_str(), path.c_str());
			}
		}

		gameLoaded = true;
		return true;
	}

	//TODO create entityFactory function
	bool createGameEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		//TODO create Factory function
		flecs::entity game = ecs.entity(name->c_str())
			.add<Game>()
			.set<EntityType>({ EntityType::Game })
			.add<IsActive>().add(flecs::CanToggle);

		if(!EntityFactory::validateEntityCreation(game, *name)) return false;

		return true;
	}

	//TODO create entityFactory function
	bool createSceneEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		//TODO create Factory function
		flecs::entity scene = ecs.entity(name->c_str())
			.add<_Scene>()
			.set<EntityType>({ EntityType::Scene })
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(*parentEnt);

		if (!EntityFactory::validateEntityCreation(scene, *name)) return false;

		return true;
	}

	bool createPlayerEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;


		if (!EntityFactory::createPlayerEntity(ecs, *parentEnt, *transform, "pipelineUnlit")) {
			return false;
		}

		return true;
	}

	bool createCapsuleEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name;
		std::optional<Transform> transform;
		std::optional<flecs::entity> parentEnt;
		std::optional<RigidBodyDesc> desc;

		if(!deserCommon(item, components, filename, name, transform, parentEnt, desc))
			return false;

		if (!EntityFactory::createCapsuleEntity(ecs, *parentEnt, *name, *transform, *desc)) {
			return false;
		}

		return true;
	}

	bool createSphereEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name;
		std::optional<Transform> transform;
		std::optional<flecs::entity> parentEnt;
		std::optional<RigidBodyDesc> desc;

		if (!deserCommon(item, components, filename, name, transform, parentEnt, desc))
			return false;

		if (!EntityFactory::createSphereEntity(ecs, *parentEnt, *name, *transform, *desc)) {
			return false;
		}

		return true;
	}

	bool createCubeEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name;
		std::optional<Transform> transform;
		std::optional<flecs::entity> parentEnt;
		std::optional<RigidBodyDesc> desc;

		if (!deserCommon(item, components, filename, name, transform, parentEnt, desc))
			return false;

		if (!EntityFactory::createCubeEntity(ecs, *parentEnt, *name, *transform, *desc)) {
			return false;
		}

		return true;
	}

	bool createCylinderEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name;
		std::optional<Transform> transform;
		std::optional<flecs::entity> parentEnt;
		std::optional<RigidBodyDesc> desc;

		if (!deserCommon(item, components, filename, name, transform, parentEnt, desc))
			return false;

		if (!EntityFactory::createCylinderEntity(ecs, *parentEnt, *name, *transform, *desc)) {
			return false;
		}

		return true;
	}

	bool createActorEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new JPH::CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = JPH::DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		if (!EntityFactory::createActorEntity(ecs, *parentEnt, *name, *transform, settings, Scripts::enemyUpdate)) {
			return false;
		}

		return true;
	}

	bool createSensorEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		JPH::Vec3 boxSensorSize = JPH::Vec3(15.0f, 15.0f, 15.0f);
		if (!EntityFactory::createBoxSensorEntity(ecs, *parentEnt, *name, *transform, boxSensorSize, SensorScripts::LogOnContact)) {
			return false;
		}

		return true;
	}

	bool createGridEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		if (!EntityFactory::createGridEntity(ecs, *parentEnt, *name, *transform, 256)) {
			return false;
		}

		return true;
	}

	bool createMountainEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		if (!EntityFactory::createMTNEntity(ecs, *parentEnt, *name, *transform)) {
			return false;
		}

		return true;
	}

	bool createStaticMeshEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename,
		EntityType entityType = EntityType::StaticMesh) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		if (!EntityFactory::createStaticMeshEntity(ecs, *parentEnt, *name, *transform, entityType, 2337188011122585679)) {
			return false;
		}

		return true;
	}

	bool createStaticMountainEntFromJson(const rapidjson::Value& item, const rapidjson::Value& components, const std::string& filename) {

		std::optional<std::string> name = deserName(item, filename);
		if (!name) return false;

		std::optional<Transform> transform = deserTransform(components, filename);
		if (!transform) return false;

		std::optional<flecs::entity> parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		if (!EntityFactory::createMTNEntity(ecs, *parentEnt, *name, *transform)) {
			return false;
		}

		return true;
	}

	bool deserCommon(const rapidjson::Value& item,
		const rapidjson::Value& components,
		const std::string& filename,
		std::optional<std::string>& name,
		std::optional<Transform>& transform,
		std::optional<flecs::entity>& parentEnt,
		std::optional<RigidBodyDesc>& desc) {

		name = deserName(item, filename);
		if (!name) return false;

		transform = deserTransform(components, filename);
		if (!transform) return false;

		parentEnt = deserParent(item, filename);
		if (!parentEnt) return false;

		desc = deserRigidBodyDesc(components, filename);
		if (!desc) return false;

		return true;
	}


	// Validates and deserializes the entity name
	std::optional<std::string> deserName(const rapidjson::Value& item, const std::string& filename) const {

		const rapidjson::Value* nameVal = findMember(item, "name");
		if (!nameVal || !nameVal->IsString()) {
			LogError(LOG_APP, "this poor entity doesn't even have a name! in file %s", filename.c_str());
			return std::nullopt;
		}

		return std::string(nameVal->GetString());
	}

	// Validates the parent path and looks up the parent entity, which must already exist
	std::optional<flecs::entity> deserParent(const rapidjson::Value& item, const std::string& filename) const {

		const rapidjson::Value* parentVal = findMember(item, "parent");
		if (!parentVal || !parentVal->IsString()) {
			LogError(LOG_APP, "Json entity does not have parent in file %s", filename.c_str());
			return std::nullopt;
		}

		const char* parentName = parentVal->GetString();
		flecs::entity parentEnt = ecs.lookup(parentName, ".");

		if (!parentEnt.is_valid()) {
			LogError(LOG_APP, "parentEnt named %s read from json is invalid entity in file %s", parentName, filename.c_str());
			return std::nullopt;
		}

		return parentEnt;
	}


	std::optional<glm::vec3> deserializeGLMVec3(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		const rapidjson::Value* x = findMember(obj, "x");
		const rapidjson::Value* y = findMember(obj, "y");
		const rapidjson::Value* z = findMember(obj, "z");
		if (!x || !y || !z) return std::nullopt;

		// GetFloat() asserts on non-numbers
		std::optional<float> fx = readFloat(*x), fy = readFloat(*y), fz = readFloat(*z);
		if (!fx || !fy || !fz) return std::nullopt;

		return glm::vec3(*fx, *fy, *fz);
	}

	std::optional<JPH::Vec3> deserializeJPHVec3(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		const rapidjson::Value* x = findMember(obj, "x");
		const rapidjson::Value* y = findMember(obj, "y");
		const rapidjson::Value* z = findMember(obj, "z");
		if (!x || !y || !z) return std::nullopt;

		// GetFloat() asserts on non-numbers
		std::optional<float> fx = readFloat(*x), fy = readFloat(*y), fz = readFloat(*z);
		if (!fx || !fy || !fz) return std::nullopt;

		return JPH::Vec3(*fx, *fy, *fz);
	}

	std::optional<glm::quat> deserializeGLMQuat(const rapidjson::Value& obj) {
		if (!obj.IsObject()) return std::nullopt;

		const rapidjson::Value* w = findMember(obj, "w");
		const rapidjson::Value* x = findMember(obj, "x");
		const rapidjson::Value* y = findMember(obj, "y");
		const rapidjson::Value* z = findMember(obj, "z");
		if (!w || !x || !y || !z) return std::nullopt;

		// GetFloat() asserts on non-numbers
		std::optional<float> fw = readFloat(*w), fx = readFloat(*x), fy = readFloat(*y), fz = readFloat(*z);
		if (!fw || !fx || !fy || !fz) return std::nullopt;

		// glm::quat constructor is (w, x, y, z)
		return glm::quat(*fw, *fx, *fy, *fz);
	}

	// Validates and deserializes in one pass. The Transform block is required,
	// missing fields keep their defaults, invalid fields are logged and fall back to defaults.
	std::optional<Transform> deserTransform(const rapidjson::Value& components, const std::string& filename) {

		const rapidjson::Value* obj = findMember(components, "Transform");
		if (!obj || !obj->IsObject()) {
			LogError(LOG_APP, "components section does not have a valid Transform in file %s", filename.c_str());
			return std::nullopt;
		}

		Transform transform;

		auto warnInvalid = [&](const char* name) {
			LogWarn(LOG_APP, "Transform %s is invalid in file %s, using default", name, filename.c_str());
		};

		if (const rapidjson::Value* v = findMember(*obj, "position")) {
			std::optional<glm::vec3> pos = deserializeGLMVec3(*v);
			if (pos && glm::all(glm::isfinite(*pos))) transform.position = *pos;
			else warnInvalid("position");
		}

		if (const rapidjson::Value* v = findMember(*obj, "rotation")) {
			std::optional<glm::quat> rot = deserializeGLMQuat(*v);
			float len = rot ? glm::length(*rot) : 0.0f;
			// Zero or non-finite quaternions can't be normalized
			if (rot && std::isfinite(len) && len > 0.0f) transform.rotation = *rot / len;
			else warnInvalid("rotation");
		}

		if (const rapidjson::Value* v = findMember(*obj, "scale")) {
			std::optional<glm::vec3> scl = deserializeGLMVec3(*v);
			if (scl && glm::all(glm::isfinite(*scl))) transform.scale = *scl;
			else warnInvalid("scale");
		}

		return transform;
	}

	// Returns nullptr instead of asserting like operator[] does when the member is missing
	static const rapidjson::Value* findMember(const rapidjson::Value& obj, const char* name) {
		if (!obj.IsObject()) return nullptr;
		auto it = obj.FindMember(name);
		return it != obj.MemberEnd() ? &it->value : nullptr;
	}

	// flecs writes some floats as strings (e.g. "-.11e20"), accept both
	static std::optional<float> readFloat(const rapidjson::Value& v) {
		if (v.IsNumber()) return v.GetFloat();
		if (v.IsString()) {
			const char* str = v.GetString();
			char* end = nullptr;
			float f = std::strtof(str, &end);
			if (end != str) return f;
		}
		return std::nullopt;
	}

	// Validates and deserializes in one pass. Missing fields keep their defaults so older save files still load,
	// fields that are present but invalid are logged and also fall back to defaults.
	// Only fails if the block exists but isn't an object.
	std::optional<RigidBodyDesc> deserRigidBodyDesc(const rapidjson::Value& components, const std::string& filename) {

		RigidBodyDesc desc;
		const RigidBodyDesc defaults;

		const rapidjson::Value* obj = findMember(components, "JPH.BodyID");
		if (!obj) {
			LogWarn(LOG_APP, "No JPH.BodyID in file %s, using default RigidBodyDesc", filename.c_str());
			return desc;
		}
		if (!obj->IsObject()) {
			LogWarn(LOG_APP, "JPH.BodyID is not an object in file %s using default", filename.c_str());
		}

		auto warnInvalid = [&](const char* name) {
			LogWarn(LOG_APP, "RigidBodyDesc %s is invalid in file %s, using default", name, filename.c_str());
		};

		auto readF = [&](const char* name, float& out, float min) {
			const rapidjson::Value* v = findMember(*obj, name);
			if (!v) return;
			std::optional<float> f = readFloat(*v);
			if (f && std::isfinite(*f) && *f >= min) out = *f;
			else warnInvalid(name);
		};

		auto readVec3 = [&](const char* name, glm::vec3& out) {
			const rapidjson::Value* v = findMember(*obj, name);
			if (!v) return;
			std::optional<glm::vec3> vec = deserializeGLMVec3(*v);
			if (vec && glm::all(glm::isfinite(*vec))) out = *vec;
			else warnInvalid(name);
		};

		// Enums and layer are stored as unsigned ints, max is inclusive
		auto readUint = [&](const char* name, uint32_t max, auto& out) {
			using T = std::remove_reference_t<decltype(out)>;
			const rapidjson::Value* v = findMember(*obj, name);
			if (!v) return;
			if (v->IsUint() && v->GetUint() <= max) out = static_cast<T>(v->GetUint());
			else warnInvalid(name);
		};

		auto readBool = [&](const char* name, bool& out) {
			const rapidjson::Value* v = findMember(*obj, name);
			if (!v) return;
			if (v->IsBool()) out = v->GetBool();
			else warnInvalid(name);
		};

		readF("mass",           desc.mass,           0.001f);
		readF("friction",       desc.friction,       0.0f);
		readF("restitution",    desc.restitution,    0.0f);
		readF("linearDamping",  desc.linearDamping,  0.0f);
		readF("angularDamping", desc.angularDamping, 0.0f);
		readF("gravityFactor",  desc.gravityFactor,  -FLT_MAX);

		readVec3("linearVelocity",  desc.linearVelocity);
		readVec3("angularVelocity", desc.angularVelocity);

		readUint("motionQuality", static_cast<uint32_t>(JPH::EMotionQuality::LinearCast), desc.motionQuality);
		readUint("allowedDOFs",   static_cast<uint32_t>(JPH::EAllowedDOFs::All),          desc.allowedDOFs);
		readUint("motionType",    static_cast<uint32_t>(JPH::EMotionType::Dynamic),       desc.motionType);
		readUint("layer",         Layers::NUM_LAYERS - 1,                                 desc.layer);

		readBool("activation",    desc.activation);
		readBool("allowSleeping", desc.allowSleeping);

		// Jolt asserts if a body has no degrees of freedom
		if (desc.allowedDOFs == JPH::EAllowedDOFs::None) {
			warnInvalid("allowedDOFs");
			desc.allowedDOFs = defaults.allowedDOFs;
		}

		return desc;
	}

	
};
