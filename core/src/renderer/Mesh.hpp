#pragma once

#include "../ecs/components.hpp"
#include "Grid.hpp"



class MeshSource {

public:

	std::vector<Vertex> vertices;
	std::vector <unsigned int> indices;

	Transform transform;

	SDL_GPUBuffer* vertexBuffer = NULL;
	SDL_GPUBuffer* indexBuffer = NULL;

	uint32_t size = 0;

	//Used to map A mesh to its corresponding MeshAsset in AssetLibrary
	uint32_t meshRegistryIndex = UINT32_MAX;

	SDL_GPUTexture* diffuseTexture;


	bool processMesh(aiMesh* importedMesh, std::string filename) {

		vertices.reserve(importedMesh->mNumVertices);

		for (int i = 0; i < importedMesh->mNumVertices; i++) {


			vertices.emplace_back();
			Vertex& currentVertex = vertices.back();

			if (!importedMesh->HasPositions()) {
				LogError(LOG_RENDER, "Mesh in file %s does not have positions !", filename.c_str());
				return false;
			}

			currentVertex.position.x = importedMesh->mVertices[i].x;
			currentVertex.position.y = importedMesh->mVertices[i].y;
			currentVertex.position.z = importedMesh->mVertices[i].z;

			if (!importedMesh->HasNormals()) {
				LogWarn(LOG_RENDER, "Mesh in file %s does not have normals !", filename.c_str());
			}
			else {
				currentVertex.normal.x = importedMesh->mNormals[i].x;
				currentVertex.normal.y = importedMesh->mNormals[i].y;
				currentVertex.normal.z = importedMesh->mNormals[i].z;
			}

			//Mesh can have multiple texture coordinates we're just using the first one for now.
			//cout << "texture coords" << importedMesh->HasTextureCoords(0) << '\n';
			if (importedMesh->HasTextureCoords(0) || importedMesh->mTextureCoords[0])
			{
				currentVertex.texCoord.x = importedMesh->mTextureCoords[0][i].x;
				currentVertex.texCoord.y = importedMesh->mTextureCoords[0][i].y;
			}
		}

		if (!importedMesh->HasFaces()) {
			LogError(LOG_RENDER, "Mesh in file %s does not have indices!", filename.c_str());
			return false;
		}

		indices.reserve(importedMesh->mNumFaces * 3);

		for (int i = 0; i < importedMesh->mNumFaces; i++) {

			for (int j = 0; j < importedMesh->mFaces[i].mNumIndices; j++) {
				indices.emplace_back(importedMesh->mFaces[i].mIndices[j]);
			}

		}

		size = indices.size();

		return true;

	}


	static void calculateMeshSize(const MeshSource& mesh, float& x, float& y, float& z) {
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



