#pragma once

#include "Mesh.hpp"
#include "Material.hpp"
#include "renderUtil.hpp"


struct LoadedTexture {
	SDL_Surface* surface;
	std::string name;
	std::string format;
	int width;
	int height;
};

// each model has one pipelineType
struct ModelInstance {
	vector <MeshInstance> meshes;
};


class ModelSource {

public:

	vector <MeshSource> meshes;
	vector<aiMatrix4x4> aiMeshTransforms;
	vector<MaterialSMPL> materials;

	ModelSource(flecs::world& ecs,const char * filePath)
	{
		if (!validateFile(filePath)) return;
		
		Assimp::Importer importer;
		const aiScene* scene = nullptr;

		try {
			scene = importer.ReadFile(
				filePath,
				aiProcess_Triangulate |
				aiProcess_GenSmoothNormals |
				aiProcess_FlipUVs |
				aiProcess_CalcTangentSpace
			);
		}
		catch (const std::exception& e) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Assimp threw exception: %s", e.what());
			return;
		}
		catch (...) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Assimp threw an unknown exception");
			return;
		}

		if (!scene) {
			const char* error = importer.GetErrorString();
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"ASSIMP Error: %s", error ? error : "Unknown error");
			return;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "processing model : %s", filePath);

		// using resize instead of reserve to set the transforms to identity matrix 
		aiMeshTransforms.resize(scene->mNumMeshes);
		// Extract all mesh transforms first
		ExtractMeshTransforms(scene->mRootNode, aiMatrix4x4(), scene);

		meshes.reserve(scene->mNumMeshes);

		const RenderContext& renderContext = ecs.get<RenderContext>();

		MaterialLoader matLoader;
		matLoader.loadMaterials(scene, materials,filePath, renderContext.device);
		

		for (int i = 0; i < scene->mNumMeshes; i++) {

			aiMesh* importedMeshCurrent = scene->mMeshes[i];

			meshes.emplace_back();
			MeshSource& currentMesh = meshes.back();

			currentMesh.processMesh(importedMeshCurrent);

			// use the aiMeshTransforms we extracted using ExtractMeshTransforms to set the mesh matrix
			Transform temp;
			decomposeModelMatrix(ConvertMatrix(aiMeshTransforms[i]), temp);

			currentMesh.transform = temp;

			RenderUtil::uploadBufferData(renderContext.device, currentMesh.vertexBuffer, currentMesh.vertices.data(),
				currentMesh.vertices.size() * sizeof(Vertex), SDL_GPU_BUFFERUSAGE_VERTEX);
			RenderUtil::uploadBufferData(renderContext.device, currentMesh.indexBuffer, currentMesh.indices.data(),
				currentMesh.indices.size() * sizeof(unsigned int), SDL_GPU_BUFFERUSAGE_INDEX);
	
			//get the first diffuse texture of the material which this mesh uses
			// the string str only contains the index to the texture
			aiString str;
			if (scene->mMaterials[importedMeshCurrent->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &str) == aiReturn_SUCCESS) {

				currentMesh.diffuseTexture = materials[importedMeshCurrent->mMaterialIndex].diffuseTexture;
			}
		}

	}

	// for GRID
	// for generated meshes
	ModelSource(flecs::world& ecs,int rows, int cols) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		meshes.emplace_back();

		MeshSource& mesh = meshes.back();

		GridGenerator::generateGrid(256, 256, mesh.vertices, mesh.indices);

		RenderUtil::uploadBufferData(renderContext.device, mesh.vertexBuffer, mesh.vertices.data(),
			mesh.vertices.size() * sizeof(Vertex), SDL_GPU_BUFFERUSAGE_VERTEX);

		RenderUtil::uploadBufferData(renderContext.device, mesh.indexBuffer, mesh.indices.data(),
			mesh.indices.size() * sizeof(unsigned int), SDL_GPU_BUFFERUSAGE_INDEX);
	}

	//for Wireframe Mountains
	ModelSource(flecs::world& ecs,const char* filePath, bool mtn) {

		if (!validateFile(filePath)) return;

		RenderContext& renderContext = ecs.get_mut<RenderContext>();

		Assimp::Importer importer;
		const aiScene* scene = nullptr;

		try {
			scene = importer.ReadFile(
				filePath,
				aiProcess_Triangulate |
				aiProcess_GenSmoothNormals |
				aiProcess_FlipUVs |
				aiProcess_CalcTangentSpace
			);
		}
		catch (const std::exception& e) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Assimp threw exception: %s", e.what());
			return;
		}
		catch (...) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Assimp threw an unknown exception");
			return;
		}

		if (!scene) {
			const char* error = importer.GetErrorString();
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				"ASSIMP Error: %s", error ? error : "Unknown error");
			return;
		}

		cout << "processing model : " << filePath << '\n';
		//cout << "Number of meshes : " << scene->mNumMeshes << '\n';

		aiMeshTransforms.resize(scene->mNumMeshes);
		// Extract all mesh transforms first
		ExtractMeshTransforms(scene->mRootNode, aiMatrix4x4(), scene);

		meshes.reserve(scene->mNumMeshes);

		for (int i = 0; i < scene->mNumMeshes; i++) {

			aiMesh* importedMesh = scene->mMeshes[i];
			meshes.emplace_back();

			MeshSource& currentMesh = meshes.back();
			currentMesh.processMeshsequential(importedMesh);

			// set mesh transform
			Transform temp;
			decomposeModelMatrix(ConvertMatrix(aiMeshTransforms[i]), temp);

			currentMesh.transform = temp;

			RenderUtil::uploadBufferData(renderContext.device, currentMesh.vertexBuffer, currentMesh.vertices.data(),
				currentMesh.vertices.size() * sizeof(Vertex), SDL_GPU_BUFFERUSAGE_VERTEX);
			RenderUtil::uploadBufferData(renderContext.device, currentMesh.indexBuffer, currentMesh.indices.data(),
				currentMesh.indices.size() * sizeof(unsigned int), SDL_GPU_BUFFERUSAGE_INDEX);

		}

	}

	// Copy constructor
	ModelSource(const ModelSource& other)
	{
		printf("\033[33mModel copy constructor called!\033[0m\n");
	}

	bool validateFile(const char* filePath) {

		if (!filePath) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "filePath is null");
			return false;
		}
		std::filesystem::path folderPath = filePath;
		if (!std::filesystem::exists(folderPath)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "File does not exist: %s", filePath);
			return false;
		}

		return true;
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

	
	ModelInstance createInstance() const {

		ModelInstance instance;

		instance.meshes.resize(meshes.size());

		for (int i = 0; i < meshes.size(); i++) {

			instance.meshes[i].transform = meshes[i].transform;

			instance.meshes[i].vertexBuffer = meshes[i].vertexBuffer;
			instance.meshes[i].indexBuffer = meshes[i].indexBuffer;

			instance.meshes[i].size = meshes[i].indices.size();

			instance.meshes[i].diffuseTexture = meshes[i].diffuseTexture;

		}

		return instance;
	}

};


