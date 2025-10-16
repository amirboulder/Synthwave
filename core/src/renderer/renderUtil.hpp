#pragma once

#include <SDL3/SDL_gpu.h>

struct Context {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;

	SDL_GPUSampleCount sampleCountMSAA;
};

struct RenderConxtext {
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
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create buffer of type: %d  %s", SDL_GPUBufferUsageFlag, SDL_GetError());
            return ;
        }

        // Create transfer buffer
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create transfer buffer: %s", SDL_GetError());
            return;
        }

        // Map and copy data
        void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mappedData) {

            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to map transfer buffer: %s", SDL_GetError());

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
            std::cerr << "Failed to open file: " << filename << std::endl;
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<Uint8> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            std::cerr << "Failed to read file: " << filename << std::endl;
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
			SDL_Log("Failed to load BMP: %s", SDL_GetError());
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


	static bool loadShaderSPRIV(
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
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load shader ");
			return false;
		}

		SDL_GPUShaderCreateInfo createinfo = {};
		createinfo.num_samplers = sampler_count;
		createinfo.num_storage_buffers = storage_buffer_count;
		createinfo.num_storage_textures = storage_texture_count;
		createinfo.num_uniform_buffers = uniform_buffer_count;
		createinfo.props = 0;
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = spirvCode.data();
		createinfo.code_size = spirvCode.size();
		createinfo.entrypoint = "main";
		createinfo.stage = stage;

		shader = SDL_CreateGPUShader(device, &createinfo);

		if (shader == NULL) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Failed to create fragment shader!");
			return false;
		}
	}
};