#pragma once

enum class DefaultAssets {

	CUBE,
	SPHERE,
	CAPSULE,
	CYLINDER,
	BOXCAR,
	ROBOT,	//TODO This should be a game asset
	MOUNTAIN, //TODO This should be a game asset
};


class AssetManager;

struct AssetManagerRef {
	AssetManager* assetManager;
};


class AssetManager {

public:

	flecs::world& ecs;
	const Manifest & manifest;

	GeometryPool geometryPool;

	std::unordered_map<DefaultAssets, uint64_t>defaultAssetsMap;

	std::unordered_map<uint64_t, MeshComponent> meshes;
	std::unordered_map<uint64_t, MeshNode> meshNodes;
	std::unordered_map<uint64_t, ModelData> models;

	std::vector<SubMeshComponent> subMeshes;

	std::unordered_map<uint64_t, uint32_t> materialIdToIndex; //Maps material Id to their index in materials Vector
	std::vector<Material> materials;

	std::unordered_map<uint64_t, uint32_t> diffuseTextureIdToIndex;//Maps texture ID to its index in textureArrays
	std::unordered_map<uint64_t, uint32_t> MRTextureIdToIndex;//Maps Metallic Roughness ID texture Id to their index in textureArrays
	std::unordered_map<uint64_t, uint32_t> NormalTextureIdToIndex;//Maps Normal texture ID to its index in textureArrays
	TextureArrays textureArrays;

	MeshComponent defaultMesh;
	MeshNode defaultMeshNode;
	ModelData defaultModel;
	Material defaultMaterial;

	AssetManager(flecs::world& ecs, const Manifest& manifest)
		:ecs(ecs), manifest(manifest)
	{
		 //Register the ref component
		ecs.component<AssetManagerRef>();
		ecs.set<AssetManagerRef>({ this });

		const RenderContext& renderContext = ecs.get<RenderContext>();

		geometryPool.init(renderContext.device);
		textureArrays.init(renderContext.device);

		createDefaultAssets(renderContext.device);
	}

	//TODO default assets should be generated not loaded that way there is no possibility of failure
	void createDefaultAssets(SDL_GPUDevice* device) {

		//TODO remove once assets are generated
		makeSureDefaultAssetsExistInManifest();

		defaultMaterial = createDefaultMaterial(device);

		generateDefaultCube(); //Cube will serve as default Mesh
		generateDefaultSphere();
		generateDefaultCylinder();
		generateDefaultCapsule();

		//BoxCar will serve as the default Model TODO Generate A Model instead
		defaultModel = requestModel(defaultAssetsMap.at(DefaultAssets::BOXCAR));

		defaultMeshNode = requestMeshNode(defaultModel.rootNodeID);
	}

	

	// A purple 1024x1024 for assets with missing textures
	// Checkerboard can be used for assets that don't have a texture
	Material createDefaultMaterial(SDL_GPUDevice* device) {

		Material mat;

		mat.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		mat.metallicFactor = 0.0f;   // generated meshes are dielectric, not metal
		mat.roughnessFactor = 0.6f;   // matte-to-semigloss; not 1.0 (kills specular) or 0 (mirror)

		// Diffuse layer 0 — white 
		SDLSurface diffuse0 = RenderUtil::makeSolidSurface(255, 255, 255, 255);
		if (!RenderUtil::uploadToTextureArray(device, textureArrays.diffuseTextures, diffuse0)) return mat;
		diffuseTextureIdToIndex[0] = 0;

		// MetallicRoughness layer 0 — white factors determine metallicRoughness
		SDLSurface mr0 = RenderUtil::makeSolidSurface(255, 255, 255, 255);
		if (!RenderUtil::uploadToTextureArray(device, textureArrays.metallicRoughnessTextures, mr0)) return mat;
		MRTextureIdToIndex[0] = 0;

		// Normal layer 0 — flat normal (0,0,1)
		SDLSurface normal0 = RenderUtil::makeSolidSurface(128, 128, 255, 255);
		if (!RenderUtil::uploadToTextureArray(device, textureArrays.normalTextures, normal0)) return mat;
		NormalTextureIdToIndex[0] = 0;

		mat.metallicFactor = 0.0f;
		mat.roughnessFactor = 0.6f;
		materials.push_back(mat);

		return mat;
	}

	/// <summary>
	/// As the name suggests this makes sure that the default assets exist in the manifest
	/// </summary>
	void makeSureDefaultAssetsExistInManifest() {

		//TODO generate boxCar as well
		defaultAssetsMap.insert({ DefaultAssets::BOXCAR, manifest.FindByName("model|assets/meshes/Robot2.glb|Hips") });
		defaultAssetsMap.insert({ DefaultAssets::ROBOT, manifest.FindByName("model|assets/meshes/EnemyCapsule.glb|Sphere") });
		defaultAssetsMap.insert({ DefaultAssets::MOUNTAIN, manifest.FindByName("mesh|assets/meshes/mtn4.glb|Plane.001") });

		for (const auto& pair : defaultAssetsMap) {

			if (pair.second == 0) {

				LogError(LOG_APP, "Default AssetID is zero Fix it!!!"); // TODO use magicEnum so we can log which asset
			}
		}
	}

	ModelData requestModel(const uint64_t& ID) {

		auto it = models.find(ID);

		if (it != models.end()) {

			//If the model is loaded then the root node is loaded
			return  it->second;
		}
		//If asset is not loaded then load it and all of its children
		else {

			const AssetMetadata* assetMetaData = manifest.Find(ID);
			if (!assetMetaData) {
				return defaultModel;
			}

			ModelHeader modelHeader;
			std::vector< MeshNode> meshNodesList;
			if (!RenderUtil::loadModelFromFile(assetMetaData->cookedPath, modelHeader, meshNodesList)) {
				LogError(LOG_APP, "Failed to loadModelFromFile for file %s returning default Model instead"
					,assetMetaData->cookedPath.c_str());
				return defaultModel;
			}

			for (const MeshNode& node : meshNodesList) {

				auto [it, inserted] = meshNodes.emplace( node.assetID, node);

				if (!inserted) {
					LogError(LOG_APP,
						"Cannot emplace Asset ID : %llu from file %s in meshNodes because it already exists,"
						"this means are the potential duplicate ids ",
						node.assetID, assetMetaData->cookedPath.c_str());
					return defaultModel;
				}
			}

			ModelData model = {
				.assetID = modelHeader.assetID,
				.rootNodeID = modelHeader.rootNodeID,
				.nodesNum = modelHeader.nodesNum,
			};

			models.emplace(model.assetID, model);

			return model;
		}	
	}

	

	MeshNode requestMeshNode(const uint64_t& ID) const {

		auto it = meshNodes.find(ID);

		if (it != meshNodes.end()) {

			return  it->second;
		}

		LogError(LOG_APP, "MeshNode with ID : %llu does not exist in AssetManager::meshNodes.Returning default meshNode instead."
			, ID);
		return defaultMeshNode;
	}

	//TODO upgrade to cpp23 and used std::expected
	/// <summary>
	/// If the mesh is already loaded then it returns a copy of its MeshComponent,
	/// If not then it will load the mesh and its dependencies, Mesh will be loaded into geometry buffer.
	/// </summary>
	MeshComponent requestMeshComponent(const uint64_t& ID) {

		MeshComponent meshComp{};

		auto it = meshes.find(ID);

		if (it != meshes.end()) {

			return  it->second;
		}
		// If not in the map then load it
		else {

			const AssetMetadata* assetMetaData =  manifest.Find(ID);
			if (!assetMetaData) {
				return defaultMesh;
			}
		
			Mesh mesh;
			MeshHeader meshHeader;

			if (!RenderUtil::loadMeshFromFile(assetMetaData->cookedPath, mesh, meshHeader)) {
				LogError(LOG_APP, "Failed to loadMeshFromFile");
				return defaultMesh; 
			}

			if (!fillMeshComp(mesh, meshComp, ID)) {
				return defaultMesh;
			}

			return meshComp;
		}
	}

	
	SubMeshComponent requestSubMeshComponent(const uint32_t& index) {

		if (index >= subMeshes.size()) {

			SubMeshComponent subMesh;
			return subMesh; // return empty subMesh
		}
		return subMeshes[index];
	}


	//loads and returns the mesh,
	// used by entities that need the mesh vertex to create their physics bodies
	//We could in theory read that data from the geometry buffer but why complicate things.
	Mesh requestMesh(const uint64_t& ID) {

		Mesh mesh;
		MeshHeader meshHeader;

		const AssetMetadata* assetMetaData = manifest.Find(ID);
		if (!assetMetaData) {
			return mesh; //returns default Mesh
		}


		if (!RenderUtil::loadMeshFromFile(assetMetaData->cookedPath, mesh, meshHeader)) {
			LogError(LOG_APP, "Failed to loadMeshFromFile");
		}

		return mesh;
	}

	// fills meshComp data from Mesh and GeometryPool, used by requestMeshComp in most circumstances,
	// but also can be used to add MeshData to the GeometryPool and meshes map for thing like generated meshes for example
	//TODO maybe a better name for this
	bool fillMeshComp(const Mesh & mesh, MeshComponent& meshComp, const uint64_t& ID) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		//Set MeshComp Data
		meshComp.subMeshCount = static_cast<uint8_t>(std::min(mesh.subMeshes.size(), size_t(255)));
		meshComp.aabb = mesh.aabb;
		meshComp.index = geometryPool.numMeshes;
		meshComp.firstSubMeshIndex = subMeshes.size();

		uint32_t vertexOffset = geometryPool.vertexHead;
		uint32_t indexOffset = geometryPool.indexHead;

		//For each subMesh create a subMesh component
		for (size_t i = 0; i < mesh.subMeshes.size(); i++) {

			const SubMesh& srcSubMesh = mesh.subMeshes[i];

			SubMeshComponent& destSubMesh = subMeshes.emplace_back();

			destSubMesh.vertexCount = srcSubMesh.vertexCount;
			destSubMesh.indexCount = srcSubMesh.indexCount;
			destSubMesh.materialIndex = requestMaterialIndex(srcSubMesh.materialID);

			destSubMesh.vertexOffset = vertexOffset;
			destSubMesh.firstIndex = indexOffset;

			vertexOffset += srcSubMesh.vertexCount;
			indexOffset += srcSubMesh.indexCount;

		}

		if (!geometryPool.addMeshToBuffer(renderContext.device, mesh)) {
			LogError(LOG_APP, "Failed to addMeshCompToBuffer");
			return false;
		}

		meshes[ID] = meshComp;

		return true;
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

			Material material{};

			const AssetMetadata* assetMetaData = manifest.Find(ID);
			if (!assetMetaData) {
				return 0; //defaultMaterial
			}

			MaterialData materialData{};

			if (!RenderUtil::loadMaterialDataFromFile(assetMetaData->cookedPath, materialData)) {
				LogError(LOG_APP,
					"Failed to load material data from file %s corresponding to source file %s",
					assetMetaData->cookedPath.c_str(), assetMetaData->sourcePath.c_str());
				return 0;
			}
			
			material.baseColorTexIndex = requestTextureIndex(materialData.baseColorTexID,
				diffuseTextureIdToIndex, TextureMapType::BaseColor);
			material.baseColorFactor = materialData.baseColorFactor;

			material.metallicRoughnessTexIndex = requestTextureIndex(
				materialData.metallicRoughnessTexID
				, MRTextureIdToIndex,
				TextureMapType::metallicRoughness);
			material.metallicFactor = materialData.metallicFactor;
			material.roughnessFactor = materialData.roughnessFactor;

			material.normalTexIndex = requestTextureIndex(materialData.normalTexID, NormalTextureIdToIndex,
				TextureMapType::Normal);

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

			const AssetMetadata* assetMetaData = manifest.Find(ID);
			if (!assetMetaData) {
				return 0; //default Texture
			}

			TexHeader texHeader{};
			std::vector<uint8_t> pixels;

			if (!RenderUtil::loadTexFromFile(assetMetaData->cookedPath, texHeader, pixels)) {
				return 0;
			}

			SDLSurface surface(RenderUtil::createSurfaceFromPixels(texHeader, pixels, mapToSDLPixelFormat[texHeader.format]), SDL_DestroySurface);
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
			else if (texType == TextureMapType::Normal) {

				textureIndex = textureArrays.normalTextures.usedLayers;

				if (!RenderUtil::uploadToTextureArray(renderContext.device, textureArrays.normalTextures, surface))
					return 0;
			}

			IdToIndex[ID] = textureIndex;

			return textureIndex;
		}
	}


	void generateDefaultCube() {

		std::string assetName = std::format("DefaultCube");
		uint64_t id = util::generateAssetID(assetName);

		Mesh cubeMesh = createCubeMesh(1.0f);

		if (!fillMeshComp(cubeMesh, defaultMesh, id)) {
			LogError(LOG_APP, "Failed to generate DefaultCube");
			//Probably a fatal error at this point
		}

		defaultAssetsMap.insert({ DefaultAssets::CUBE, id });
	}

	void generateDefaultSphere() {

		std::string assetName = std::format("DefaultSphere");
		uint64_t id = util::generateAssetID(assetName);

		MeshComponent meshComp;
		Mesh mesh = createSphereMesh(1.0f);

		if (!fillMeshComp(mesh, meshComp, id)) {
			LogError(LOG_APP, "Failed to generate DefaultSphere");
			//Probably a fatal error at this point
		}

		defaultAssetsMap.insert({ DefaultAssets::SPHERE, id });
	}

	void generateDefaultCylinder() {

		std::string assetName = std::format("DefaultCylinder");
		uint64_t id = util::generateAssetID(assetName);

		MeshComponent meshComp;
		Mesh mesh = generateCylinderMesh(1.0f);

		if (!fillMeshComp(mesh, meshComp, id)) {
			LogError(LOG_APP, "Failed to generate DefaultCylinder");
			//Probably a fatal error at this point
		}

		defaultAssetsMap.insert({ DefaultAssets::CYLINDER, id });
	}

	void generateDefaultCapsule() {

		std::string assetName = std::format("DefaultCapsule");
		uint64_t id = util::generateAssetID(assetName);


		MeshComponent meshComp;
		Mesh mesh = generateCapsuleMesh(1.0f, 2.0f, 32, 16);

		if (!fillMeshComp(mesh, meshComp, id)) {
			LogError(LOG_APP, "Failed to generate DefaultCapsule");
			//Probably a fatal error at this point
		}

		defaultAssetsMap.insert({ DefaultAssets::CAPSULE, id });
	}
};

