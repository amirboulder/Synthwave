#pragma once


struct Material {
	int32_t baseColorTexIndex = -1;  // index into a flat texture array
	int32_t normalTexIndex = -1;
	int32_t metallicRoughnessTexIndex = -1; // GLTF packs these into one texture (G=roughness, B=metallic)
	glm::vec4 baseColorFactor = { 1,1,1,1 };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
};

struct MaterialData {
	int64_t baseColorTexID = 0;  
	int64_t normalTexID = 0;
	int64_t metallicRoughnessTexID = 0; // GLTF packs these into one texture (G=roughness, B=metallic)
	glm::vec4 baseColorFactor = { 1,1,1,1 };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
};


enum class TextureMapType {

	BaseColor,
	Normal,
	metallicRoughness,
};

struct TextureArrays {

	TextureArray diffuseTextures;
	TextureArray normalTextures;
	TextureArray metallicRoughnessTextures;

	void init(SDL_GPUDevice* device) {

		diffuseTextures.init(device);
		normalTextures.init(device);
		metallicRoughnessTextures.init(device);
	}
};
