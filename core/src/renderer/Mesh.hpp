#pragma once

#include "../ecs/components.hpp"
#include "Grid.hpp"

struct MeshHeader {
	uint32_t magic = 0x4D455348; // 'MESH' catches corrupted files on load
	uint32_t version = 1;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	uint32_t subMeshCount = 0;
	uint32_t vertexStride = sizeof(Vertex);
	uint64_t AssetID;
	Transform transform;
	glm::vec3 size = glm::vec3(0.0f);
};

//TODO make this part of an engine config file
constexpr uint32_t MAX_SUBMESHES = 8;


struct SubMesh {

	uint32_t baseVertex = UINT32_MAX;
	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	uint64_t materialID = 0;
	uint32_t materialIndex = 0; // relatvie to Materials Vector in AssetManger
};

struct MeshComponent {

	Transform transform; //local transform relative to entity's position

	std::array<SubMesh, 8> subMeshes; // subMesh data in here is relative to the geometry pool

	uint32_t index = 0; // relative to the geometry buffer

	uint32_t subMeshCount = 0; // number of used sub meshes

	glm::vec3 size = glm::vec3(0.0f);
};


class Mesh {

public:

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	Transform transform;

	std::array<SubMesh, 8> subMeshes; // subMesh data in here is relative to the mesh

	uint32_t subMeshCount = 0; // number of used submeshes

	glm::vec3 size = glm::vec3(0.0f);
	
	static glm::vec3 calculateMeshSize(const std::vector<Vertex> & vertices) {

		if (vertices.empty()) {
			LogError(LOG_APP, "vertices are empty Cannot calculate Mesh size !!!");
			return glm::vec3(0.0f);
		}

		float minX = FLT_MAX, maxX = -FLT_MAX;
		float minY = FLT_MAX, maxY = -FLT_MAX;
		float minZ = FLT_MAX, maxZ = -FLT_MAX;

		for (const auto& current : vertices) {

			minX = std::min(minX, current.position.x);
			maxX = std::max(maxX, current.position.x);

			minY = std::min(minY, current.position.y);
			maxY = std::max(maxY, current.position.y);

			minZ = std::min(minZ, current.position.z);
			maxZ = std::max(maxZ, current.position.z);
		}

		return glm::vec3(maxX - minX, maxY - minY, maxZ - minZ);
	}
};

/// <summary>
/// A standalone mesh, not a part of the mesh registry or mega buffers
/// </summary>
struct MeshStandalone {

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	Transform transform;//local transform relative to entity's position

	std::array<SubMesh, 8> subMeshes; // subMesh data in here is relative to the mesh

	uint32_t subMeshCount = 0;

	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;

	uint32_t indexCount = 0;
};


 Mesh createGridMesh(uint32_t size) {

	Mesh mesh;

	GridGenerator::generateGrid(size, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = mesh.indices.size();
	mesh.subMeshes[0].vertexCount = mesh.vertices.size();

	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.subMeshCount = 1;

	return mesh;
}