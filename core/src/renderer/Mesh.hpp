#pragma once

#include "../ecs/components.hpp"
#include "Grid.hpp"


using std::vector, std::string;


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


	bool processMesh(aiMesh* importedMesh) {

		vertices.reserve(importedMesh->mNumVertices);

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		int count = 1;


		for (int i = 0; i < importedMesh->mNumVertices; i++) {


			vertices.emplace_back();
			Vertex& currentVertex = vertices.back();

			currentVertex.position.x = importedMesh->mVertices[i].x;
			currentVertex.position.y = importedMesh->mVertices[i].y;
			currentVertex.position.z = importedMesh->mVertices[i].z;

			if (importedMesh->HasNormals()) {
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

			if (count > 3) {
				count = 1;
				r = 0;
				g = 0;
				b = 0;

			}

			if (count == 1) {
				r = 1.0;
				g = 0.0;
				b = 0.0;
			}

			if (count == 2) {
				r = 0.0;
				g = 1.0;
				b = 0.0;
			}

			if (count == 3) {
				r = 0.0;
				g = 0.0;
				b = 1.0;
			}

			currentVertex.color.x = r;
			currentVertex.color.y = g;
			currentVertex.color.z = b;
			currentVertex.color.w = 1.0f;

			count++;
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

	//MTN
	bool processMeshSequential(aiMesh* importedMesh) {

		vertices.reserve(importedMesh->mNumVertices);


		//Using Color for barycentric coords

		for (unsigned int i = 0; i < importedMesh->mNumFaces; ++i) {
			const aiFace& face = importedMesh->mFaces[i];


			for (int j = 0; j < 3; ++j) {
				unsigned int index = face.mIndices[j];

				vertices.emplace_back();
				Vertex& v = vertices.back();

				v.position.x = importedMesh->mVertices[index].x;
				v.position.y = importedMesh->mVertices[index].y;
				v.position.z = importedMesh->mVertices[index].z;

				if (importedMesh->HasNormals())
				{
					v.normal.x = importedMesh->mNormals[index].x;
					v.normal.y = importedMesh->mNormals[index].y;
					v.normal.z = importedMesh->mNormals[index].z;
				}
					

				if (importedMesh->HasTextureCoords(0))
				{
					v.texCoord.x = importedMesh->mTextureCoords[0][index].x;
					v.texCoord.y = importedMesh->mTextureCoords[0][index].y;
				}
					

				// Using color to Assign barycentric for wireframe
				switch (j) {
				case 0:
					v.color = glm::vec4(1, 0, 0, 0);
					break;
				case 1:
					v.color = glm::vec4(0, 1, 0, 0);
					break;
				case 2:
					v.color = glm::vec4(0, 0, 1, 0);
					break;
				}

			}
		}

		//Process indices to use for physics
		indices.reserve(importedMesh->mNumFaces * 3);
		// Generate sequential indices
		indices.reserve(vertices.size());
		for (size_t i = 0; i < vertices.size(); ++i) {
			indices.push_back(i);
		}

		size = indices.size();

		return true;
	}

	//TODO
	bool assignMaterial() {

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



