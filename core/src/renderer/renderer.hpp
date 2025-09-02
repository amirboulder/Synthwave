#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Entity.hpp"
#include "Grid.hpp"
#include "RendererConfig.hpp"
#include "Camera.hpp"
#include "text/freeType.hpp"
#include "text/textRenderer.hpp"

#include "Shader.hpp"
#include "pipeline.hpp"

#include "optick.h"

using std::vector;

struct FrameDataUniforms {
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection;
};
 
struct RenderProps {
	SDL_GPUDevice* device = NULL;
};


struct Renderer {

	vector<Pipeline> pipelines;


	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_GPUDevice* device = NULL;

	SDL_GPUCommandBuffer* commandBuffer;
	SDL_GPUTexture* swapchainTexture;
	Uint32 swapchainWidth, swapchainHeight;


	SDL_GPUTexture* defaultTexture = NULL;
	SDL_GPUSampler* defaultSampler = NULL;

	SDL_GPUTexture* depthTexture = NULL;


	SDL_GPUTexture* msaaColorTarget;
	SDL_GPUTexture* resolveTarget;


	RendererConfig& config;

	
	std::unique_ptr<fisiksDebugRenderer> fisiksRenderer; // Deferred construction

	Renderer(RendererConfig renderConfig, PhysicsSystem& physicsSystem)
		:	config(renderConfig), fisiksRenderer(nullptr)
	{


		createWindow();
		createAndClaimGPU();
		createRenderTargets();

		createSamplerAndDefaultTexture();
		
		if (config.RendererPhysics) {
			initFisisksRenderer(physicsSystem);

		}

	}


	bool createWindow() {
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			printf("SDL_Init failed: %s\n", SDL_GetError());
			return false;
		}

		window = SDL_CreateWindow("Synthwave", config.windowWidth, config.windowHeight, NULL);
		if (!window) {
			SDL_Log("Failed to create window!");
			return false;
		}
		

		SDL_SetWindowRelativeMouseMode(window, true);

		return true;
	}

	bool createAndClaimGPU() {

		device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
		if (!device)
		{
			SDL_Log("GPUCreateDevice failed: %s", SDL_GetError());
			return false;
		}

		if (!SDL_ClaimWindowForGPUDevice(device, window))
		{
			SDL_Log("GPUClaimWindow failed");
			return false;
		}

		

		SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

		return true;
	}


	void drawFps(int fps) {
		//OPTICK_EVENT();
		//textRenderer.renderText(std::to_string(fps), 0.0, winHeight - 60, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	}

	void drawText(std::string text, glm::vec2 postion, float scale, glm::vec3 color) {
		//OPTICK_EVENT();
		//textRenderer.renderText(text, postion.x, postion.y, scale, color);
	}

	

	bool createPipeline(SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader,
		SDL_GPUGraphicsPipeline* & pipeline,bool line = false)
	{

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
		vertexBufferDescription.pitch = sizeof(VertexData);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 4;



		// Create pipeline
		SDL_GPUColorTargetDescription coldescs = {};
		coldescs.format = SDL_GetGPUSwapchainTextureFormat(device, window);

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
			.sample_count = config.sampleCountMSAA,  // Enable MSAA in pipeline
			.sample_mask = 0  // Use all samples
		};

		pipeInfo.vertex_input_state = vertexInputState;

		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
		if (!pipeline) {
			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
			return false;
		}

		if (line) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
			pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
			if (!pipeline) {
				SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
				return false;
			}

		}

		SDL_Log("Built pipeline");

		return true;
	}

	bool createSamplerAndDefaultTexture() {


		SDL_GPUSamplerCreateInfo samplerCreateInfo{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		};

		defaultSampler = SDL_CreateGPUSampler(device, &samplerCreateInfo);

		if (!defaultSampler) {
			SDL_Log("Could not create GPU sampler!");
			return -1;
		}


		// Load the image
		SDL_Surface* imageData1 = LoadImage("assets/checkerboard.bmp", 4);
		if (imageData1 == NULL)
		{
			SDL_Log("Could not load first image data!");
			return -1;
		}

		// Set up texture data
		const Uint32 imageSizeInBytes = imageData1->w * imageData1->h * 4;

		SDL_GPUTextureCreateInfo textureCreateInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width = static_cast<Uint32>(imageData1->w),
			.height = static_cast<Uint32>(imageData1->h),
			.layer_count_or_depth = 1, //maybe 2 ?
			.num_levels = 1,

		};
		defaultTexture = SDL_CreateGPUTexture(device, &textureCreateInfo);

		if (!defaultTexture) {
			SDL_Log("Could not create GPU texture");
			return -1;
		}

		SDL_SetGPUTextureName(
			device,
			defaultTexture,
			"Default Texture"
		);

		// Set up buffer data
		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = imageSizeInBytes
		};
		SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferInfo);

		void* textureTransferPtr = SDL_MapGPUTransferBuffer(device, textureTransferBuffer, false);

		SDL_memcpy(textureTransferPtr, imageData1->pixels, imageSizeInBytes);

		SDL_UnmapGPUTransferBuffer(device, textureTransferBuffer);

		// Upload the transfer data to the GPU resources
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);



		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = textureTransferBuffer,
			.offset = 0,
		};

		SDL_GPUTextureRegion textureRegion = {
			.texture = defaultTexture,
			.w = static_cast<Uint32>(imageData1->w),
			.h = static_cast<Uint32>(imageData1->h),
			.d = 1

		};

		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);



		
	}


	bool createRenderTargets() {

		SDL_GPUTextureCreateInfo depthTextureCreateInfo = {
		 .type = SDL_GPU_TEXTURETYPE_2D,
		 .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
		 .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		 .width = static_cast<uint32_t>(config.windowWidth),
		 .height = static_cast<uint32_t>(config.windowHeight),
		 .layer_count_or_depth = 1,
		 .num_levels = 1,
		.sample_count = config.sampleCountMSAA,
		};
		this->depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);

		if (!depthTexture) {
			SDL_Log("Failed to create depth texture: %s", SDL_GetError());
			return false;
		}

		// Color render target with MSAA
		SDL_GPUTextureCreateInfo colorTextureInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, 
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
			.width = static_cast<uint32_t>(config.windowWidth),
			.height = static_cast<uint32_t>(config.windowHeight),
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = config.sampleCountMSAA
		};

		msaaColorTarget = SDL_CreateGPUTexture(device, &colorTextureInfo);
		if (!msaaColorTarget) {
			SDL_Log("Failed to create MSAA color target: %s", SDL_GetError());
			return false;
		}

		SDL_GPUTextureCreateInfo resolveTextureInfo = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = static_cast<uint32_t>(config.windowWidth),
		.height = static_cast<uint32_t>(config.windowHeight),
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1  // Always 1x for resolve target
		};

		resolveTarget = SDL_CreateGPUTexture(device, &resolveTextureInfo);
		if (!resolveTarget) {
			SDL_Log("Failed to create resolve target: %s", SDL_GetError());
			return false;
		}

		return true;
	}


	void draw(glm::mat4 view, glm::mat4 proj,FreeCam & camera) {
		
		// Begin command buffer
		commandBuffer = SDL_AcquireGPUCommandBuffer(device);
		if (!commandBuffer) {
			std::cerr << "Failed to acquire command buffer: " << SDL_GetError() << std::endl;
			return;
		}

		
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, &swapchainWidth, &swapchainHeight)) {
			std::cerr << "Failed to acquire swapchain texture: " << SDL_GetError() << std::endl;
			return;
		}

		if (!swapchainTexture) {
			return; // Window is probably minimized
		}

		

		// Color target info
		SDL_GPUColorTargetInfo colorTargetInfo = {
		.texture = msaaColorTarget,
		.clear_color = { 0.0f, 0.0f, 1.0f, 1.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_RESOLVE,
		.resolve_texture = resolveTarget

		
		};
		
		SDL_GPUDepthStencilTargetInfo depthTargetInfo = { 0 };
		depthTargetInfo.texture = depthTexture;
		depthTargetInfo.clear_depth = 1.0f;
		depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;


		// Begin render pass
		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			commandBuffer,
			&colorTargetInfo, 1,
			&depthTargetInfo 
		);


		FrameDataUniforms uniforms;
		uniforms.view = view;
		uniforms.projection = proj;
		uniforms.viewProjection = proj * view;

		SDL_PushGPUVertexUniformData(commandBuffer, 0, &uniforms, sizeof(uniforms));

		SDL_GPUTextureSamplerBinding sampler = { .texture = defaultTexture, .sampler = defaultSampler };


		for (int i = 0; i < pipelines.size(); i++) {

			Pipeline& pipeline = pipelines[i];

			SDL_BindGPUFragmentSamplers(renderPass, 0, &sampler, 1);

			pipeline.draw(renderPass,commandBuffer, uniforms.viewProjection,defaultTexture,defaultSampler);
			
		}


		if (config.RendererPhysics) {

			RVec3Arg camPos(camera.position.x, camera.position.y, camera.position.z);
			fisiksRenderer->setCameraUnifroms(camPos, camera.generateview(), camera.generateProj());

			// The following maybe updated everyframe so we need to pass them to fisiksRenderer
			// here instead of during construction
			fisiksRenderer->renderPass = renderPass;
			fisiksRenderer->commandBuffer = commandBuffer;

			SDL_BindGPUGraphicsPipeline(renderPass, fisiksRenderer->pipeline);

			fisiksRenderer->physicsSystem.DrawBodies(fisiksRenderer->drawSettings, &*fisiksRenderer);
			
		}

		SDL_EndGPURenderPass(renderPass);

		submitCommandBuffer();


	}

	void submitCommandBuffer() {

		SDL_GPUBlitInfo blitInfo = {};
		blitInfo.source.texture = resolveTarget;
		blitInfo.source.x = 0;
		blitInfo.source.y = 0;
		blitInfo.source.w = config.windowWidth;
		blitInfo.source.h = config.windowHeight;
		blitInfo.destination.texture = swapchainTexture;
		blitInfo.destination.x = 0;
		blitInfo.destination.y = 0;
		blitInfo.destination.w = swapchainWidth;
		blitInfo.destination.h = swapchainHeight;
		blitInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		blitInfo.filter = SDL_GPU_FILTER_LINEAR;

		SDL_BlitGPUTexture(commandBuffer, &blitInfo);
		

		// Submit command buffer
		SDL_SubmitGPUCommandBuffer(commandBuffer);

	}

	void initFisisksRenderer(PhysicsSystem& physicsSystem) {

		SDL_Log("initFisisksRenderer()");

		if (!fisiksRenderer) {
			fisiksRenderer = std::make_unique<fisiksDebugRenderer>(device,window,config.DrawBoundingBoxPhysics,config.DrawShapeWireframePhysics,physicsSystem);

			//shader::generateSpirvShaders("shaders/slang/physicsRender.slang", "shaders/compiled/physicsRenderVS.spv", "shaders/compiled/physicsRenderFS.spv");
			PL::loadVertexShader(device, fisiksRenderer->vertexShader, "shaders/compiled/physicsRenderVS.spv", 0, 2, 0, 0);
			PL::loadFragmentShader(device, fisiksRenderer->fragmentShader, "shaders/compiled/physicsRenderFS.spv", 0, 0, 0, 0);

			fisiksRenderer->createPipeline();
		}
	}

	//TODO move to Material.hpp
	SDL_Surface* LoadImage(const char* path, int desiredChannels)
	{
		//char fullPath[256];
		SDL_Surface* result;
		SDL_PixelFormat format;

		//SDL_snprintf(fullPath, sizeof(fullPath), "%sContent/Images/%s", BasePath, imageFilename);


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


};
