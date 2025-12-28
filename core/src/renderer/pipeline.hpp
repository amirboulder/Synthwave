#pragma once

#include "renderUtil.hpp"

//TODO reduce code duplication here.

class Pipeline {

public:

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;

	PipelineType type = PipelineType::Vertex;

	Pipeline() {};


	// create pipeline for Struct Vertex
	bool createPipeline(flecs::world& ecs,const char * name, SDL_GPUSampleCount sampleCount, bool line = false)
	{
		//Check vertex and fragment shader not being null
		if (!vertexShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vertex Shader is NULL unable to create pipeline");
			return false;
		}
		if (!fragmentShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Fragment Shader is NULL unable to create pipeline");
			return false;
		}


		const RenderContext& renderContext = ecs.get<RenderContext>();

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

		//Two color Targets,one for the color and one entID
		const uint32_t colorTargetSize = 1;
		SDL_GPUColorTargetDescription colorTargets[colorTargetSize]{};
		
		colorTargets[0].format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);
		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		pipeInfo.target_info.color_target_descriptions = colorTargets;
		pipeInfo.target_info.num_color_targets = colorTargetSize;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		pipeInfo.depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_LESS,
		.enable_depth_test = true,
		.enable_depth_write = true,
		};

		//MSAA
		pipeInfo.multisample_state = {
			.sample_count = sampleCount,  // Enable MSAA in pipeline
			.sample_mask = 0  // Use all samples
		};

		pipeInfo.vertex_input_state = vertexInputState;

		//The same for almost all pipelines
		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create fill pipeline: %s", SDL_GetError());
			return false;
		}

		if (line) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
			pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
			if (!pipeline) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create line pipeline: %s", SDL_GetError());
				return false;
			}

		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,"Created pipeline %s", name);

		return true;

	}

	bool createLineVertPipeline(flecs::world& ecs, const char* name,SDL_GPUSampleCount sampleCount) {
		//Check vertex and fragment shader not being null
		if (!vertexShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vertex Shader is NULL unable to create pipeline");
			return false;
		}
		if (!fragmentShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Fragment Shader is NULL unable to create pipeline");
			return false;
		}

		const RenderContext& renderContext = ecs.get<RenderContext>();

		// Vertex attributes: position + color
		SDL_GPUVertexAttribute vertexAttributes[2] = {};

		// Position
		vertexAttributes[0].location = 0;
		vertexAttributes[0].buffer_slot = 0;
		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[0].offset = offsetof(LineVertex, position);

		// Color
		vertexAttributes[1].location = 1;
		vertexAttributes[1].buffer_slot = 0;
		vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		vertexAttributes[1].offset = offsetof(LineVertex, color);

		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
		vertexBufferDescription.slot = 0;
		vertexBufferDescription.pitch = sizeof(LineVertex);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 2;

		SDL_GPUColorTargetDescription colorTarget = {};
		colorTarget.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		colorTarget.blend_state.enable_blend = true;
		colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
		colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

		// Create the pipeline
		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);

		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
		pipeInfo.vertex_input_state = vertexInputState;

		// Target info
		pipeInfo.target_info.color_target_descriptions = &colorTarget;
		pipeInfo.target_info.num_color_targets = 1;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		// Depth stencil state
		pipeInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
		pipeInfo.depth_stencil_state.enable_depth_test = true;
		pipeInfo.depth_stencil_state.enable_depth_write = true;

		// MSAA
		pipeInfo.multisample_state.sample_count = sampleCount;
		pipeInfo.multisample_state.sample_mask = 0;

		// Rasterizer state
		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;  // Don't cull for lines
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

		pipeInfo.props = 0;

		// Create the pipeline
		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);

		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create %s pipeline: %s", name, SDL_GetError());
			return false;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Created pipeline %s", name);

		return true;
	}

	bool createPhysicsDebugPipeline(flecs::world& ecs, const char* name, SDL_GPUSampleCount sampleCount) {

		if (!vertexShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vertex Shader is NULL unable to create pipeline");
			return false;
		}
		if (!fragmentShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Fragment Shader is NULL unable to create pipeline");
			return false;
		}

		const RenderContext& renderContext = ecs.get<RenderContext>();

		// Vertex input state
		SDL_GPUVertexAttribute vertexAttributes[1] = {};

		// Position attribute
		vertexAttributes[0].location = 0;
		vertexAttributes[0].buffer_slot = 0;
		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[0].offset = 0;

		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
		vertexBufferDescription.slot = 0;
		vertexBufferDescription.pitch = sizeof(glm::vec3);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 1;


		// Create pipeline
		SDL_GPUColorTargetDescription colorDescs = {};
		colorDescs.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);
		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		pipeInfo.target_info.color_target_descriptions = &colorDescs;
		pipeInfo.target_info.num_color_targets = 1;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		pipeInfo.depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_LESS,
		.enable_depth_test = true,
		.enable_depth_write = true,
		};

		//MSAA
		pipeInfo.multisample_state = {
			.sample_count = sampleCount,  
			.sample_mask = 0 
		};

		pipeInfo.vertex_input_state = vertexInputState;

		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;

		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);
		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create %s pipeline: %s", name, SDL_GetError());
			return false;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Created pipeline %s", name);
		return true;
	}

	
	bool createEntIdPipeline(flecs::world& ecs, const char* name, SDL_GPUSampleCount sampleCount) {
		//Check vertex and fragment shader not being null
		if (!vertexShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Vertex Shader is NULL unable to create pipeline");
			return false;
		}
		if (!fragmentShader) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Fragment Shader is NULL unable to create pipeline");
			return false;
		}

		const RenderContext& renderContext = ecs.get<RenderContext>();

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

		SDL_GPUColorTargetDescription colorTarget = {};
		colorTarget.format = SDL_GPU_TEXTUREFORMAT_R32_UINT;
		colorTarget.blend_state.enable_blend = false;


		// Create the pipeline
		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);

		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		pipeInfo.vertex_input_state = vertexInputState;

		// Target info
		pipeInfo.target_info.color_target_descriptions = &colorTarget;
		pipeInfo.target_info.num_color_targets = 1;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		// Depth stencil state
		pipeInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
		pipeInfo.depth_stencil_state.enable_depth_test = true;
		pipeInfo.depth_stencil_state.enable_depth_write = true;

		pipeInfo.multisample_state = {
			.sample_count = sampleCount,  
			.sample_mask = 0  
		};

		// Rasterizer state
		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

		pipeInfo.props = 0;

		// Create the pipeline
		pipeline = SDL_CreateGPUGraphicsPipeline(renderContext.device, &pipeInfo);

		if (!pipeline) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create %s pipeline: %s", name, SDL_GetError());
			return false;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Created pipeline %s", name);

		return true;
	}
};


