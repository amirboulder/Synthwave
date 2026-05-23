#pragma once

class AssetManager;

struct AssetManagerRef {
	AssetManager* assetManager;
};


class AssetManager {

public:

	flecs::world& ecs;
	Manifest & manifest;

	GeometryPool geometryPool;

	std::unordered_map<uint64_t, MeshComponent> meshes;

	std::unordered_map<uint64_t, uint32_t> materialIdToIndex; //Maps material Id to their index in materials Vector
	std::vector<Material> materials;

	std::unordered_map<uint64_t, uint32_t> diffuseTextureIdToIndex;//Maps texture Id to their index in textureArrays
	std::unordered_map<uint64_t, uint32_t> MRTextureIdToIndex;//Maps Metallic Roughness texture Id to their index in textureArrays
	std::unordered_map<uint64_t, uint32_t> NormalTextureIdToIndex;//Maps Metallic Roughness texture Id to their index in textureArrays
	TextureArrays textureArrays;

	AssetManager(flecs::world& ecs, Manifest& manifest)
		:ecs(ecs), manifest(manifest)
	{
		 //Register the ref component
		ecs.component<AssetManagerRef>();
		ecs.set<AssetManagerRef>({ this });

		const RenderContext& renderContext = ecs.get<RenderContext>();

		geometryPool.init(renderContext.device);
		textureArrays.init(renderContext.device);

		//Creating defaultTexture for meshes that don't have a texture
		SDL_Surface* imageData = RenderUtil::LoadImage("assets/checkerboard.bmp", 4);
		if (imageData == NULL)
		{
			LogError(LOG_RENDER, "Could not load checkerboard.bmp image data!");
		}

		//Setting texture 0 and material 0
		RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.diffuseTextures, imageData);
		//TODO
		//RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.metallicRoughnessTextures, imageData);
		//RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.normalTextures, imageData);
		Material mat;
		materials.push_back(mat);

	}

	//TODO upgrade to cpp23 and used std::expected
	/// <summary>
	/// If the mesh is already loaded then it returns a copy of its MeshComponent,
	/// If not then it will load the mesh and its dependencies, Mesh will be loaded into geometry buffer.
	/// </summary>
	MeshComponent requestMeshComponent(const uint64_t& ID) {

		MeshComponent meshComp;

		auto it = meshes.find(ID);

		if (it != meshes.end()) {

			return  it->second;
		}
		// If not in the map then load it
		else {

			
			AssetMetadata* assetMetaData =  manifest.Find(ID);
		
			Mesh mesh;
			MeshHeader meshHeader;

			if (!RenderUtil::loadMeshFromFile(assetMetaData->cookedPath, mesh, meshHeader)) {
				LogError(LOG_APP, "Failed to loadMeshFromFile");
			}

			fillMeshComp(mesh, meshComp, ID);

			return meshComp;
		}
	}


	//Just loads and returns the mesh,
	// used by entities that need the mesh vertex to create their physics bodies
	//We could in theory read that data from the geomtery bufffer buy why complicate things.
	Mesh requestMesh(const uint64_t& ID) {

		Mesh mesh;
		MeshHeader meshHeader;

		AssetMetadata* assetMetaData = manifest.Find(ID);

		if (!RenderUtil::loadMeshFromFile(assetMetaData->cookedPath, mesh, meshHeader)) {
			LogError(LOG_APP, "Failed to loadMeshFromFile");
		}

		return mesh;
	}

	// fills meshComp data from Mesh and GeometryPool, used by requestMeshComp in most circumstances,
	// but also can be used to add MeshData to the Geopool and meshes map for thing like generated meshes for example
	//TODO maybe a better name for this
	void fillMeshComp(const Mesh & mesh, MeshComponent& meshComp, const uint64_t& ID) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		//Set MeshComp Data
		meshComp.transform = mesh.transform;
		meshComp.subMeshCount = mesh.subMeshCount;
		meshComp.size = mesh.size;

		for (size_t i = 0; i < mesh.subMeshCount; i++) {

			const SubMesh& srcSubMesh = mesh.subMeshes[i];

			SubMesh& destSubMesh = meshComp.subMeshes[i];

			destSubMesh.vertexCount = srcSubMesh.vertexCount;
			destSubMesh.indexCount = srcSubMesh.indexCount;
			destSubMesh.materialID = srcSubMesh.materialID;

		}

		//This sets the rest of the data
		if (!geometryPool.addMeshCompToBuffer(renderContext.device, mesh, meshComp)) {
			LogError(LOG_APP, "Failed to addMeshCompToBuffer");
		}


		//For all the active subMeshes in the meshComp get the materialIndex in materials Vector
		// and update the subMeshes to reference it.
		for (int i = 0; i < meshComp.subMeshCount; i++) {

			uint32_t materialIndex = requestMaterialIndex(meshComp.subMeshes[i].materialID);

			meshComp.subMeshes[i].materialIndex = materialIndex;
		}

		//This will copy the meshComp so it will still be outside of this function right ?
		meshes[ID] = meshComp;

	}

	bool isMeshCompLoaded(const uint64_t& ID) {

		auto it = meshes.find(ID);

		if (it != meshes.end()) {

			return true;
		}
		else {

			return false;
		}
	}

	//Return MaterialIndex of each Material in materials vector.
	//If the material is not loaded it will load it and its dependencies.
	uint32_t requestMaterialIndex(const uint64_t& ID) {

		if (ID == 0) {
			return 0; // default Material
		}

		auto it = materialIdToIndex.find(ID);

		if (it != materialIdToIndex.end()) {

			//If the material is loaded then we assume the textures it references are loaded
			return  it->second;
		}
		else {

			Material material;

			const RenderContext& renderContext = ecs.get<RenderContext>();
			AssetMetadata* assetMetaData = manifest.Find(ID);

			MaterialData materialData;

			RenderUtil::loadMaterialDataFromFile(assetMetaData->cookedPath, materialData);
			
			material.baseColorTexIndex = requestTextureIndex(materialData.baseColorTexID, diffuseTextureIdToIndex, TextureMapType::BaseColor);
			material.baseColorFactor = materialData.baseColorFactor;

			material.metallicRoughnessTexIndex = requestTextureIndex(materialData.metallicRoughnessTexID, MRTextureIdToIndex, TextureMapType::metallicRoughness);
			material.metallicFactor = materialData.metallicFactor;
			material.roughnessFactor = materialData.roughnessFactor;

			material.normalTexIndex = requestTextureIndex(materialData.normalTexID, NormalTextureIdToIndex, TextureMapType::Normal);

			materials.push_back(material);

			uint32_t index = static_cast<uint32_t>(materials.size()) - 1;
			materialIdToIndex[ID] = index;
			return index;

		}
	}

	/// <summary>
	/// returns  the index of the texture within its relevant textureArray within textureArrays
	/// Will Attempt to load the texture if not not loaded.
	/// </summary>
	uint32_t requestTextureIndex(const uint64_t& ID, std::unordered_map<uint64_t, uint32_t> & IdToIndex, TextureMapType texType) {

		if (ID == 0) {
			return 0; // default texture
		}

		uint32_t textureIndex = 0;

		auto it = IdToIndex.find(ID);

		if (it != IdToIndex.end()) {

			return  it->second;
		}
		else {

			const RenderContext& renderContext = ecs.get<RenderContext>();

			AssetMetadata* assetMetaData = manifest.Find(ID);

			TexHeader texHeader{};
			std::vector<uint8_t> pixels;

			if (!RenderUtil::loadTexFromFile(assetMetaData->cookedPath, texHeader, pixels)) {
				return 0;
			}

			SDL_Surface* surface = RenderUtil::createSurfaceFromPixels(texHeader, pixels);

			if (!surface) {
				return 0;
			}


			if (texType == TextureMapType::BaseColor) {

				textureIndex = textureArrays.diffuseTextures.usedLayers;

				if (!RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.diffuseTextures, surface))
					return 0;
			}
			else if (texType == TextureMapType::metallicRoughness) {

				textureIndex = textureArrays.metallicRoughnessTextures.usedLayers;

				if(!RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.metallicRoughnessTextures, surface))
					return 0;
			}

			IdToIndex[ID] = textureIndex;

			return textureIndex;
		}
	}
};

