#pragma once

#include<glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <glad/glad.h>

//TODO use this eventually as it fits inside a cache line 

struct TransformData2 {
	glm::vec3 prevPosition;   // 12 bytes
	glm::quat prevRotation;   // 16 bytes
	glm::vec3 currPosition;   // 12 bytes
	glm::quat currRotation;   // 16 bytes
	float scale;			  // 4 bytes
};

struct TransformData {
	glm::mat4 previousMatrix;
	glm::mat4 currentMatrix;
	glm::mat4 interpolatedMatrix;

	TransformData(const glm::mat4& m)
		: previousMatrix(m), currentMatrix(m), interpolatedMatrix(m) {
	}
};


struct VertexData {
	glm::vec3 vertex;
	glm::uvec3 normal;
	glm::vec2 texCoords;
};

struct MeshData {
	glm::vec3 position; 
	glm::quat rotation;
	float scale;
	GLuint shaderID;
	GLuint VAO;
	GLuint diffuseTextureID;
	uint32_t indicesSize;
};

struct PhysicsData {
	JPH::BodyID bodyID;
};


struct PlayerInput {
	glm::vec3 direction = glm::vec3(0);
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	bool jump = false;
};


struct PlayerData {
	glm::vec3 Position = glm::vec3(0);
	glm::vec3 direction = glm::vec3(0);
	bool jump = false;              
};