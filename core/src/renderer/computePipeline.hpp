#pragma once

#include "renderUtil.hpp"

class ComputePipeline {

public:

	SDL_GPUComputePipeline* pipeline = NULL;

	SDL_GPUShader* computeShader = NULL;



	ComputePipeline() {};


	//TODO get shader 
	bool createPipeline(flecs::world& ecs, const char* name, const char* shaderFilePath,
		uint32_t num_samplers,
		uint32_t num_readonly_storage_textures,
		uint32_t num_readonly_storage_buffers,
		uint32_t num_readwrite_storage_textures,
		uint32_t num_readwrite_storage_buffers,
		uint32_t num_uniform_buffers,
		uint32_t threadcount_x,
		uint32_t threadcount_y,
		uint32_t threadcount_z ) {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RendererConfig& renderConfig = ecs.get<RendererConfig>();

		std::vector<Uint8> spirvCode = RenderUtil ::loadBinaryFile(shaderFilePath);
		if (spirvCode.empty()) {
			std::cerr << "Failed to load compute shader: " << shaderFilePath << std::endl;
			return false;
		}

		SDL_GPUComputePipelineCreateInfo createInfo = {
		
		.code_size = spirvCode.size(),
		.code = spirvCode.data(),
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		 .num_samplers = num_samplers,
		 .num_readonly_storage_textures = num_readonly_storage_textures,
		 .num_readonly_storage_buffers = num_readonly_storage_buffers,
		 .num_readwrite_storage_textures = num_readwrite_storage_textures,
		 .num_readwrite_storage_buffers = num_readwrite_storage_buffers,
		 .num_uniform_buffers = num_uniform_buffers,
		.threadcount_x = threadcount_x,
		.threadcount_y = threadcount_y,
		.threadcount_z = threadcount_z,
		};
		
		SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(renderContext.device, &createInfo);
		if (pipeline == NULL)
		{
			SDL_Log("Failed to create compute pipeline!");
			return false;
		}

		return true;
	}
};