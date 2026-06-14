#pragma once

#include "../ecs/components.hpp"
#include "ProceduralMeshes.hpp"

constexpr uint8_t numberOfLODs = 3;

struct AABB {
	glm::vec3 center = glm::vec3(0.0f);
	glm::vec3 extents = glm::vec3(0.0f); // half-size
};

struct MeshHeader {
	uint32_t magic = 0x4D455348; // 'MESH' catches corrupted files on load
	uint32_t version = 1;
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	uint32_t subMeshCount = 0;
	uint32_t vertexStride = sizeof(Vertex);
	uint64_t AssetID;
	uint32_t subMeshOffset = 0;  // bytes from file start
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	AABB aabb;
};


//struct LOD {
//
//	std::vector<uint32_t> indices;
//	float error;
//};

struct LODComponent {
	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	float error;
};

struct SubMesh {

	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	uint32_t baseVertex = UINT32_MAX;
	uint32_t vertexCount = 0;
	uint64_t materialID = 0;
};


struct SubMeshComponent {

	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	uint32_t vertexOffset = UINT32_MAX;
	uint32_t vertexCount = 0;

	uint32_t materialIndex = 0; // relative to Materials Vector in AssetManger
};


struct MeshNode {
	std::string name;
	uint64_t meshID = { 0 };
	uint64_t assetID = { 0 };
	Transform transform;
	std::vector<uint64_t> children;
};


struct ModelHeader {
	static constexpr uint64_t magic = 0x4D4F44454C; //MODEL 
	uint64_t assetID;       
	uint64_t rootNodeID;    // assetID of the root MeshNode
	uint32_t version = 1;
	uint32_t nodesNum;
};


struct ModelData {

	uint64_t assetID;
	uint64_t rootNodeID;
	uint32_t nodesNum;
};


//TODO use this 
struct MeshComponent {

	uint64_t index = 0; // relative to the geometry buffer used for sorting
	AABB aabb; // local aabb used for culling
	uint32_t firstSubMeshIndex; // index of the first submesh in AssetManager subMeshes vector
	uint8_t subMeshCount = 0;
	bool visible = true;
};




class Mesh {

public:

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	std::vector<SubMesh> subMeshes; // subMesh data in here is relative to the mesh

	AABB aabb;

//	std::array<LOD, numberOfLODs> LODs; 
	
	static AABB CalculateMeshAABB(const std::vector<Vertex>& vertices) {
		AABB aabb;
		if (vertices.empty()) {
			LogError(LOG_APP, "Vertices are empty, cannot calculate AABB!");
			return aabb;
		}

		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);

		for (const auto& v : vertices) {
			min = glm::min(min, v.position);
			max = glm::max(max, v.position);
		}

		aabb.center = (min + max) * 0.5f;
		aabb.extents = (max - min) * 0.5f;
		return aabb;
	}
};



/*
void generateLODs(Mesh& mesh)
{
	// --- Step 1: Optimize the base mesh first ---
	meshopt_optimizeVertexCache(
		mesh.indices.data(), mesh.indices.data(),
		mesh.indices.size(), mesh.vertices.size());

	meshopt_optimizeOverdraw(
		mesh.indices.data(), mesh.indices.data(),
		mesh.indices.size(),
		&mesh.vertices[0].position.x,      // float* positions
		mesh.vertices.size(),
		sizeof(Vertex),
		1.05f);                             // threshold: allow 5% more overdraw

	meshopt_optimizeVertexFetch(
		mesh.vertices.data(), mesh.indices.data(),
		mesh.indices.size(), mesh.vertices.data(),
		mesh.vertices.size(), sizeof(Vertex));


	// --- Step 2: Generate simplified LODs ---
	const std::vector<uint32_t>* prevIndices = &mesh.indices;

	for (int lod = 1; lod <= numberOfLODs; ++lod)
	{
		// Target: halve triangle count each LOD
		size_t targetIndexCount = prevIndices->size() / (2 * lod);
		float  targetError = 0.01f * lod; // grow error tolerance per LOD

		std::vector<uint32_t> lodBuffer(prevIndices->size());
		float resultError = 0.0f;

		size_t newIndexCount = meshopt_simplify(
			lodBuffer.data(),
			prevIndices->data(), prevIndices->size(),
			&mesh.vertices[0].position.x,     // float* positions
			mesh.vertices.size(),
			sizeof(Vertex),
			targetIndexCount,
			targetError,
			0,             // options (e.g. meshopt_SimplifyLockBorder)
			&resultError);

		lodBuffer.resize(newIndexCount);

		// Optionally stop if simplification stalled (< 10% reduction)
		if (newIndexCount > prevIndices->size() * 0.9f && lod > 1)
			break;

		mesh.LODs[lod].indices = lodBuffer;
		mesh.LODs[lod].error =  resultError;

		prevIndices = &mesh.LODs[lod].indices;
	}
}
*/

/// <summary>
/// A standalone mesh, not a part of the mesh registry or mega buffers
/// </summary>
struct MeshStandalone {

	std::vector<Vertex> vertices;
	std::vector <uint32_t> indices;
	Transform transform;//local transform relative to entity's position

	std::array<SubMesh, 8> subMeshes;

	uint32_t subMeshCount = 0;

	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;

	uint32_t indexCount = 0;
};


 Mesh createGridMesh(uint32_t size) {

	Mesh mesh;

	mesh.subMeshes.emplace_back();

	MeshGen::generateGrid(size, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = mesh.indices.size();
	mesh.subMeshes[0].vertexCount = mesh.vertices.size();

	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;


	return mesh;
}

Mesh createCubeMesh(float scale = 1.0f) {

	Mesh mesh;
	mesh.subMeshes.emplace_back();

	MeshGen::generateCube(scale, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = static_cast<uint32_t>(mesh.indices.size());
	mesh.subMeshes[0].vertexCount = static_cast<uint32_t>(mesh.vertices.size());
	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.aabb = Mesh::CalculateMeshAABB(mesh.vertices);

	return mesh;
}


Mesh createSphereMesh(float radius = 1, int sectors = 32, int stacks = 16) {

	Mesh mesh;
	mesh.subMeshes.emplace_back();

	MeshGen::generateSphere(radius, sectors, stacks, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = static_cast<uint32_t>(mesh.indices.size());
	mesh.subMeshes[0].vertexCount = static_cast<uint32_t>(mesh.vertices.size());
	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.aabb = Mesh::CalculateMeshAABB(mesh.vertices);

	return mesh;
}

Mesh generateCylinderMesh(float radius = 1, int height = 1, int segments = 32) {

	Mesh mesh;
	mesh.subMeshes.emplace_back();

	MeshGen::generateCylinder(radius, height, segments, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = static_cast<uint32_t>(mesh.indices.size());
	mesh.subMeshes[0].vertexCount = static_cast<uint32_t>(mesh.vertices.size());
	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.aabb = Mesh::CalculateMeshAABB(mesh.vertices);

	return mesh;
}

Mesh generateCapsuleMesh(float radius = 1.0f, int height = 2.0f, int sectors = 32, int ringsPerDome = 32) {

	Mesh mesh;
	mesh.subMeshes.emplace_back();

	MeshGen::generateCapsule(radius, height, sectors, ringsPerDome, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = static_cast<uint32_t>(mesh.indices.size());
	mesh.subMeshes[0].vertexCount = static_cast<uint32_t>(mesh.vertices.size());
	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.aabb = Mesh::CalculateMeshAABB(mesh.vertices);

	return mesh;
}