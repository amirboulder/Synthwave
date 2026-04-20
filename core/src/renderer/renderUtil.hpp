#pragma once

struct Context {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;

	SDL_GPUSampleCount sampleCountMSAA;
};

struct RenderContext {
	SDL_GPUDevice* device = NULL;
	SDL_Window* window = NULL;
};

struct FrameContext {
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;
};



struct FrameDataUniforms {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
	glm::vec3 cameraPos;
	float _pad;
};

struct PerModelUniforms {
    glm::mat4 model;
    glm::mat4 mvp;
};

class RenderUtil {


public:

	static void uploadBufferData(SDL_GPUDevice* device, SDL_GPUBuffer* & buffer, const void* data, size_t size, Uint32 SDL_GPUBufferUsageFlag) {

        //create buffer
        SDL_GPUBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.usage = SDL_GPUBufferUsageFlag;
        bufferCreateInfo.size = size;

        buffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
        if (!buffer) {
            LogError(LOG_RENDER, "Failed to create buffer of type: %d  %s", SDL_GPUBufferUsageFlag, SDL_GetError());
            return ;
        }

        // Create transfer buffer
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer) {

			LogError(LOG_RENDER, "Failed to create transfer buffer: %s", SDL_GetError());
            return;
        }

        // Map and copy data
        void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mappedData) {

			LogError(LOG_RENDER, "Failed to map transfer buffer: %s", SDL_GetError());
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

            return;
        }

        memcpy(mappedData, data, size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        // Upload to GPU
        SDL_GPUCommandBuffer* cmdBuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuf);

        SDL_GPUTransferBufferLocation srcLoc = { transferBuffer, 0 };
        SDL_GPUBufferRegion dstRegion = { buffer, 0, size };
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstRegion, false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(cmdBuf);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
    }


    // Helper function to load binary file data
    static std::vector<Uint8> loadBinaryFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {

			LogError(LOG_RENDER, "Failed to open file: %s", filename.c_str());
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<Uint8> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {

			LogError(LOG_RENDER, "Failed to read file: %s", filename.c_str());
            return {};
        }

        return buffer;
    }

	static SDL_Surface* LoadImage(const char* path, int desiredChannels)
	{
		SDL_Surface* result;
		SDL_PixelFormat format;

		result = SDL_LoadBMP(path);
		if (result == NULL)
		{
			LogError(LOG_RENDER, "Failed to load BMP: %s", SDL_GetError());
			return NULL;
		}

		if (desiredChannels == 4)
		{
			format = SDL_PIXELFORMAT_ABGR8888;
		}
		else
		{
			SDL_assert(!"Unexpected desiredChannels");
			SDL_DestroySurface(result);
			return NULL;
		}
		if (result->format != format)
		{
			SDL_Surface* next = SDL_ConvertSurface(result, format);
			SDL_DestroySurface(result);
			result = next;
		}

		return result;
	}


	static bool loadShaderSPIRV(
		SDL_GPUDevice* device,
		SDL_GPUShader*& shader,
		const std::string& filename,
		SDL_GPUShaderStage stage,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{

		std::vector<Uint8> spirvCode = RenderUtil::loadBinaryFile(filename);
		if (spirvCode.empty()) {
			LogError(LOG_RENDER, "Failed to load shader %s", filename.c_str());
			return false;
		}

		SDL_GPUShaderCreateInfo createInfo = {};
		createInfo.num_samplers = sampler_count;
		createInfo.num_storage_buffers = storage_buffer_count;
		createInfo.num_storage_textures = storage_texture_count;
		createInfo.num_uniform_buffers = uniform_buffer_count;
		createInfo.props = 0;
		createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createInfo.code = spirvCode.data();
		createInfo.code_size = spirvCode.size();
		createInfo.entrypoint = "main";
		createInfo.stage = stage;

		shader = SDL_CreateGPUShader(device, &createInfo);

		std::string stageName = " ";

		if (stage == SDL_GPU_SHADERSTAGE_VERTEX) {
			stageName = "vertex";
		}
		else if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT) {
			stageName = "fragment";
		}

		if (shader == NULL) {
			LogError(LOG_RENDER, "Failed to create SDL_GPU %s shader form file ", stageName.c_str(), filename.c_str());

			SDL_ReleaseGPUShader(device, shader);
			return false;
		}
	}

	// 1x1 white pixel — lets lighting still show correctly
	// 0xFF000000 for black, or 0xFFFF00FF for obvious missing texture magenta.
    static SDL_GPUTexture* createColorTexture(SDL_GPUDevice* device, uint32_t color) {
       

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        texInfo.width = 1;
        texInfo.height = 1;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        SDL_GPUTexture* tex = SDL_CreateGPUTexture(device, &texInfo);

        // Upload the pixel
        SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
		transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferBufferInfo.size = sizeof(uint32_t);
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

        void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        memcpy(mapped, &color, sizeof(color));
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = transferBuffer,
			.offset = 0,
		};

		SDL_GPUTextureRegion textureRegion = {
			.texture = tex,
			.w = 1,
			.h = 1,
			.d = 1

		};


		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);


		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return tex;
    }
};