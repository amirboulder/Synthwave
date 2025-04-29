#pragma once

#include "Entity.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

typedef std::vector<std::unique_ptr<Entity>>  EntityVector;
typedef std::vector<Model> modelVector;
typedef	std::vector<TransformData> TransformVector;
typedef std::vector<PhysicsData> PhysicsVector;

class EntityFactory {

public:

	static uint32 entityIDCounter;

	EntityVector& entities;

	modelVector& models;

	TransformVector& transforms;

	PhysicsVector & physicsComponents;



	EntityFactory(EntityVector& entities, PhysicsVector & physicsComponents,modelVector& models, TransformVector& transforms)
		: entities(entities), physicsComponents(physicsComponents), models(models), transforms(transforms)
	{

	}

	//TODO
	void createSphereEntity() {

	}

	Entity createCapsuleEntity(const char* filePath, GLuint ShaderID, Fisiks& physick, glm::mat4 transform) {

		transforms.emplace_back(transform);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);

		//TODO make these parameters
		// Compute capsule dimensions
		float modelRadius = 0.5f; // Unscaled model radius
		float modelHeight = 2.0f; // Unscaled model total height
		float physicsRadius = modelRadius * scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * scale.y; // Scale height (y-axis)

		JPH::CapsuleShape* capsuleShape = new JPH::CapsuleShape(physicsHalfHeight, physicsRadius);

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(position.x, position.y, position.z);
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


		models.emplace_back(filePath, ShaderID, transforms.size() - 1);

		// Create and add body
		BodyID physicsID = physick.bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::Activate);

		physicsComponents.emplace_back(physicsID);

		physick.bodyInterface.SetLinearVelocity(physicsID, Vec3(10.0f, 10.0f, 0.0f));

		Entity temp(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;
	}


	Entity createStaticMeshEntity(const char* filePath, GLuint ShaderID, Fisiks& physick, glm::mat4 transform) {

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
		for (const VertexData vertex : models.back().meshes[0].vertices) {
			glm::vec3 scaledVertex = vertex.vertices * scale; // Apply scale
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
		BodyID physicsID = physick.bodyInterface.CreateAndAddBody(
			meshBodySettings,
			EActivation::DontActivate
		);

		physicsComponents.emplace_back(physicsID);

		Entity temp(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;

	}

	
	Entity createGridEntity( GLuint ShaderID, Fisiks& physick, glm::mat4 transform,int rows , int cols) {

		transforms.emplace_back(transform);
		
		models.emplace_back(ShaderID, transforms.size() - 1,rows, cols);

		// Decompose model matrix
		glm::vec3 position, scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, rotation, position, skew, perspective);

		// Calculate box dimensions based on grid size and scale
		// Assuming each grid cell is 1 unit in size; adjust scale accordingly
		Vec3 boxHalfExtents(
			(cols * 4) * 0.5f, // Half-width
			0.5f * scale.y,          // Half-height (assuming thin box in Y)
			(rows * 4) * 0.5f);  // Half-depth

		
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

		
		BodyID physicsID = physick.bodyInterface.CreateAndAddBody(
			boxBodySettings,
			EActivation::Activate
		);

		physicsComponents.emplace_back(physicsID);

		Entity temp(entityIDCounter, physicsID, static_cast<uint32_t>(models.size() - 1));

		entityIDCounter++;

		return temp;

	}



};

uint32 EntityFactory::entityIDCounter = 0;
