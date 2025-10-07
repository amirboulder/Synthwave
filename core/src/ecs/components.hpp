#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>


struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};


struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);

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

struct RenderState {
	flecs::entity activePipeline;
};

struct PipelineRef {
	flecs::entity pipeline;
};



// Tags 
struct DynamicEnt {};
struct StaticEnt {};
struct Renderable {};
struct Sensor {};
struct CustomPipeline {};
