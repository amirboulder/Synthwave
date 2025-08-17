#pragma once

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../ecs/components.hpp"
#include "Shader.hpp"
#include "Grid.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using std::vector, std::string;

void PrintGLMMat4(const glm::mat4& mat, const char * name) {
	std::cout << "GLM Matrix with index : " << name << ":\n";
	for (int row = 0; row < 4; ++row) {
		std::cout << "| ";
		for (int col = 0; col < 4; ++col) {
			std::cout << std::setw(10) << std::setprecision(4) << mat[col][row] << " ";
		}
		std::cout << "|\n";
	}
}

class MeshSource {

public:

	std::vector<VertexData> vertices;
	std::vector <unsigned int> indices;

	Transform  transform;

	SDL_GPUBuffer* vertexBuffer = NULL;
	SDL_GPUBuffer* indexBuffer = NULL;

	SDL_GPUTexture* diffuseTexture;


	bool processMesh(aiMesh* importedMesh) {

		vertices.reserve(importedMesh->mNumVertices);

		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		int count = 1;


		for (int i = 0; i < importedMesh->mNumVertices; i++) {


			vertices.emplace_back();
			VertexData& currentVertex = vertices.back();

			currentVertex.vertex.x = importedMesh->mVertices[i].x;
			currentVertex.vertex.y = importedMesh->mVertices[i].y;
			currentVertex.vertex.z = importedMesh->mVertices[i].z;

			if (importedMesh->HasNormals()) {
				currentVertex.normal.x = importedMesh->mNormals[i].x;
				currentVertex.normal.y = importedMesh->mNormals[i].y;
				currentVertex.normal.z = importedMesh->mNormals[i].z;
			}

			//Mesh can have multiple texture coordinates we're just using the first one for now.
			//cout << "texture coords" << importedMesh->HasTextureCoords(0) << '\n';
			if (importedMesh->HasTextureCoords(0) || importedMesh->mTextureCoords[0])
			{
				currentVertex.texCoords.x = importedMesh->mTextureCoords[0][i].x;
				currentVertex.texCoords.y = importedMesh->mTextureCoords[0][i].y;
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

		return true;

	}

	bool processMeshVertsOnly(aiMesh* importedMesh) {

		vertices.reserve(importedMesh->mNumVertices);


		//Using Color for barycentrcic coords

		for (unsigned int i = 0; i < importedMesh->mNumFaces; ++i) {
			const aiFace& face = importedMesh->mFaces[i];


			for (int j = 0; j < 3; ++j) {
				unsigned int index = face.mIndices[j];

				vertices.emplace_back();
				VertexData& v = vertices.back();

				v.vertex.x = importedMesh->mVertices[index].x;
				v.vertex.y = importedMesh->mVertices[index].y;
				v.vertex.z = importedMesh->mVertices[index].z;

				if (importedMesh->HasNormals())
				{
					v.normal.x = importedMesh->mNormals[index].x;
					v.normal.y = importedMesh->mNormals[index].y;
					v.normal.z = importedMesh->mNormals[index].z;
				}
					

				if (importedMesh->HasTextureCoords(0))
				{
					v.texCoords.x = importedMesh->mTextureCoords[0][index].x;
					v.texCoords.y = importedMesh->mTextureCoords[0][index].y;
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

		return true;
	}


	bool createVertexBuffer(SDL_GPUDevice* device) {

		//create vertex buffer
		SDL_GPUBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bufferCreateInfo.size = vertices.size() * sizeof(VertexData);

		vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
		if (!vertexBuffer) {
			std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
			return false;
		}

		//create index buffer
		SDL_GPUBufferCreateInfo idxCreateInfo = {};
		idxCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
		idxCreateInfo.size = indices.size() * sizeof(unsigned int);

		indexBuffer = SDL_CreateGPUBuffer(device, &idxCreateInfo);
		if (!indexBuffer) {
			std::cerr << "Failed to create index buffer: " << SDL_GetError() << std::endl;
			return false;
		}

		// Upload vertex data
		SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
		transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferBufferCreateInfo.size = bufferCreateInfo.size;

		// upload index data:
		SDL_GPUTransferBufferCreateInfo idxTransferInfo = {};
		idxTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		idxTransferInfo.size = idxCreateInfo.size;


		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
		if (!transferBuffer) {
			std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
			return false;
		}

		SDL_GPUTransferBuffer* idxTransfer = SDL_CreateGPUTransferBuffer(device, &idxTransferInfo);
		void* idxMap = SDL_MapGPUTransferBuffer(device, idxTransfer, false);
		memcpy(idxMap, indices.data(), idxCreateInfo.size);
		SDL_UnmapGPUTransferBuffer(device, idxTransfer);

		// Copy into GPU buffer
		auto cmdBufIndex = SDL_AcquireGPUCommandBuffer(device);
		auto copyPassIndex = SDL_BeginGPUCopyPass(cmdBufIndex);
		SDL_GPUTransferBufferLocation srcLoc{ idxTransfer, 0 };
		SDL_GPUBufferRegion dstRegion{ indexBuffer, 0, idxCreateInfo.size };
		SDL_UploadToGPUBuffer(copyPassIndex, &srcLoc, &dstRegion, false);
		SDL_EndGPUCopyPass(copyPassIndex);
		SDL_SubmitGPUCommandBuffer(cmdBufIndex);
		SDL_ReleaseGPUTransferBuffer(device, idxTransfer);


		// Map and copy data
		void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
		if (!mappedData) {
			std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
			SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
			return false;
		}

		memcpy(mappedData, vertices.data(), vertices.size() * sizeof(VertexData));
		SDL_UnmapGPUTransferBuffer(device, transferBuffer);

		// Copy from transfer buffer to vertex buffer
		SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

		SDL_GPUTransferBufferLocation transferLocation = {};
		transferLocation.transfer_buffer = transferBuffer;
		transferLocation.offset = 0;

		SDL_GPUBufferRegion bufferRegion = {};
		bufferRegion.buffer = vertexBuffer;
		bufferRegion.offset = 0;
		bufferRegion.size = bufferCreateInfo.size;

		SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
		return true;
	}

	// NO indices!!
	bool createVBuffer(SDL_GPUDevice* device) {

		//create vertex buffer
		SDL_GPUBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bufferCreateInfo.size = vertices.size() * sizeof(VertexData);

		vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
		if (!vertexBuffer) {
			std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
			return false;
		}

		// Upload vertex data
		SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
		transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferBufferCreateInfo.size = bufferCreateInfo.size;

		
		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
		if (!transferBuffer) {
			std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
			return false;
		}


		// Map and copy data
		void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
		if (!mappedData) {
			std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
			SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
			return false;
		}

		memcpy(mappedData, vertices.data(), vertices.size() * sizeof(VertexData));
		SDL_UnmapGPUTransferBuffer(device, transferBuffer);

		// Copy from transfer buffer to vertex buffer
		SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

		SDL_GPUTransferBufferLocation transferLocation = {};
		transferLocation.transfer_buffer = transferBuffer;
		transferLocation.offset = 0;

		SDL_GPUBufferRegion bufferRegion = {};
		bufferRegion.buffer = vertexBuffer;
		bufferRegion.offset = 0;
		bufferRegion.size = bufferCreateInfo.size;

		SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
		return true;
	}

	//TODO
	bool assignMaterial() {

	}


};

struct MeshInstance {

	Transform  transform;

	SDL_GPUBuffer* vertexBuffer = NULL;
	SDL_GPUBuffer* indexBuffer = NULL;

	SDL_GPUTexture* diffuseTexture;

	Uint32 size = 0;
};



