#pragma once

#include "renderUtil.hpp"

class Pipeline {

public:

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;

	Pipeline() {};


	/// create pipeline for Struct Vertex
	bool createPipeline(flecs::world& ecs,const char * name, bool line = false)
	{
		//Check vertex and fragment shader not being null
		if (!vertexShader) {
			SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Vertex Shader is NULL unable to create pipeline");
		}
		if (!fragmentShader) {
			SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Fragment Shader is NULL unable to create pipeline");
		}


		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RendererConfig& renderConfig = ecs.get<RendererConfig>();

		// Vertex input state
		SDL_GPUVertexAttribute vertexAttributes[4] = {};

		// Position attribute
		vertexAttributes[0].location = 0;
		vertexAttributes[0].buffer_slot = 0;
		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[0].offset = 0;

		// normal attribute
		vertexAttributes[1].location = 1;
		vertexAttributes[1].buffer_slot = 0;
		vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[1].offset = sizeof(glm::vec3);

		// texture Coordinates attribute
		vertexAttributes[2].location = 2;
		vertexAttributes[2].buffer_slot = 0;
		vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		vertexAttributes[2].offset = sizeof(glm::vec3) + sizeof(glm::vec3);

		// Color attribute
		vertexAttributes[3].location = 3;
		vertexAttributes[3].buffer_slot = 0;
		vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		vertexAttributes[3].offset = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2);

		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
		vertexBufferDescription.slot = 0;
		vertexBufferDescription.pitch = sizeof(Vertex);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 4;


		// Create pipeline
		SDL_GPUColorTargetDescription coldescs = {};
		coldescs.format = SDL_GetGPUSwapchainTextureFormat(renderContext.device, renderContext.window);

		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);
		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		pipeInfo.target_info.color_target_descriptions = &coldescs;
		pipeInfo.target_info.num_color_targets = 1;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		pipeInfo.depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_LESS,
		.enable_depth_test = true,
		.enable_depth_write = true,
		};

		//MSAA
		pipeInfo.multisample_state = {
			.sample_count = renderConfig.sampleCountMSAA,  // Enable MSAA in pipeline
			.sample_mask = 0  // Use all samples
		};

		pipeInfo.vertex_input_state = vertexInputState;

		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create fill pipeline: %s", SDL_GetError());
			return false;
		}

		if (line) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
			pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
			if (!pipeline) {
				SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create line pipeline: %s", SDL_GetError());
				return false;
			}

		}

		SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,"Built pipeline %s", name);

		return true;

	}
};


