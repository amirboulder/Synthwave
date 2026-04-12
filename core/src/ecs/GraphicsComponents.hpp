
struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
	glm::vec4 color;
};

struct LineVertex {
	glm::vec3 position;
	glm::vec4 color;
};

struct EntIdVertex {
	glm::vec3 position;
	glm::uint32 entID;
};

//TODO stop using as a component use it only for serialization
struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);

};

struct MeshComponent {
	std::vector<uint32_t> MeshAssetIndices;
};


struct LineVertices {
	std::vector<LineVertex> data;
};


struct RenderState {
	flecs::entity activePipeline;
};

struct PipelineRef {
	flecs::entity pipeline;
};



struct RenderPipeline {};

struct ActiveCamera {};


/// <summary>
/// A Tag attached to objects that should be rendered in game.
/// Used by renderer queries.
/// </summary>
struct Renderable {};

/// <summary>
/// Attached to Lights.
/// Used by renderer queries.
/// </summary>
struct Light {
	glm::vec3 color = glm::vec3(1.0f);
	float     intensity = 1.0f;
	bool      castsShadows = false;
};

/// <summary>
/// Infinitely far away, parallel rays — sun, moon
/// </summary>
struct DirectionalLight {};

/// <summary>
/// Radiates in all directions from a point, fades with distance
/// </summary>
struct PointLight {
	float     radius = 10.0f;   // max influence distance (for attenuation cutoff)
};

// Cone-shaped light — flashlight, stage light
struct SpotLight {
	float     radius = 10.0f;
	float     innerConeAngle = 15.0f; // degrees — full intensity inside this
	float     outerConeAngle = 30.0f; // degrees — fades to zero at outer edge
};


// Emits from a surface area
struct AreaLight {
	glm::vec2 size = glm::vec2(1.0f); // width and height of the emitting surface
};

// Global constant light — no position, no direction, hits everything equally
struct AmbientLight {};