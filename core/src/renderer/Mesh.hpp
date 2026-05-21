#pragma once

#include "../ecs/components.hpp"
#include "Grid.hpp"

struct MeshHeader {
	uint32_t magic = 0x4D455348; // 'MESH' catches corrputed files on load
	uint32_t version = 1;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	uint32_t subMeshCount = 0;
	uint32_t vertexStride = sizeof(Vertex);
	uint64_t AssetID;
};

//TODO make this part of an engine config file
constexpr uint32_t MAX_SUBMESHES = 8;


struct SubMesh {

	uint32_t baseVertex = UINT32_MAX;
	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	uint32_t materialID = 0;
};


class Mesh {

public:

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	Transform transform;

	std::array<SubMesh, 8> subMeshes; // subMesh data in here is relative to the mesh

	uint32_t subMeshCount = 0;

	



	static void calculateMeshSize(const Mesh& mesh, float& x, float& y, float& z) {
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

			minX = std::min(minX, current.position.x);
			maxX = std::max(maxX, current.position.x);

			minY = std::min(minY, current.position.y);
			maxY = std::max(maxY, current.position.y);

			minZ = std::min(minZ, current.position.z);
			maxZ = std::max(maxZ, current.position.z);
		}

		x = maxX - minX;
		y = maxY - minY;
		z = maxZ - minZ;
	}
};

/// <summary>
/// A standalone mesh, not a part of the mesh registry or mega buffers
/// </summary>
struct MeshStandalone {

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	Transform transform;

	std::array<SubMesh, 8> subMeshes; // subMesh data in here is relative to the mesh

	uint32_t subMeshCount = 0;

	Transform transform; //local transform relative to entity's position

	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;

	uint32_t indexCount = 0;
};
