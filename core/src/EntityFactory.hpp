#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <float.h> 

#include "character/actor.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>


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


	static void createCapsuleEntity(flecs::world& ecs,Fisiks& fisiks,std::string name ,ModelSource& modelSource, Transform transform) {

		// Flip Y for Vulkan
		transform.position.y *= -1;

		float meshX;
		float meshY;
		float meshZ;

		calculateMeshDimensions(modelSource.meshes[0], meshX, meshY, meshZ);

		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * transform.scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * transform.scale.y; // Scale height (y-axis)

		JPH::CapsuleShape* capsuleShape = new JPH::CapsuleShape(physicsHalfHeight, physicsRadius);


		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create body settings
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
		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::Activate);


		auto e1 = ecs.entity(name.c_str())
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID)
			;

	}

	static void createCharacterEntity(flecs::world& ecs, Fisiks& fisiks, std::string name,ModelSource& modelSource, Transform transform, Actor & actor) {

		// Flip Y for Vulkan
		transform.position.y *= -1;

		float meshX;
		float meshY;
		float meshZ;

		calculateMeshDimensions(modelSource.meshes[0], meshX, meshY, meshZ);

		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * transform.scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * transform.scale.y; // Scale height (y-axis)

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		actor.createPhysicsBody(fisiks.physics_system);

		BodyID physicsID = actor.joltCharacter->GetBodyID();

		auto e1 = ecs.entity(name.c_str())
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID)
			;

	}
	

	// A Renderable is just a model and a transform no physics body
	static void createRenderableEntity(flecs::world& ecs, std::string name,ModelSource& modelSource, Transform transform, const char* pipelineName = NULL) {

		// Flip Y for Vulkan
		transform.position.y *= -1;

		auto entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance());
		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

	}

	static void createStaticMeshEntity(flecs::world& ecs, Fisiks& fisiks, std::string name, ModelSource& modelSource,Transform transform, const char* pipelineName = NULL) {

		float scaleFactor = 1.0f;

		// Flip Y for Vulkan
		transform.position.y *= -1;

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

		auto entity = ecs.entity(name.c_str())
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID);

		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

	}
	
	static void createGridEntity(flecs::world& ecs, Fisiks& fisiks, std::string name, ModelSource& modelSource,Transform  transform,const char * pipelineName, int rows, int cols) {

		// Flip Y for Vulkan 
		transform.position.y *= -1;

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


		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(
			boxBodySettings,
			EActivation::Activate
		);

		//needed to visually align the grid render with the physics body #MAGICNUMBER\
		//TODO find out why the grid render slightly below its physics body by default
		transform.position.y -= boxThickness + 0.5 ;

		auto entity = ecs.entity(name.c_str())
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance())
			.set<JPH::BodyID>(physicsID);

		if (pipelineName) {
			entity.add<CustomPipeline>(ecs.lookup(pipelineName));

		}

	}

	//TODO move 
	static void calculateMeshDimensions(const MeshSource& mesh, float& x, float& y,float & z) {
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





