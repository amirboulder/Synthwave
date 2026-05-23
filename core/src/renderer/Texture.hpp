#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct TexHeader {
	uint32_t magic = 0x54455820;    //'TEX' for format validation on load
	int32_t  width;
	int32_t  height;
	int32_t  pitch;      // bytes per row (may have padding)
	uint32_t format;     // SDL_PixelFormat enum value
	uint32_t pixelDataSize;
	uint64_t AssetID;        
};

struct TextureArray {

	SDL_GPUTexture* textureArray = nullptr;
	uint32_t usedLayers = 0;
	uint32_t maxLayers = 0;

	void init(SDL_GPUDevice* device, uint32_t numLayers = 32) {

		this->maxLayers = numLayers;

		SDL_GPUTextureCreateInfo info{};
		info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
		info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		info.width = 1024;  // all layers MUST match
		info.height = 1024;  // all layers MUST match
		info.layer_count_or_depth = numLayers;  // max number of textures
		info.num_levels = 1;
		info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

		textureArray = SDL_CreateGPUTexture(device, &info);
	}
};

namespace Texture {

	bool loadImageFromGLTF(std::string_view filename, fastgltf::Asset& asset, fastgltf::Image& image, stbi_uc*& pixels, int& width, int& height, int& channels)
	{

		std::visit(fastgltf::visitor{
			// 1. EXTERNAL — just a URI filepath
			[&](const fastgltf::sources::URI& uri) {
			// uri.uri.path() gives you the relative file path
			// Load it yourself from disk

			pixels = stbi_load(uri.uri.path().data(), &width, &height, &channels, 4);
		},

			// 2. EMBEDDED in .glb — binary buffer view
			[&](const fastgltf::sources::BufferView& bufferView) {
				auto& view = asset.bufferViews[bufferView.bufferViewIndex];
				auto& buffer = asset.buffers[view.bufferIndex];

				// The buffer itself is also a variant
				auto* array = std::get_if<fastgltf::sources::Array>(&buffer.data);
				if (array) {
					const unsigned char* dataPtr =
						reinterpret_cast<const unsigned char*>(array->bytes.data()) + view.byteOffset;

					pixels = stbi_load_from_memory(dataPtr, static_cast<int>(view.byteLength),
										  &width, &height, &channels, 4);
				}
			},

			// 3. EMBEDDED in .gltf — base64 data URI, already decoded by fastgltf
			[&](const fastgltf::sources::Array& array) {

				pixels = stbi_load_from_memory(
					reinterpret_cast<const unsigned char*>(array.bytes.data()),
					static_cast<int>(array.bytes.size()),
					&width, &height, &channels, 4);
			},

			// 4. Fallback — shouldn't normally hit this
			[&](const auto&) {

			LogError(LOG_RENDER,"Hit Fallback in loadImage while loading %s ", filename.data());
			}

			}, image.data);

		return true;
	}

	static void createSDLGPUTexture(SDL_Surface* imageData, SDL_GPUTexture*& TextureSDL, const std::string textureName, SDL_GPUDevice* device) {

		// Set up texture data
		const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4; //This assumes rgba maybe add this as a parameter

		SDL_GPUTextureCreateInfo textureCreateInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width = static_cast<Uint32>(imageData->w),
			.height = static_cast<Uint32>(imageData->h),
			.layer_count_or_depth = 1,
			.num_levels = 1,

		};
		TextureSDL = SDL_CreateGPUTexture(device, &textureCreateInfo);

		if (!TextureSDL) {
			SDL_Log("Could not create GPU texture");
			return;
		}

		SDL_SetGPUTextureName(
			device,
			TextureSDL,
			textureName.c_str()
		);

		// Set up buffer data
		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = imageSizeInBytes
		};
		SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

		void* textureTransferPtr = SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false);

		SDL_memcpy(textureTransferPtr, imageData->pixels, imageSizeInBytes);

		SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

		// Upload the transfer data to the GPU resources
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);



		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = textureTransferBuffer,
			.offset = 0,
		};

		SDL_GPUTextureRegion textureRegion = {
			.texture = TextureSDL,
			.w = static_cast<Uint32>(imageData->w),
			.h = static_cast<Uint32>(imageData->h),
			.d = 1

		};


		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);

		SDL_DestroySurface(imageData);

		SDL_ReleaseGPUTransferBuffer(device, textureTransferBuffer);

	}

}





