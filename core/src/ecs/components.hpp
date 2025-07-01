#pragma once

#include<glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>


struct TransformData {
	//glm::mat4 previousMatrix;
	glm::mat4 currentMatrix;
	//glm::mat4 interpolatedMatrix;

};


//for static meshes
struct TransformData2 {
	glm::vec3 position;  
	glm::quat rotation;   
	glm::vec3 scale;		
};


struct VertexData {
	glm::vec3 vertex;
	glm::vec3 normal;
	glm::vec2 texCoords;
	glm::vec4 color;
};

//struct MaterialData {
//	GLuint diffuseTextureID;
//	GLuint SpecularTextureID;
//};

struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);

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


struct Material {

};
