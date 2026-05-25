#pragma once

#include "Manifest.hpp"


namespace AssetImporter {


	/// <summary>
	/// processes texture, materials, meshes, Scenes in a glTF file,
	/// assigns IDs to each and save the processed data to their respective files.
	/// inserts AssetMetadata into the manifest.
	/// Enforces assets having names.
	/// </summary>
	bool ImportGLTF(const fs::path& filePath, const fs::path& destFolder, Manifest& manifest)
	{
		//Used to map local texture/materials/meshes to their AssetIDs,
		//materials store AssetIDs of textures,
		// Meshes store AssetIDs of materials.
		//Scene nodes have meshes
		std::unordered_map<uint32_t, uint64_t> localToMaterialId;
		std::unordered_map<uint32_t, uint64_t> localToTextureId;
		std::unordered_map<uint32_t, uint64_t> localToMeshId;

		const std::string filePathStr = filePath.generic_string();

		// Ensure the directory exists before trying to write files
		fs::create_directories(destFolder);


		// Create the parser
		fastgltf::Parser parser;

		auto gltfData = fastgltf::GltfDataBuffer::FromPath(filePath);
		if (gltfData.error() != fastgltf::Error::None) {

			LogError(LOG_ERR, "Failed to load glTF file: %s error code %d", filePathStr.c_str(), fastgltf::to_underlying(gltfData.error()));
			return false;
		}

		// Parse the gltf
		auto asset = parser.loadGltf(gltfData.get(),
			std::filesystem::path(filePath).parent_path(),  // base path for resolving buffers/images
			fastgltf::Options::LoadExternalBuffers |         // auto-load .bin files
			fastgltf::Options::LoadExternalImages
		);

		if (asset.error() != fastgltf::Error::None) {
			LogError(LOG_ERR, "Failed to parse gltf: %s error code%d", filePathStr.c_str(), fastgltf::to_underlying(asset.error()));
			return false;
		}

		fastgltf::Asset& gltf = asset.get();

		LogDebug(LOG_ERR, "Info for 3D Asset file %s", filePath.generic_string().c_str());
		LogDebug(LOG_ERR, "Number of meshes : %d", gltf.meshes.size());
		LogDebug(LOG_ERR, "Number of Materials : %d", gltf.materials.size());
		LogDebug(LOG_ERR, "Number of Textures : %d", gltf.textures.size());
		LogDebug(LOG_ERR, "Number of Images : %d", gltf.images.size());
		LogDebug(LOG_ERR, "Number of Nodes : %d", gltf.nodes.size());
		LogDebug(LOG_ERR, "Number of Scenes : %d", gltf.scenes.size());

		

		uint64_t gltfFileContentHash = util::generateContentHash(filePath);

		if (gltfFileContentHash == 0) {
			LogError(LOG_ERR, "Failed to generate Content Hash for file %s", filePathStr.c_str());
			return false;
		}


		for (size_t i = 0; i < gltf.textures.size(); i++) {
			auto& texture = gltf.textures[i];

			if (!texture.imageIndex.has_value()) {

				LogError(LOG_ERR, "Texture image index has no value for file %s", filePathStr.c_str());
				return false;
			}

			auto& image  = gltf.images[texture.imageIndex.value()];


			stbi_uc* pixels = nullptr;
			int width, height, channels;

			if (!Texture::loadImageFromGLTF(filePath.string(), gltf, image, pixels, width, height, channels))
			{
				LogError(LOG_ERR, "Failed to load image %zu from %s", i, filePathStr.c_str());
				stbi_image_free(pixels);
				continue;
			}

			if (image.name.empty()) {

				LogError(LOG_ERR, "Empty image name in file %s ", filePathStr.c_str());
				LogError(LOG_ERR, "all assets must have unique names within the file, cannot import asset, please add names and try again!");
				return false;
			}

			uint64_t assetID = util::generateAssetID(filePathStr + image.name.c_str());

			TexHeader header = {

					.magic = 0x544558, // 'TEX' for corruptionCheck,
					.width = width,
					.height = height,
					.pitch = width * channels,
					.format = SDL_PIXELFORMAT_ABGR8888,
					.pixelDataSize = static_cast<uint32_t> (width * channels * height),
					.AssetID = assetID,
			};

			fs::path textureDest = destFolder / std::format("{:016x}.tex", assetID);

			//save image to file
			if (!RenderUtil::saveTexToFile(textureDest, header, pixels)) {
				//saveSTBImageToFile aleady logs
				stbi_image_free(pixels);
				continue;
			}

			//free the image data because we're done with it
			stbi_image_free(pixels);

			localToTextureId[i] = assetID;

			uint64_t contentHash = util::generateContentHash(textureDest);
			if (contentHash == 0) {
				LogError(LOG_ERR, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				continue;
			}

			AssetMetadata assetMetaData;
			assetMetaData.contentHash = contentHash;
			assetMetaData.cookedPath = textureDest.generic_string();
			assetMetaData.id = assetID;
			assetMetaData.importedAt = util::Now();
			assetMetaData.type = AssetType::Texture2D;
			assetMetaData.sourcePath = filePathStr; //external textures should have a  different source path
			assetMetaData.sourceHash = gltfFileContentHash;

			manifest.Insert(assetMetaData);
		}

		//Load all Materials based on loaded texture and create a map from gltf to materialRegistry so subMeshes can ref it.
		for (size_t i = 0; i < gltf.materials.size(); i++) {

			auto& material = gltf.materials[i];

			if (material.name.empty()) {
				LogError(LOG_ERR, "Empty Material name in file %s ", filePathStr.c_str());
				LogError(LOG_ERR, "all assets must have unique names within the file, cannot import asset, please add names and try again!");
				return false;
			}

			MaterialData currentMat;

			currentMat.baseColorFactor = glm::vec4(material.pbrData.baseColorFactor.x(),
				material.pbrData.baseColorFactor.y(),
				material.pbrData.baseColorFactor.z(),
				material.pbrData.baseColorFactor.w());

			currentMat.metallicFactor = material.pbrData.metallicFactor;
			currentMat.roughnessFactor = material.pbrData.roughnessFactor;

			if (material.pbrData.baseColorTexture.has_value()) {

				currentMat.baseColorTexID = localToTextureId[material.pbrData.baseColorTexture.value().textureIndex];
			}

			if (material.pbrData.metallicRoughnessTexture.has_value()) {

				currentMat.metallicRoughnessTexID = localToTextureId[material.pbrData.metallicRoughnessTexture.value().textureIndex];
			}

			// TODO the rest of the material data


			uint64_t assetID = util::generateAssetID(filePathStr + material.name.c_str());

			fs::path materialDest = destFolder / std::format("{:016x}.mat", assetID);

			if (!RenderUtil::saveMaterialDataToFile(materialDest, currentMat)) {
				LogError(LOG_ERR, "Failed to save material %zu from %s", i, filePathStr.c_str());
				continue;
			}

			localToMaterialId[i] = assetID;

			uint64_t contentHash = util::generateContentHash(materialDest);
			if (contentHash == 0) {
				LogError(LOG_ERR, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				continue;
			}

			AssetMetadata assetMetaData;
			assetMetaData.contentHash = contentHash;
			assetMetaData.cookedPath = materialDest.generic_string();
			assetMetaData.id = assetID;
			assetMetaData.importedAt = util::Now();
			assetMetaData.type = AssetType::Material;
			assetMetaData.sourcePath = filePathStr;
			assetMetaData.sourceHash = gltfFileContentHash;

			if (currentMat.baseColorTexID != 0)
				assetMetaData.dependencies.push_back(currentMat.baseColorTexID);
			if (currentMat.metallicRoughnessTexID != 0)
				assetMetaData.dependencies.push_back(currentMat.metallicRoughnessTexID);

			manifest.Insert(assetMetaData);
		}

		//Load all meshes.
		for (size_t i = 0; i < gltf.meshes.size(); i++) {

			Mesh currentMesh;

			fastgltf::Mesh& mesh = gltf.meshes.at(i);

			uint16_t subMeshIndex = 0;
			for (auto& primitive : mesh.primitives) {

				if (subMeshIndex >= currentMesh.subMeshes.size()) {
					LogWarn(LOG_ERR, "Mesh %s has more than %d primitives, skipping remainder", mesh.name.c_str(), currentMesh.subMeshes.size());
					break;
				}

				currentMesh.subMeshCount++;

				SubMesh& sub = currentMesh.subMeshes[subMeshIndex++];
				sub.baseVertex = (uint32_t)currentMesh.vertices.size();
				sub.firstIndex = (uint32_t)currentMesh.indices.size();

				//If the subMesh has a material then use localToMaterialRegistry to map it to correct material.
				if (primitive.materialIndex.has_value()) {

					sub.materialID = localToMaterialId[primitive.materialIndex.value()];
				}
				else {

					//If not material set it default material
					sub.materialID = 0;
				}

				// ---- INDICES ----
				if (primitive.indicesAccessor.has_value()) {
					auto& accessor = gltf.accessors[primitive.indicesAccessor.value()];
					sub.indexCount = (uint32_t)accessor.count;
					currentMesh.indices.reserve(currentMesh.indices.size() + accessor.count);
					fastgltf::iterateAccessor<uint32_t>(gltf, accessor, [&](uint32_t index) {
						currentMesh.indices.push_back(index);
					});
				}

				// ---- VERTICES ----
				// positions (required)
				{
					auto* attrib = primitive.findAttribute("POSITION");
					if (attrib == primitive.attributes.end()) {

						LogError(LOG_ERR, "Mesh primitive missing POSITION, skipping");
						return false;
					}
					auto& accessor = gltf.accessors[attrib->accessorIndex];
					sub.vertexCount = (uint32_t)accessor.count;
					currentMesh.vertices.resize(sub.baseVertex + accessor.count);
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(gltf, accessor,
						[&](fastgltf::math::fvec3 pos, size_t i) {
						Vertex& v = currentMesh.vertices[sub.baseVertex + i];
						v.position = glm::vec3(pos.x(), pos.y(), pos.z());
						v.normal = { 0, 1, 0 };
						v.texCoord = { 0, 0 };
					});
				}
				// normals (optional)
				{
					auto* attrib = primitive.findAttribute("NORMAL");
					if (attrib != primitive.attributes.end()) {
						auto& accessor = gltf.accessors[attrib->accessorIndex];
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(gltf, accessor,
							[&](fastgltf::math::fvec3 normal, size_t i) {
							currentMesh.vertices[sub.baseVertex + i].normal =
								glm::vec3(normal.x(), normal.y(), normal.z());
						});
					}
				}
				// uvs (optional)
				{
					auto* attrib = primitive.findAttribute("TEXCOORD_0");
					if (attrib != primitive.attributes.end()) {
						auto& accessor = gltf.accessors[attrib->accessorIndex];
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(gltf, accessor,
							[&](fastgltf::math::fvec2 uv, size_t i) {
							currentMesh.vertices[sub.baseVertex + i].texCoord =
								glm::vec2(uv.x(), uv.y());
						});
					}
				}
			}

			//TODO generate LODs here.

			//save Mesh to file
			
			currentMesh.size = Mesh::calculateMeshSize(currentMesh.vertices);

			if (mesh.name.empty()) {

				LogError(LOG_ERR, "Empty Mesh name in file %s ", filePathStr.c_str());
				LogError(LOG_ERR, "all assets must have unique names within the file, cannot import asset, please add names and try again!");
				return false;
			}

			//todo make sure name is not null
			uint64_t assetID = util::generateAssetID(filePathStr + mesh.name.c_str());

			// --- Build header ---
			MeshHeader header;
			header.vertexCount = (uint32_t)currentMesh.vertices.size();
			header.indexCount = (uint32_t)currentMesh.indices.size();
			header.subMeshCount = currentMesh.subMeshCount;
			header.AssetID = assetID;
			header.size = currentMesh.size;

			fs::path meshDest = destFolder / std::format("{:016x}.mesh", assetID);

			if (!RenderUtil::saveMeshToFile(meshDest, currentMesh, header)) {
				LogError(LOG_ERR, "Failed to save mesh %s", meshDest.generic_string().c_str());
				return false;
			}

			localToMeshId[i] = assetID;

			uint64_t contentHash = util::generateContentHash(meshDest);
			if (contentHash == 0) {
				LogError(LOG_ERR, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				return false;
			}

			AssetMetadata assetMetaData;
			assetMetaData.contentHash = contentHash;
			assetMetaData.cookedPath = meshDest.generic_string();
			assetMetaData.id = assetID;
			assetMetaData.importedAt = util::Now();
			assetMetaData.type = AssetType::Mesh;
			assetMetaData.sourcePath = filePathStr;
			assetMetaData.sourceHash = gltfFileContentHash;

			for (int j = 0; j < currentMesh.subMeshCount; j++) {
				assetMetaData.dependencies.push_back(currentMesh.subMeshes[j].materialID);
			}

			manifest.Insert(assetMetaData);

		}

		//Go through all the scenes
		for (std::size_t i = 0; i < gltf.scenes.size(); i++) {

			std::vector<SceneNodeData> sceneNodeDatalist;

			//Iterate through all scenes and all nodes and process every mesh
			fastgltf::iterateSceneNodes(gltf, i, fastgltf::math::fmat4x4(),
				[&](fastgltf::Node& node, const auto& matrix) {

				//We only care about node with meshes
				if (node.meshIndex.has_value()) {

					SceneNodeData & nodeData = sceneNodeDatalist.emplace_back();

					if (node.name.empty()) {

						LogError(LOG_ERR, "Empty Node name in file %s ", filePathStr.c_str());
						LogError(LOG_ERR, "all assets must have unique names within the file, cannot import asset, please add names and try again!");
						return false;
					}

					nodeData.name = node.name;
					nodeData.meshID = localToMeshId[node.meshIndex.value()];

					Transform& transform = nodeData.transform;
					
					//process the meshes transform
					//we only care about a nodes transform if it has a mesh
					//Do we always care about the transform ???
					auto transformVarient = node.transform;
					if (std::holds_alternative<fastgltf::TRS>(transformVarient)) {

						fastgltf::TRS& trs = get<fastgltf::TRS>(transformVarient);

						transform.position = glm::vec3(trs.translation[0],
							trs.translation[1],
							trs.translation[2]);

						transform.rotation = glm::quat(trs.rotation[3],
							trs.rotation[0],
							trs.rotation[1],
							trs.rotation[2]);

						transform.scale = glm::vec3(trs.scale[0],
							trs.scale[1],
							trs.scale[2]);
					}
					else 
					{

						fastgltf::math::fmat4x4 matrix = get<fastgltf::math::fmat4x4>(transformVarient);

						fastgltf::math::fvec3 translation = fastgltf::math::fvec3();
						fastgltf::math::fquat rotation  = fastgltf::math::fquat();
						fastgltf::math::fvec3 scale = fastgltf::math::fvec3();

						fastgltf::math::decomposeTransformMatrix(matrix, scale, rotation, translation);

						transform.position = glm::vec3(translation[0], translation[1], translation[2]);
						transform.rotation = glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]);
						transform.scale = glm::vec3(scale[0], scale[1], scale[2]);

					}
				}

			});


			if (gltf.scenes[i].name.empty()) {

				LogError(LOG_ERR, "Empty Scene name in file %s ", filePathStr.c_str());
				LogError(LOG_ERR, "all assets must have unique names within the file, cannot import asset, please add names and try again!");
				return false;
			}

			uint64_t assetID = util::generateAssetID(filePathStr + gltf.scenes[i].name.c_str());

			SceneHeader sceneHeader;
			sceneHeader.assetID = assetID;
			sceneHeader.nodesNum = sceneNodeDatalist.size();

			fs::path sceneDestPath = destFolder / std::format("{:016x}.scene", assetID);

			if (!RenderUtil::saveSceneDataFile(sceneDestPath, sceneHeader, sceneNodeDatalist)) {

				return false;
			}

			uint64_t SceneContentHash = util::generateContentHash(sceneDestPath);
			if (SceneContentHash == 0) {
				LogError(LOG_ERR, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				continue;
			}


			AssetMetadata assetMetaData;
			assetMetaData.contentHash = SceneContentHash;
			assetMetaData.cookedPath = sceneDestPath.generic_string();
			assetMetaData.id = assetID;
			assetMetaData.importedAt = util::Now();
			assetMetaData.type = AssetType::Scene;
			assetMetaData.sourcePath = filePathStr;
			assetMetaData.sourceHash = gltfFileContentHash;

			manifest.Insert(assetMetaData);


		}

		manifest.Save();

		return true;
	}

	//TODO
	static bool reImportGLTF(const fs::path& filePath, const fs::path& destFolder, Manifest& manifest)
	{
		return true;
	}

}
