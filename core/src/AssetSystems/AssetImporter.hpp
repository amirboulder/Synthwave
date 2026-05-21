#pragma once

#include "Manifest.hpp"


namespace AssetImporter {


	/// <summary>
	/// processes texture, materials, meshes in a glTF file,
	/// assigns IDs to each and save the processed data to their respective files.
	/// inserts AssetMetadata into the manifest
	/// </summary>
	static bool ImportGLTF(const fs::path& filePath, const fs::path& destFolder, Manifest& manifest)
	{
		//Used to map local texture/materials to their AssetIDs,
		//materials store AssetIDs of textures,
		// Meshes store AssetIDs of materials.
		std::unordered_map<uint32_t, uint64_t> localToMaterialId;
		std::unordered_map<uint32_t, uint64_t> localToTextureId;

		const std::string filePathStr = filePath.string().c_str();

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

		LogDebug(LOG_RENDER, "Info for 3D Asset file %s", filePath.string().c_str());
		LogDebug(LOG_RENDER, "Number of meshes : %d", gltf.meshes.size());
		LogDebug(LOG_RENDER, "Number of Materials : %d", gltf.materials.size());
		LogDebug(LOG_RENDER, "Number of Textures : %d", gltf.textures.size());
		LogDebug(LOG_RENDER, "Number of Images : %d", gltf.images.size());
		LogDebug(LOG_RENDER, "Number of Nodes : %d", gltf.nodes.size());
		LogDebug(LOG_RENDER, "Number of Scenes : %d", gltf.scenes.size());


		uint64_t assetID = util::generateAssetID(filePathStr);

		uint64_t gltfFileContentHash = util::generateContentHash(filePath);

		if (gltfFileContentHash == 0) {
			LogError(LOG_RENDER, "Failed to generate Content Hash for file %s", filePathStr.c_str());
			return false;
		}

		AssetMetadata assetMetaData;
		assetMetaData.contentHash = gltfFileContentHash;
		assetMetaData.cookedPath = " ";
		assetMetaData.id = assetID;
		assetMetaData.importedAt = util::Now();
		assetMetaData.type = AssetType::Scene;
		assetMetaData.sourcePath = filePathStr;
		assetMetaData.sourceHash = gltfFileContentHash;

		manifest.Insert(assetMetaData);



		for (size_t i = 0; i < gltf.images.size(); i++) {
			auto& image = gltf.images[i];


			stbi_uc* pixels = nullptr;
			int width, height, channels;

			if (!Texture::loadImageFromGLTF(filePath.string(), gltf, image, pixels, width, height, channels))
			{
				LogError(LOG_RENDER, "Failed to load image %zu from %s", i, filePathStr.c_str());
				stbi_image_free(pixels);
				continue;
			}

			uint64_t assetID = util::generateAssetID(filePathStr + image.name.c_str());

			TexHeader header = {

					.corruptionCheck = 0x54455831, // 'TEX1' corruptionCheck,
					.width = width,
					.height = height,
					.pitch = width * channels,
					.format = SDL_PIXELFORMAT_ABGR8888,
					.pixelDataSize = width * channels * height,
					.AssetID = assetID,
			};

			fs::path textureDest = destFolder / std::format("{:016x}.tex", assetID);

			//save image to file
			if (!RenderUtil::saveSTBImageToFile(textureDest, header, pixels)) {
				//saveSTBImageToFile aleady logs
				stbi_image_free(pixels);
				continue;
			}

			//free the image data because we're done with it
			stbi_image_free(pixels);

			localToTextureId[i] = assetID;

			uint64_t contentHash = util::generateContentHash(textureDest);
			if (contentHash == 0) {
				LogError(LOG_RENDER, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				continue;
			}

			AssetMetadata assetMetaData;
			assetMetaData.contentHash = contentHash;
			assetMetaData.cookedPath = textureDest.string();
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
				LogError(LOG_RENDER, "Failed to save material %zu from %s", i, filePathStr.c_str());
				continue;
			}

			localToMaterialId[i] = assetID;

			uint64_t contentHash = util::generateContentHash(materialDest);
			if (contentHash == 0) {
				LogError(LOG_RENDER, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
				continue;
			}

			AssetMetadata assetMetaData;
			assetMetaData.contentHash = contentHash;
			assetMetaData.cookedPath = materialDest.string();
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


		//Go through all the scenes
		for (std::size_t i = 0; i < gltf.scenes.size(); i++) {


			//Iterate through all scenes and all nodes and process every mesh
			fastgltf::iterateSceneNodes(gltf, i, fastgltf::math::fmat4x4(),
				[&](fastgltf::Node& node, const auto& matrix) {


				if (node.meshIndex.has_value()) {

					Mesh currentMesh;

					fastgltf::Mesh& mesh = gltf.meshes.at(node.meshIndex.value());

					uint16_t subMeshIndex = 0;
					for (auto& primitive : mesh.primitives) {

						if (subMeshIndex >= currentMesh.subMeshes.size()) {
							LogWarn(LOG_RENDER, "Mesh %s has more than %d primitives, skipping remainder", mesh.name.c_str(), currentMesh.subMeshes.size());
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

								LogWarn(LOG_RENDER, "Mesh primitive missing POSITION, skipping");
								return;
							}
							auto& accessor = gltf.accessors[attrib->accessorIndex];
							sub.vertexCount = (uint32_t)accessor.count;
							currentMesh.vertices.resize(sub.baseVertex + accessor.count);
							fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(gltf, accessor,
								[&](fastgltf::math::fvec3 pos, size_t i) {
								Vertex& v = currentMesh.vertices[sub.baseVertex + i];
								v.position = glm::vec3(pos.x(), -pos.y(), pos.z());
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
										glm::vec3(normal.x(), -normal.y(), normal.z());
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


					//process the meshes transform
					//we only care about a nodes transform if it has a mesh
					//Do we always care about the transfrom ???
					auto transformVarient = node.transform;
					if (std::holds_alternative<fastgltf::TRS>(transformVarient)) {

						fastgltf::TRS& trs = get<fastgltf::TRS>(transformVarient);

						currentMesh.transform.position = glm::vec3(trs.translation[0],
							trs.translation[1],
							trs.translation[2]);

						currentMesh.transform.rotation = glm::quat(trs.rotation[3],
							trs.rotation[0],
							trs.rotation[1],
							trs.rotation[2]);

						currentMesh.transform.scale = glm::vec3(trs.scale[0],
							trs.scale[1],
							trs.scale[2]);
					}
					else {

						cout << "TODO handle mat4 case " << '\n';
					}

					//todo make sure name is not null
					uint64_t assetID = util::generateAssetID(filePathStr + mesh.name.c_str());

					// --- Build header ---
					MeshHeader header;
					header.vertexCount = (uint32_t)currentMesh.vertices.size();
					header.indexCount = (uint32_t)currentMesh.indices.size();
					header.subMeshCount = currentMesh.subMeshCount;
					header.AssetID = assetID;

					fs::path meshDest = destFolder / std::format("{:016x}.mesh", assetID);

					RenderUtil::saveMeshToFile(meshDest, currentMesh, header);


					uint64_t contentHash = util::generateContentHash(meshDest);
					if (contentHash == 0) {
						LogError(LOG_RENDER, "Failed to generate Content Hash for asset %zu in %s", i, filePathStr.c_str());
						return;
					}

					AssetMetadata assetMetaData;
					assetMetaData.contentHash = contentHash;
					assetMetaData.cookedPath = meshDest.string();
					assetMetaData.id = assetID;
					assetMetaData.importedAt = util::Now();
					assetMetaData.type = AssetType::Mesh;
					assetMetaData.sourcePath = filePathStr;
					assetMetaData.sourceHash = gltfFileContentHash;

					for (int i = 0; i < currentMesh.subMeshCount; i++) {
						assetMetaData.dependencies.push_back(currentMesh.subMeshes[i].materialID);
					}

					manifest.Insert(assetMetaData);
				}


			});
		}

		return true;
	}

	//TODO
	static bool reImportGLTF(const fs::path& filePath, const fs::path& destFolder, Manifest& manifest)
	{

	}

}

