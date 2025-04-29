#pragma once

#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>


//TODO use this eventually as it fits inside a cache line 
/*
struct TransformData {
	glm::vec3 prevPosition;   // 12 bytes
	glm::quat prevRotation;   // 16 bytes
	glm::vec3 currPosition;   // 12 bytes
	glm::quat currRotation;   // 16 bytes
};
*/

struct TransformData {
	glm::mat4 previousMatrix;
	glm::mat4 currentMatrix;
	glm::mat4 interpolatedMatrix;

	TransformData(const glm::mat4& m)
		: previousMatrix(m), currentMatrix(m), interpolatedMatrix(m) {
	}
};


struct VertexData {
	glm::vec3 vertices;
	glm::uvec3 normal;
	glm::vec2 texCoords;
};

struct PhysicsData {
	JPH::BodyID bodyID;
};
