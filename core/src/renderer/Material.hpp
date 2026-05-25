#pragma once


struct Material {
	uint32_t baseColorTexIndex = 0;  // index into a flat texture array
	uint32_t normalTexIndex = 0;
	uint32_t metallicRoughnessTexIndex = 0; // GLTF packs these into one texture (G=roughness, B=metallic)
	glm::vec4 baseColorFactor = { 1,1,1,1 };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
};

struct MaterialData {
	uint64_t baseColorTexID = 0;  
	uint64_t normalTexID = 0;
	uint64_t metallicRoughnessTexID = 0; // GLTF packs these into one texture (G=roughness, B=metallic)
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
