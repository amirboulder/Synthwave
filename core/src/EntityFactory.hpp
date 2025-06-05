#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <float.h> 

#include "Entity.hpp"
#include "ecs/archetypes.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

typedef std::vector<Entity>  EntityVector;
typedef std::vector<Model> modelVector;
typedef	std::vector<TransformData> TransformVector;
typedef std::vector<PhysicsData> PhysicsVector;

class EntityFactory {

public:

	static uint32_t entityIDCounter;

	std::vector<Entity> & entities;

	modelVector& models;

	TransformVector& transforms;

	PhysicsVector & physicsComponents;



	EntityFactory(std::vector<Entity> & entities, PhysicsVector & physicsComponents,modelVector& models, TransformVector& transforms)
		: entities(entities), physicsComponents(physicsComponents), models(models), transforms(transforms)
	{

	}

	Entity createPlayerEntity(Player & player,const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {

		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);


		float meshX;
		float meshY;
		float meshZ;
		calculateMeshDimensions(models.back().meshes[0], meshX, meshY, meshZ);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);


		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * scale.y; // Scale height (y-axis)

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(position.x, position.y, position.z);
		JPH::Quat joltRotation(rotation.x, rotation.y, rotation.z, rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		player.CreatePlayer(fisiks.physics_system, joltPosition, joltRotation, physicsRadius, physicsHalfHeight);

		physicsComponents.emplace_back(player.JoltCharacter->GetBodyID());

		Entity temp(entityIDCounter, player.JoltCharacter->GetBodyID(), static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;


	}

	//TODO
	void createHumanoidEntity(const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {


		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);

		/*
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
		*/


	}

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

	void createCapsuleEntity(const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {

		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);

	
		float meshX;
		float meshY;
		float meshZ;

		calculateMeshDimensions(models.back().meshes[0], meshX, meshY, meshZ);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);


		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * scale.y; // Scale height (y-axis)

		JPH::CapsuleShape* capsuleShape = new JPH::CapsuleShape(physicsHalfHeight, physicsRadius);


		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(position.x, position.y , position.z );
		JPH::Quat joltRotation(rotation.x, rotation.y, rotation.z, rotation.w);
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
		pillSettings.mRestitution = 0.08f;



		// Create and add body
		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::Activate);

		physicsComponents.emplace_back(physicsID);

		entities.emplace_back(entityIDCounter,  physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;
	
	}


	Entity createStaticMeshEntity(const char* filePath, GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform) {

		transforms.emplace_back(transform);

		models.emplace_back(filePath, ShaderID, transforms.size() - 1);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);



		// Scale vertices
		VertexList scaledVertexList;
		for (const VertexData & vertexData : models.back().meshes[0].vertices) {
			glm::vec3 scaledVertex = vertexData.vertex * scale; // Apply scale
			scaledVertexList.push_back(Float3(scaledVertex.x, scaledVertex.y, scaledVertex.z));
		}

		// Create triangle list
		IndexedTriangleList triangleList;
		for (size_t i = 0; i < models.back().meshes[0].indices.size(); i += 3) {
			triangleList.push_back(IndexedTriangle(
				models.back().meshes[0].indices[i],
				models.back().meshes[0].indices[i + 1],
				models.back().meshes[0].indices[i + 2]
			));
		}

		// Create MeshShapeSettings
		MeshShapeSettings meshSettings(scaledVertexList, triangleList);

		// Create MeshShape
		Ref<Shape> meshShape = meshSettings.Create().Get();

		// Convert GLM to Jolt types
		Vec3 joltPosition(position.x, position.y, position.z);
		Quat joltRotation(rotation.x, rotation.y, rotation.z, rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

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

		physicsComponents.emplace_back(physicsID);

		Entity temp(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;

	}

	
	Entity createGridEntity( GLuint ShaderID, Fisiks& fisiks, glm::mat4 transform,int rows , int cols) {

		transforms.emplace_back(transform);
		
		models.emplace_back(ShaderID, transforms.size() - 1,rows, cols);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);

		// Calculate box dimensions based on grid size and scale
		// 0.01 for y breaks it
		Vec3 boxHalfExtents(rows * 0.5,0.1, cols *  0.5);  

		
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
			EMotionType::Static, 
			Layers::NON_MOVING   
		);

		boxBodySettings.mRestitution = 0.0f; // High restitution for bounciness
		boxBodySettings.mFriction = 1.0f;    // Low friction for sliding

		
		BodyID physicsID = fisiks.bodyInterface.CreateAndAddBody(
			boxBodySettings,
			EActivation::Activate
		);



		physicsComponents.emplace_back(physicsID);

		Entity temp(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;

	}


	void calculateMeshDimensions(const Mesh& mesh, float& x, float& y,float & z) {
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

			minX = std::min(minX, current.vertex.x);
			maxX = std::max(maxX, current.vertex.x);

			minY = std::min(minY, current.vertex.y);
			maxY = std::max(maxY, current.vertex.y);

			minZ = std::min(minZ, current.vertex.z);
			maxZ = std::max(maxZ, current.vertex.z);
		}

		x = maxX - minX;
		y = maxY - minY; 
		z = maxZ - minZ;
	}


};

uint32 EntityFactory::entityIDCounter = 0;
