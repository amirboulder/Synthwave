#pragma once

#include "core/src/pch.h"

#include "renderUtil.hpp"
 
struct MaterialSMPL {
    SDL_GPUTexture* diffuseTexture;
};

class MaterialLoader {
public:

	SDL_PixelFormat targetFormat = SDL_PIXELFORMAT_ABGR8888;

    void loadMaterials(const aiScene* scene, std::vector<MaterialSMPL> & materials, const std::string& modelPath, SDL_GPUDevice* device) {

        std::string directory = std::filesystem::path(modelPath).parent_path().string();

        materials.reserve(scene->mNumMaterials);

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {

            aiMaterial* aiMat = scene->mMaterials[i];

            materials.emplace_back();

			MaterialSMPL& mat = materials[i];

            // The texture types we are using right now
			//TODO add more texture types
            std::vector<aiTextureType> textureTypes = {
                aiTextureType_DIFFUSE,
                /*            aiTextureType_SPECULAR,
                            aiTextureType_AMBIENT,
                            aiTextureType_EMISSIVE,
                            aiTextureType_HEIGHT,
                            aiTextureType_NORMALS*/
            };

            // for each texture type get the first of that texture from material
            // TODO handle more than one of each texture type
            for (aiTextureType type : textureTypes) {

                aiString str;
                if (aiMat->GetTexture(type, 0, &str) == AI_SUCCESS) {

                    //Embedded texture
                    if (str.data[0] == '*') {

						int index = atoi(&str.data[1]);
						aiTexture* texture = scene->mTextures[index];
						loadEmbededTexture(texture, materials[i].diffuseTexture, "Diffuse Texture", device);

                    }
                    //External texture
                    else {
                        std::string texturePath = directory + "/" + std::string(str.C_Str());
						loadExternalTexture(texturePath, materials[i].diffuseTexture, "Diffuse Texture", device);

                    }
                }
            }
        }
    }


    void loadEmbededTexture(aiTexture* textureImported, SDL_GPUTexture* & TextureSDL,const std::string textureName, SDL_GPUDevice* device) {

		SDL_Surface* imageData;

		// Check if it's compressed (like PNG, JPG)
		if (textureImported->mHeight == 0) {
			// Compressed texture - load from memory
			std::string extension = std::string(textureImported->achFormatHint);
			if (extension.empty()) {
				extension = "png"; // Default
			}

			// Create SDL_IOStream from memory
			SDL_IOStream* rw = SDL_IOFromMem(textureImported->pcData, textureImported->mWidth);
			if (!rw) {
				SDL_Log("Failed to create SDL_IOStream for embedded texture %s", SDL_GetError());
				return;
			}

			// Load image from memory stream
			imageData = IMG_Load_IO(rw, true); // SDL_TRUE means close the stream when done
			if (imageData == NULL) {
				SDL_Log("Failed to load embedded texture(%s): %s", extension.c_str(), SDL_GetError());
				return;
			}

			/*SDL_Log("Loaded embedded texture : %dx%d, format: %s",
				imageData->w, imageData->h, extension.c_str());*/

			// Convert to desired format if necessary
			if (imageData->format != targetFormat) {
				SDL_Surface* converted = SDL_ConvertSurface(imageData, targetFormat);
				SDL_DestroySurface(imageData);
				if (converted == NULL) {
					SDL_Log("Failed to convert embedded texture format: %s", SDL_GetError());
					return;
				}
				imageData = converted;
			}


			createGPUTexture(imageData, TextureSDL, textureName, device);

		}
		else {
			SDL_Log("embedded texture format issue!");
		}

    }

    void loadExternalTexture(const std::string texturePath, SDL_GPUTexture* & TextureSDL, const std::string textureName, SDL_GPUDevice* device) {

        // Load external texture file
        SDL_Surface* imageData = IMG_Load(texturePath.c_str());
        if (imageData == NULL) {
            SDL_Log("Failed to load external texture '%s': %s", texturePath.c_str(), SDL_GetError());
            return;
        }

		// Convert format if needed
		if (imageData->format != targetFormat) {
			SDL_Surface* converted = SDL_ConvertSurface(imageData, targetFormat);
			SDL_DestroySurface(imageData);
			if (converted == NULL) {
				SDL_Log("Failed to convert external texture format: %s", SDL_GetError());
				return;
			}
			imageData = converted;
		}

		createGPUTexture(imageData, TextureSDL, textureName, device);

    }

	void createGPUTexture(SDL_Surface* imageData, SDL_GPUTexture* & TextureSDL, const std::string textureName, SDL_GPUDevice* device) {

		// Set up texture data
		const Uint32 imageSizeInBytes = imageData->w * imageData->h * 4;

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

};