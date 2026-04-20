#pragma once

#include "Mesh.hpp"
#include "Materials/Material.hpp"
#include "renderUtil.hpp"

#include "GeometryPool.hpp"



class Model {

public:

	vector <Mesh> meshes;
	vector<aiMatrix4x4> aiMeshTransforms;
	vector<MaterialSMPL> materials;

	Model(flecs::world& ecs, std::vector<MaterialSMPL>& materialRegistry, SDL_GPUTexture* defaultTexture, const char* filePath)
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
			LogError(LOG_RENDER, "Assimp threw exception: %s", e.what());
			return;
		}
		catch (...) {
			LogError(LOG_RENDER, "Assimp threw an unknown exception");
			return;
		}

		if (!scene) {
			const char* error = importer.GetErrorString();
			LogError(LOG_RENDER,
				"ASSIMP Error: %s", error ? error : "Unknown error");
			return;
		}

		//TODO maybe change to debug priority eventually
		LogDebug(LOG_RENDER, "processing model : %s", filePath);

		// using resize instead of reserve to set the transforms to identity matrix 
		aiMeshTransforms.resize(scene->mNumMeshes);
		// Extract all mesh transforms first
		ExtractMeshTransforms(scene->mRootNode, aiMatrix4x4(), scene);

		meshes.reserve(scene->mNumMeshes);

		const RenderContext& renderContext = ecs.get<RenderContext>();
		GeometryPool& geometryPool = ecs.get_mut<GeometryPool>();

		MaterialLoader matLoader;
		matLoader.loadMaterials(scene, materials, filePath, renderContext.device);

		std::unordered_map<uint32_t, uint32_t> aiMatIndexToRegistryIndex;
		for (int i = 0; i < scene->mNumMeshes; i++) {

			aiMesh* importedMeshCurrent = scene->mMeshes[i];

			meshes.emplace_back();
			Mesh& currentMesh = meshes.back();

			currentMesh.processMesh(importedMeshCurrent, filePath);

			// use the aiMeshTransforms we extracted using ExtractMeshTransforms to set the mesh matrix
			Transform temp;
			decomposeModelMatrix(ConvertMatrix(aiMeshTransforms[i]), temp);

			currentMesh.transform = temp;

			geometryPool.upload(renderContext.device, currentMesh);

			//get the first diffuse texture of the material which this mesh uses
			// the string str only contains the index to the texture

			MaterialSMPL mat = {};
			uint32_t aiMatIdx = importedMeshCurrent->mMaterialIndex;
			aiString str;
			if (scene->mMaterials[aiMatIdx]->GetTexture(aiTextureType_DIFFUSE, 0, &str) == aiReturn_SUCCESS) {
				mat = materials[aiMatIdx];  // has a real texture
			}
			else 
			{
				mat.diffuseTexture = defaultTexture;  // fallback
			}


			auto it = aiMatIndexToRegistryIndex.find(aiMatIdx);
			if (it != aiMatIndexToRegistryIndex.end()) {
				// Already registered — reuse
				currentMesh.materialID = it->second;
			}
			else {
				// New material — register it
				uint32_t newIndex = (uint32_t)materialRegistry.size();
				materialRegistry.push_back(materials[aiMatIdx]);
				aiMatIndexToRegistryIndex[aiMatIdx] = newIndex;
				currentMesh.materialID = newIndex;
			}
		}

	}

	// for GRID
	// for generated meshes
	Model(flecs::world& ecs, uint32_t size) {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		GeometryPool& geometryPool = ecs.get_mut<GeometryPool>();

		meshes.emplace_back();

		Mesh& mesh = meshes.back();

		GridGenerator::generateGrid(size, mesh.vertices, mesh.indices);

		mesh.indexCount = mesh.indices.size();
		mesh.vertexCount = mesh.vertices.size();

		geometryPool.upload(renderContext.device, mesh);

	}


	/// <summary>
	/// Used for any arbitrary generated single mesh model
	/// </summary>
	Model(flecs::world& ecs, Mesh& mesh) {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		GeometryPool& geometryPool = ecs.get_mut<GeometryPool>();

		mesh.indexCount = mesh.indices.size();
		mesh.vertexCount = mesh.vertices.size();

		geometryPool.upload(renderContext.device, mesh);

	}

	// Copy constructor
	Model(const Model& other)
	{
		LogWarn(LOG_RENDER, "Model copy constructor called!");
	}

	bool validateFile(const char* filePath) {

		if (!filePath) {
			LogError(LOG_RENDER, "filePath is null");
			return false;
		}
		std::filesystem::path folderPath = filePath;
		if (!std::filesystem::exists(folderPath)) {
			LogError(LOG_RENDER, "File does not exist: %s", filePath);
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
		const glm::mat4& matrix, Transform& tranform) {

		glm::vec3 skew;
		glm::vec4 perspective;
		return glm::decompose(matrix, tranform.scale, tranform.rotation, tranform.position, skew, perspective);
	}



};

