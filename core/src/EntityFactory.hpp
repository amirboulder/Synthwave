#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <float.h> 

#include <flecs.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// TODO maybe add flecs::world& ecs to this class and make the functions not static to reduce one parameter 
class EntityFactory {

public:


	EntityFactory() {};


	//TODO
	/*
	void createHumanoidEntity(const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {


		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);


		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);

		StaticCompoundShapeSettings shapeSettings;



		Ref<Shape> humanoidShape = new StaticCompoundShape();

		BodyCreationSettings boxBodySettings(
			humanoidShape,
			joltPosition,
			joltRotation,
			EMotionType::Dynamic,
			Layers::MOVING
		);



	}
	*/


	/*
	void createBoxEntity(const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {

		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);

		float meshXsize;
		float meshYsize;
		float meshZsize;

		calculateMeshDimensions(models.back().meshes[0], meshXsize, meshYsize, meshZsize);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);

		// Calculate box dimensions based on grid size and scale

		Vec3 boxHalfExtents(meshXsize * 0.5, meshYsize * 0.5, meshZsize * 0.5);


		// Create BoxShape
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// Convert GLM to Jolt types
		Vec3 joltPosition(position.x, position.y, position.z);
		Quat joltRotation(rotation.x, rotation.y, rotation.z, rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create BodyCreationSettings
		BodyCreationSettings boxBodySettings(
			boxShape,
			joltPosition,
			joltRotation,
			EMotionType::Dynamic,
			Layers::MOVING
		);

		boxBodySettings.mRestitution = 0.1f; // High restitution = more bounciness
		boxBodySettings.mFriction = 1.0f;

		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(
			boxBodySettings,
			EActivation::Activate );

		physicsComponents.emplace_back(physicsID);

		entities.emplace_back(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

	}
	*/

	// create a static box shaped sensor 
	static bool createBoxSensorEntity(flecs::world& ecs, Fisiks& fisiks, std::string name,
		Transform transform, JPH::Vec3Arg size,
		std::function<void(flecs::world& ecs,flecs::entity self, flecs::entity other)> onContactAdded) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name)) return false;
		if (!EntityFactory::validateSize(size, name,/*isDynamic=*/false)) return false;


		Vec3 boxHalfExtents(size.GetX() * 0.5, size.GetY() * 0.5, size.GetZ() * 0.5);

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::BodyCreationSettings sensorSetting(
			boxShape,
			joltPosition,
			joltRotation,
			JPH::EMotionType::Static,
			Layers::NON_MOVING
		);

		//Make it a sensor!
		sensorSetting.mIsSensor = true;

		// Create and add body
		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(sensorSetting, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.add<StaticEnt>()
			.add<Sensor>()
			.set<Transform>(transform)
			.set<JPH::BodyID>(physicsID)
			.emplace<SensorBehavior>(onContactAdded);

		if (!validateEntityCreation(entity, name)) return false;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		fisiks.bodyInterface.SetUserData(physicsID, entity.id());

		return true;
	}

	//Creates a capsule shaped entity
	static bool createCapsuleEntity(flecs::world& ecs, Fisiks& fisiks, std::string name, ModelSource& modelSource, Transform transform) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		
		float meshX;
		float meshY;
		float meshZ;

		// Asumming modelSource has only 1 mesh
		calculateMeshSize(modelSource.meshes[0], meshX, meshY, meshZ);

		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * transform.scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * transform.scale.y; // Scale height (y-axis)

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> capsuleShape = new JPH::CapsuleShape(physicsHalfHeight, physicsRadius);


		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}


		JPH::BodyCreationSettings pillSettings(
			capsuleShape,
			joltPosition,
			joltRotation,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		// bounciness
		pillSettings.mRestitution = 0.5f;

		pillSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		pillSettings.mMassPropertiesOverride.mMass = 5000.1f;


		// Create and add body
		const BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID)
			;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		fisiks.bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity,name))  return false;

		return true;
	}

	static bool createActorEntity(flecs::world& ecs, Fisiks& fisiks, std::string name,
		ModelSource& modelSource,Transform transform, JPH::CharacterSettings settings, 
		std::function<void(flecs::world& ecs, flecs::entity self)> actorUpdate) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;


		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		flecs::entity actorEnt = ecs.entity("Actor")
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.add<JoltCharacter>()
			.add<JPH::BodyID>()
			.emplace<ActorBehavior>(actorUpdate);
			;

		if (!validateEntityCreation(actorEnt, name)) return false;

		JPH::Character* & joltCharacter = actorEnt.get_mut<JoltCharacter>().characterPtr;

		joltCharacter = new JPH::Character(&settings, joltPosition, joltRotation, actorEnt.id(), &fisiks.physics_system);

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(joltCharacter->GetBodyID(), name)) return false;

		actorEnt.get_mut<JPH::BodyID>() = joltCharacter->GetBodyID();

		
		return true;
	}


	// A Renderable is just a model and a transform no physics body
	static bool createRenderableEntity(flecs::world& ecs, std::string name, ModelSource& modelSource, Transform transform, const char* pipelineName = NULL) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance());
		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	static bool createStaticMeshEntity(flecs::world& ecs, Fisiks& fisiks, std::string name, ModelSource& modelSource, Transform transform, const char* pipelineName = NULL) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		float scaleFactor = 1.0f;


		// Scale vertices
		//ASSUMING the model only has one mesh
		VertexList scaledVertexList;
		for (const Vertex& vertexData : modelSource.meshes[0].vertices) {
			glm::vec3 scaledVertex = vertexData.position * scaleFactor; // Apply scale
			scaledVertexList.push_back(Float3(scaledVertex.x, scaledVertex.y, scaledVertex.z));
		}


		// Create triangle list
		//ASSUMING the model only has one mesh
		IndexedTriangleList triangleList;
		for (size_t i = 0; i < modelSource.meshes[0].indices.size(); i += 3) {
			triangleList.push_back(IndexedTriangle(
				modelSource.meshes[0].indices[i],
				modelSource.meshes[0].indices[i + 1],
				modelSource.meshes[0].indices[i + 2]
			));
		}

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create MeshShapeSettings
		MeshShapeSettings meshSettings(scaledVertexList, triangleList);

		// Create MeshShape
		Ref<Shape> meshShape = meshSettings.Create().Get();

		// Create BodyCreationSettings
		BodyCreationSettings meshBodySettings(
			meshShape,
			joltPosition,
			joltRotation,
			EMotionType::Static,
			Layers::NON_MOVING
		);

		// Create and add body
		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(
			meshBodySettings,
			EActivation::DontActivate
		);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID);

		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		fisiks.bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	static bool createGridEntity(flecs::world& ecs, Fisiks& fisiks, std::string name, ModelSource& modelSource, Transform  transform, const char* pipelineName, int rows, int cols) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;


		float boxThickness = 1;

		// any tickenss less than 0.01 will break jolt!
		Vec3 boxHalfExtents(rows * 0.5, boxThickness * 0.5, cols * 0.5);


		// Create BoxShape
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// Convert GLM to Jolt types
		Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create BodyCreationSettings
		BodyCreationSettings boxBodySettings(
			boxShape,
			joltPosition,
			joltRotation,
			EMotionType::Static,
			Layers::NON_MOVING
		);

		boxBodySettings.mRestitution = 0.1f; // High restitution for bounciness
		boxBodySettings.mFriction = 1.0f;    // Low friction for sliding


		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(boxBodySettings,EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		//needed to visually align the grid render with the physics body #MAGICNUMBER
		//TODO find out why the grid render slightly below its physics body by default
		transform.position.y += boxThickness + 0.5;

		const flecs::entity entity = ecs.entity(name.c_str())
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID);

		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		fisiks.bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	static bool createHUDElementEntity(flecs::world& ecs, std::string name,std::function<void(flecs::world& ecs)> drawFunction) {

		flecs::entity actorEnt = ecs.entity("HUD")
			.emplace<HudRender>(drawFunction);

		if (!validateEntityCreation(actorEnt, name)) return false;


		return true;

	}


	static bool validateName(std::string name) {
		if (name.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error name cannot be empty");
			return false;
		}
		if (name.length() > 256) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Entity name %s  ' exceeds 256 characters ", name.c_str());
			return false;
		}
		return true;
	}

	static bool validateTransform(Transform transform, std::string name) {
		if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) || !std::isfinite(transform.position.z)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid position (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.rotation.x) || !std::isfinite(transform.rotation.y) ||
			!std::isfinite(transform.rotation.z) || !std::isfinite(transform.rotation.w)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid rotation (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
			!std::isfinite(transform.scale.z)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid scale (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		return true;
	}


	// Jolt documentation says dynamic objects should be in the order [0.1, 10]
	// Static objects should be in the order [0.1, 2000] meters long
	static bool validateSize(JPH::Vec3Arg size, const std::string& name, bool dynamicObject)
	{
		const JPH::Vec3 minSizeConstraint(0.1f, 0.1f, 0.1f);
		const JPH::Vec3 maxSizeConstraint = dynamicObject
			? JPH::Vec3(10.0f, 10.0f, 10.0f)
			: JPH::Vec3(2000.0f, 2000.0f, 2000.0f);

		// Check for NaN / Inf first
		if (!std::isfinite(size.GetX()) || !std::isfinite(size.GetY()) || !std::isfinite(size.GetZ())) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error: Entity %s size contains NaN or Inf", name.c_str());
			return false;
		}

		// Minimum constraint check
		if (size.GetX() < minSizeConstraint.GetX() || size.GetY() < minSizeConstraint.GetY() || size.GetZ() < minSizeConstraint.GetZ()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR,
				"Error: Size components for entity %s must be >= 0.1 m (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		// Maximum constraint check
		if (size.GetX() > maxSizeConstraint.GetX() || size.GetY() > maxSizeConstraint.GetY() || size.GetZ() > maxSizeConstraint.GetZ()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"Warning: Entity %s size exceeds recommended bounds for %s objects (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), dynamicObject ? "dynamic" : "static", size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		return true;
	}

	static bool validatePhysicsBodyCreation(JPH::BodyID id, std::string name) {

		if (id.IsInvalid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating physics body for Entity : %s", name.c_str());
			return false;
		}
		return true;
	}

	static bool validateEntityCreation(flecs::entity entity , std::string name) {
		if (!entity.is_valid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating Entity : %s", name.c_str());
			return false;
		}
		return true;
	}

	//TODO move this function
	static void calculateMeshSize(const MeshSource& mesh, float& x, float& y,float & z) {
		if (mesh.vertices.empty()) {
			//width = 0.0f;
			//height = 0.0f;
			cout << "MESH DIMENSIONS ARE ZERO !!!\n";
			return;
		}

		float minX = FLT_MAX, maxX = -FLT_MAX;
		float minY = FLT_MAX, maxY = -FLT_MAX;
		float minZ = FLT_MAX, maxZ = -FLT_MAX;

		for (const auto& current : mesh.vertices) {

			minX = std::min(minX, current.position.x);
			maxX = std::max(maxX, current.position.x);

			minY = std::min(minY, current.position.y);
			maxY = std::max(maxY, current.position.y);

			minZ = std::min(minZ, current.position.z);
			maxZ = std::max(maxZ, current.position.z);
		}

		x = maxX - minX;
		y = maxY - minY; 
		z = maxZ - minZ;
	}
	

};





