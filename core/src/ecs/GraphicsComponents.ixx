module;

#include <vector>
#include <string>

export module GraphicsComponents;

import GLM;
import Flecs;

export struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};

export struct LineVertex {
	glm::vec3 position;
	glm::vec4 color;
};

export struct EntIdVertex {
	glm::vec3 position;
	uint32_t entID;
};


export struct Transform {
	glm::vec3 position = glm::vec3(0);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);
};

export struct WorldMatrix {
	glm::mat4 matrix = glm::mat4(1.0f);
};


export struct LineVertices {
	std::vector<LineVertex> data;
};


export struct RenderState {
	flecs::entity activePipeline;
};

export struct PipelineRef {
	flecs::entity pipeline;
};


export struct ModelSourceName {
	std::string name;
};

export struct RenderPipeline {};

export struct ActiveCamera {};


/// <summary>
/// A Tag attached to objects that should be rendered in game.
/// Used by renderer queries.
/// </summary>
export struct Renderable {};

/// <summary>
/// Attached to Lights.
/// Used by renderer queries.
/// </summary>
export struct Light {};

/// <summary>
/// Infinitely far away, parallel rays — sun, moon
/// </summary>
export struct DirectionalLight {
	glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
	float     intensity = 0.3f;
	glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.80f);
	float  _pad;
};

/// <summary>
/// Radiates in all directions from a point, fades with distance
/// </summary>
export struct PointLight {
	glm::vec3 position = { 1.0, 1.0 ,1.0 };
	float     radius = 10.0f;   // max influence distance (for attenuation cutoff)
	float     intensity = 0.1f;
	glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.80f);

};

// Cone-shaped light — flashlight, stage light
export struct SpotLight {
	float     radius = 10.0f;
	float     innerConeAngle = 15.0f; // degrees — full intensity inside this
	float     outerConeAngle = 30.0f; // degrees — fades to zero at outer edge
};


// Emits from a surface area
export struct AreaLight {
	glm::vec2 size = glm::vec2(1.0f); // width and height of the emitting surface
};


export struct ActorDebugInfo {

	bool playerVisible = false;
};
