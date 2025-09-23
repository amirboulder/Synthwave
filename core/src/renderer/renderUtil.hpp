#pragma once

#include <SDL3/SDL_gpu.h>

struct Context {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;
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

};