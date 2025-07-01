#pragma once

#include <unordered_map>

#include "Mesh.hpp"
#include <glm/gtx/matrix_decompose.hpp>

using std::vector;


struct ModelInstance {
	vector <MeshInstance> meshes;

};


class ModelSource {

public:

	//Transform transform;

	vector <MeshSource> meshes;
	vector<aiMatrix4x4> aiMeshTransforms;

	ModelSource(const char * filePath,SDL_GPUDevice* device)
	{

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
			return;
		}

		cout << "processsing model : " << filePath << '\n';

		aiMeshTransforms.resize(scene->mNumMeshes);
		// Extract all mesh transforms first
		ExtractMeshTransforms(scene->mRootNode, aiMatrix4x4(), scene);

		meshes.reserve(scene->mNumMeshes);

		for (int i = 0; i < scene->mNumMeshes; i++) {

			aiMesh* importedMesh = scene->mMeshes[i];
			meshes.emplace_back();
			meshes.back().processMesh(importedMesh);

			// set mesh transfrom
			Transform temp;
			decomposeModelMatrix(ConvertMatrix(aiMeshTransforms[i]), temp);

			meshes.back().transform = temp;

			meshes.back().createVertexBuffer(device);

		}

	}

	ModelSource(int rows, int cols,SDL_GPUDevice* device) {

		meshes.emplace_back();

		MeshSource& mesh = meshes.back();

		GridGenerator::generateGrid(256, 256, mesh.vertices, mesh.indices);

		mesh.createVertexBuffer(device);
	}

	// Copy constructor
	ModelSource(const ModelSource& other)
	{
		printf("\033[33mModel copy constructor called!\033[0m\n");
	}


	void ExtractMeshTransforms(const aiNode* node, const aiMatrix4x4& parentTransform, const aiScene* scene) {
		// Calculate current node's world transform
		aiMatrix4x4 currentTransform = parentTransform * node->mTransformation;

		// Process all meshes in this node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			unsigned int meshIndex = node->mMeshes[i];
			aiMeshTransforms[meshIndex] = currentTransform;
		}

		// Recursively process child nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ExtractMeshTransforms(node->mChildren[i], currentTransform, scene);
		}
	}

	// Convert aiMatrix4x4 to your matrix type 
	glm::mat4 ConvertMatrix(const aiMatrix4x4& aiMat) {
		return glm::mat4(
			aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
		);
	}

	bool decomposeModelMatrix(
		const glm::mat4& matrix,Transform & tranfrom){
		
		glm::vec3 skew;
		glm::vec4 perspective;
		return glm::decompose(matrix, tranfrom.scale, tranfrom.rotation, tranfrom.position, skew, perspective);
	}

	bool getTransfromFromMat4(const glm::mat4& matrix) {

	}

	bool createInstance(ModelInstance & instance) {

		instance.meshes.resize(meshes.size());

		for (int i = 0; i < meshes.size(); i++) {

			instance.meshes[i].transform = meshes[i].transform;

			instance.meshes[i].vertexBuffer = meshes[i].vertexBuffer;
			instance.meshes[i].indexBuffer = meshes[i].indexBuffer;

			instance.meshes[i].indicesNum = meshes[i].indices.size();

		}

		return true;
	}

};


