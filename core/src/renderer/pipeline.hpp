#pragma once

#include "renderUtil.hpp"

/// <summary>
/// Pipeline class is a wrapper around SDL_GPUGraphicsPipeline and is responsible for creating graphics pipelines.
/// Some PipelineTypes needs both singleSampled and multiSampled versions,
/// This is because the main render pass may of may not use MSAA or a pipeline may be used outside of the main render pass as well.
/// </summary>
class Pipeline {

public:

	SDL_GPUGraphicsPipeline* pipeline = NULL; //singleSampled

	//Maybe deprecate I'm not sure if we need  singleSampled and multiSampled at the same time but we might
	SDL_GPUGraphicsPipeline* pipelineMS = NULL; //multiSampled

	SDL_GPUShader* vertexShader = nullptr;
	SDL_GPUShader* fragmentShader = nullptr;

	std::string name;

	//Deprecate
	//PipelineType type = PipelineType::NONE;

	Pipeline() {};

	bool createPipeline(flecs::world& ecs, const std::string& pipelineName,
		const ShaderReflectionData& reflectionData)
	{
		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig& renderConfig = ecs.get<RenderConfig>();

		name = pipelineName;

		if (!RenderUtil::loadShaderSPIRV(renderContext.device, vertexShader,
			reflectionData.reflectionVS.outputFile, SDL_GPU_SHADERSTAGE_VERTEX,
			reflectionData.reflectionVS.numSamplers,
			reflectionData.reflectionVS.numUniformBuffers,
			reflectionData.reflectionVS.numStorageBuffers,
			reflectionData.reflectionVS.numStorageTextures))
		{
			return false;
		}

		if (!RenderUtil::loadShaderSPIRV(renderContext.device, fragmentShader,
			reflectionData.reflectionFS.outputFile, SDL_GPU_SHADERSTAGE_FRAGMENT,
			reflectionData.reflectionFS.numSamplers,
			reflectionData.reflectionFS.numUniformBuffers,
			reflectionData.reflectionFS.numStorageBuffers,
			reflectionData.reflectionFS.numStorageTextures))
		{
			
			return false;
		}

		// Build color targets locally
		std::vector<SDL_GPUColorTargetDescription> colorTargetDescs;
		colorTargetDescs.reserve(reflectionData.colorTargets.size());
		for (SDL_GPUTextureFormat fmt : reflectionData.colorTargets)
		{
			SDL_GPUColorTargetDescription desc{};
			desc.format = fmt;
			desc.blend_state = reflectionData.blendState;
			colorTargetDescs.push_back(desc);
		}

		// Vertex buffer description
		SDL_GPUVertexBufferDescription vertexBufferDesc{};
		vertexBufferDesc.slot = 0;
		vertexBufferDesc.pitch = reflectionData.vertexStride;
		vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		// Build pipelineCreateInfo fully on the stack
		SDL_GPUGraphicsPipelineCreateInfo pipeInfo{};
		SDL_zero(pipeInfo);

		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = reflectionData.primitiveType;

		pipeInfo.rasterizer_state.fill_mode = reflectionData.fillMode;
		pipeInfo.rasterizer_state.cull_mode = reflectionData.cullMode;
		pipeInfo.rasterizer_state.front_face = reflectionData.frontFace;

		pipeInfo.depth_stencil_state.compare_op = reflectionData.depthCompareOp;
		pipeInfo.depth_stencil_state.enable_depth_test = reflectionData.depthTestEnabled;
		pipeInfo.depth_stencil_state.enable_depth_write = reflectionData.depthWriteEnabled;
		pipeInfo.depth_stencil_state.enable_stencil_test = reflectionData.enableStencilTest;

		pipeInfo.target_info.color_target_descriptions = colorTargetDescs.data();
		pipeInfo.target_info.num_color_targets = static_cast<uint32_t>(colorTargetDescs.size());
		pipeInfo.target_info.depth_stencil_format = reflectionData.depthFormat;
		pipeInfo.target_info.has_depth_stencil_target = true;

		pipeInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;
		pipeInfo.vertex_input_state.num_vertex_buffers = 1;
		pipeInfo.vertex_input_state.vertex_attributes = reflectionData.vertexAttributes.data();
		pipeInfo.vertex_input_state.num_vertex_attributes = static_cast<uint32_t>(reflectionData.vertexAttributes.size());

		// Single sampled pipeline always created
		pipeInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
		pipeInfo.multisample_state.sample_mask = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
		if (!pipeline)
		{
			LogError(LOG_RENDER, "Failed to create pipeline %s: %s", pipelineName.c_str(), SDL_GetError());
			SDL_ReleaseGPUShader(renderContext.device, vertexShader);
			SDL_ReleaseGPUShader(renderContext.device, fragmentShader);
			return false;
		}

		// MSAA pipeline created only if shader supports it and device sample count > 1
		if (reflectionData.msaaCapable && renderConfig.sampleCount > SDL_GPU_SAMPLECOUNT_1)
		{
			pipeInfo.multisample_state.sample_count = renderConfig.sampleCount;
			pipelineMS = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
			if (!pipelineMS)
			{
				LogError(LOG_RENDER, "Failed to create MSAA pipeline %s: %s", pipelineName.c_str(), SDL_GetError());
				// Non-fatal — single sampled pipeline still valid
			}
		}

		SDL_ReleaseGPUShader(renderContext.device, vertexShader);
		SDL_ReleaseGPUShader(renderContext.device, fragmentShader);

		LogDebug(LOG_RENDER, "Created pipeline %s", pipelineName.c_str());
		return true;
	}

};


