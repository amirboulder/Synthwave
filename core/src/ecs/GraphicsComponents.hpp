
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

//TODO maybe stop using his as a component use it only for serialization
struct Transform {
	glm::vec3 position = glm::vec3(1);
	glm::quat rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 scale = glm::vec3(1);
};

//TODO remove this 
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


struct ModelSourceName {
	std::string name;
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
struct Light {};

/// <summary>
/// Infinitely far away, parallel rays — sun, moon
/// </summary>
struct DirectionalLight {
	glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
	float     intensity = 0.3f;
	glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.80f);
	float  _pad;
};

/// <summary>
/// Radiates in all directions from a point, fades with distance
/// </summary>
struct PointLight {
	glm::vec3 position = { 1.0, 1.0 ,1.0 };
	float     radius = 10.0f;   // max influence distance (for attenuation cutoff)
	float     intensity = 0.1f;
	glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.80f);

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


//The following is Used as relationship tag between entities and meshes.
//Having multiple tag allows 
struct BaseMesh {};
struct SecondaryMesh {};

