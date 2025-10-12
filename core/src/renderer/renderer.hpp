#pragma once

#include <vector>

#include <flecs.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Model.hpp"
#include "../physics/physics.hpp"

#include "Grid.hpp"
#include "RendererConfig.hpp"
#include "Camera.hpp"
#include "text/textRenderer.hpp"

#include "../util/util.hpp"

#include "renderUtil.hpp"

#include "Shader.hpp"
#include "pipeline.hpp"

#include "optick.h"

using std::vector;

struct Renderer {

	Context context;

	TextRenderer textRenderer;

	Uint32 swapchainWidth, swapchainHeight;

	SDL_GPUTexture* defaultTexture = NULL;
	SDL_GPUSampler* defaultSampler = NULL;
	SDL_GPUTextureSamplerBinding defaultSamplerBinding;

	SDL_GPUTexture* depthTexture = NULL;

	SDL_GPUTexture* msaaColorTarget;
	SDL_GPUTexture* resolveTarget;

	SDL_GPURenderPass* mainRenderPass;

	RendererConfig& config;

	FrameDataUniforms uniforms;

	flecs::world & ecs;

	flecs::query<Transform, ModelInstance>q1;
	flecs::query<Transform, ModelInstance>q2;


	#if defined(JPH_DEBUG_RENDERER)
	std::unique_ptr<fisiksDebugRenderer> fisiksRenderer;
	#endif

	Renderer(flecs::world & ecs, RendererConfig renderConfig, PhysicsSystem& physicsSystem)
		: ecs(ecs),	config(renderConfig)
	#if defined(JPH_DEBUG_RENDERER)
		, fisiksRenderer(nullptr)
	#endif

	{

		createWindow();
		createAndClaimGPU();
		createRenderTargets();

		createSamplerAndDefaultTexture();
			
		context.sampleCountMSAA = config.sampleCountMSAA;

		textRenderer.init(&context);
		textRenderer.updateText("Synthwave", textRenderer.staticText);
		SetDrawFPS(true);

		
		#if defined(JPH_DEBUG_RENDERER)
		if (config.RendererPhysics) {
			initFisisksRenderer(physicsSystem);
		}
		#endif


		//Build render Queries
		buildRenderQueries();
	}


	bool createWindow() {
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			printf("SDL_Init failed: %s\n", SDL_GetError());
			return false;
		}

		context.window = SDL_CreateWindow("Synthwave", config.windowWidth, config.windowHeight, NULL);
		if (!context.window) {
			SDL_Log("Failed to create window!");
			return false;
		}
		

		SDL_SetWindowRelativeMouseMode(context.window, true);

		return true;
	}

	bool createAndClaimGPU() {

		context.device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
		if (!context.device)
		{
			SDL_Log("GPUCreateDevice failed: %s", SDL_GetError());
			return false;
		}

		if (!SDL_ClaimWindowForGPUDevice(context.device, context.window))
		{
			SDL_Log("GPUClaimWindow failed");
			return false;
		}

		
		// SDL_GPU_PRESENTMODE_IMMEDIATE for uncapped fps
		// SDL_GPU_PRESENTMODE_VSYNC for VSYNC
		SDL_SetGPUSwapchainParameters(context.device, context.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

		return true;
	}


	void SetDrawFPS(bool drawFPS) {
		textRenderer.fps = drawFPS;
	}

	void setFPSText(int fps) {

		std::string strFPS = std::to_string(fps);
		textRenderer.updateText(strFPS.c_str(), textRenderer.fpsText);
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

		defaultSampler = SDL_CreateGPUSampler(context.device, &samplerCreateInfo);

		if (!defaultSampler) {
			SDL_Log("Could not create GPU sampler!");
			return -1;
		}


		// Load the image
		SDL_Surface* imageData1 = RenderUtil::LoadImage("assets/checkerboard.bmp", 4);
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
		defaultTexture = SDL_CreateGPUTexture(context.device, &textureCreateInfo);

		if (!defaultTexture) {
			SDL_Log("Could not create GPU texture");
			return -1;
		}

		SDL_SetGPUTextureName(
			context.device,
			defaultTexture,
			"Default Texture"
		);

		// Set up buffer data
		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = imageSizeInBytes
		};
		SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(context.device, &transferBufferInfo);

		void* textureTransferPtr = SDL_MapGPUTransferBuffer(context.device, textureTransferBuffer, false);

		SDL_memcpy(textureTransferPtr, imageData1->pixels, imageSizeInBytes);

		SDL_UnmapGPUTransferBuffer(context.device, textureTransferBuffer);

		// Upload the transfer data to the GPU resources
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(context.device);
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
		this->depthTexture = SDL_CreateGPUTexture(context.device, &depthTextureCreateInfo);

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

		msaaColorTarget = SDL_CreateGPUTexture(context.device, &colorTextureInfo);
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

		resolveTarget = SDL_CreateGPUTexture(context.device, &resolveTextureInfo);
		if (!resolveTarget) {
			SDL_Log("Failed to create resolve target: %s", SDL_GetError());
			return false;
		}

		return true;
	}


	void buildRenderQueries() {

		//TODO consider using <PipelineType> tag instead of .without and .with, do performance benchmarks

		q1 = ecs.query_builder<Transform, ModelInstance>()
			.without<CustomPipeline>(flecs::Wildcard) // no custom pipeline
			.cached()
			.build();


		// Queries that use cascade(), group_by() or order_by() are cached
		q2 = ecs.query_builder<Transform, ModelInstance>()
			.with<CustomPipeline>(flecs::Wildcard)  // Match any (CustomPipeline, *)
			.group_by<CustomPipeline>()
			.build();
	}


	void beginDraw() {
		context.commandBuffer = check_error_ptr(SDL_AcquireGPUCommandBuffer(context.device));

		check_error_bool(SDL_WaitAndAcquireGPUSwapchainTexture(context.commandBuffer, context.window, &context.swapchainTexture, &swapchainWidth, &swapchainHeight));

		if (!context.swapchainTexture) {
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
		mainRenderPass = SDL_BeginGPURenderPass(
			context.commandBuffer,
			&colorTargetInfo, 1,
			&depthTargetInfo
		);


		//TODO see if there is any benefit to taking this out ot the draw so it  only happens once
		defaultSamplerBinding = { .texture = defaultTexture, .sampler = defaultSampler };

		//SDL_BindGPUFragmentSamplers(mainRenderPass, 0, &defaultSamplerBinding, 1);

		ecs.query<Camera, ActiveCamera>()
			.each([&](Camera& cam, ActiveCamera) {

			uniforms.view = cam.generateview();
			uniforms.projection = cam.generateProj();
			uniforms.viewProjection = uniforms.projection * uniforms.view;

		});
		
		SDL_PushGPUVertexUniformData(context.commandBuffer, 0, &uniforms, sizeof(uniforms));
	}

	void drawModel(ModelInstance & model, Transform& transform) {

		glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
		glm::mat4 modelRototaion = glm::toMat4(transform.rotation);
		glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
		glm::mat4 modelMat = modelTranslation * modelRototaion * modelScale;

		for (int j = 0; j < model.meshes.size(); j++) {

			MeshInstance& mesh = model.meshes[j];

			//If mesh has a texture then 
			if (mesh.diffuseTexture != nullptr) {
				SDL_GPUTextureSamplerBinding sampler = { .texture = mesh.diffuseTexture, .sampler = defaultSampler };
				SDL_BindGPUFragmentSamplers(mainRenderPass, 0, &sampler, 1);

			}
			//else use the default texture
			else {

				SDL_BindGPUFragmentSamplers(mainRenderPass, 0, &defaultSamplerBinding, 1);
			}

			SDL_GPUBufferBinding vertexBufferBinding = {};
			vertexBufferBinding.buffer = mesh.vertexBuffer;
			vertexBufferBinding.offset = 0;

			SDL_GPUBufferBinding indexBufferBinding = {};
			indexBufferBinding.buffer = mesh.indexBuffer;
			indexBufferBinding.offset = 0;

			SDL_BindGPUVertexBuffers(mainRenderPass, 0, &vertexBufferBinding, 1);
			SDL_BindGPUIndexBuffer(mainRenderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);


			glm::mat4 meshTrans = glm::translate(glm::mat4(1.0f), mesh.transform.position);
			glm::mat4 meshRot = glm::toMat4(mesh.transform.rotation);
			glm::mat4 meshScale = glm::scale(glm::mat4(1.0f), mesh.transform.scale);
			glm::mat4 meshMat = meshTrans * meshRot * meshScale;

			meshMat = modelMat * meshMat;

			glm::mat4 mvp = uniforms.viewProjection * meshMat;

			mvp = glm::transpose(mvp);

			PerModelUniforms modelUnifroms;
			modelUnifroms.mvp = mvp;
			modelUnifroms.model = meshMat;

			SDL_PushGPUVertexUniformData(context.commandBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));

			SDL_DrawGPUIndexedPrimitives(mainRenderPass, mesh.size, 1, 0, 0, 0);
		}
	}

	void endDraw() {

		#if defined(JPH_DEBUG_RENDERER)
		if (config.RendererPhysics) {
			renderPhysics(mainRenderPass);
		}
		#endif


		SDL_EndGPURenderPass(mainRenderPass);
		resolveBlit();


		textRenderer.drawAll(config.windowWidth, config.windowHeight);

		SDL_SubmitGPUCommandBuffer(context.commandBuffer);

	}

	
	void resolveBlit() {

		SDL_GPUBlitInfo blitInfo = {};
		blitInfo.source.texture = resolveTarget;
		blitInfo.source.x = 0;
		blitInfo.source.y = 0;
		blitInfo.source.w = config.windowWidth;
		blitInfo.source.h = config.windowHeight;
		blitInfo.destination.texture = context.swapchainTexture;
		blitInfo.destination.x = 0;
		blitInfo.destination.y = 0;
		blitInfo.destination.w = swapchainWidth;
		blitInfo.destination.h = swapchainHeight;
		blitInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		blitInfo.filter = SDL_GPU_FILTER_LINEAR;

		SDL_BlitGPUTexture(context.commandBuffer, &blitInfo);
		
	}

	#if defined(JPH_DEBUG_RENDERER)
	void initFisisksRenderer(PhysicsSystem& physicsSystem) {

		SDL_Log("initFisisksRenderer()");

		if (!fisiksRenderer) {
			fisiksRenderer = std::make_unique<fisiksDebugRenderer>(context.device, context.window,config.DrawBoundingBoxPhysics,config.DrawShapeWireframePhysics,physicsSystem);

			//TODO MOVE THIS 
			//shader::generateSpirvShaders("shaders/slang/physicsRender.slang", "shaders/compiled/physicsRender.vert.spv", "shaders/compiled/physicsRender.frag.spv");
			RenderUtil::loadShaderSPRIV(context.device, fisiksRenderer->vertexShader, "shaders/compiled/physicsRender.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,0, 2, 0, 0);
			RenderUtil::loadShaderSPRIV(context.device, fisiksRenderer->fragmentShader, "shaders/compiled/physicsRender.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT,0, 0, 0, 0);

			fisiksRenderer->createPipeline();
		}
	}

	void renderPhysics(SDL_GPURenderPass* renderPass) {

		ecs.query<Camera, ActiveCamera>()
			.each([&](Camera& cam, ActiveCamera) {

			RVec3Arg camPos(cam.position.x, cam.position.y, cam.position.z);
			fisiksRenderer->setCameraUnifroms(camPos, cam.generateview(), cam.generateProj());

		});

		

		// The following might be updated everyframe so we need to pass them to fisiksRenderer
		// here instead of during construction
		fisiksRenderer->renderPass = renderPass;
		fisiksRenderer->commandBuffer = context.commandBuffer;

		SDL_BindGPUGraphicsPipeline(renderPass, fisiksRenderer->pipeline);

		fisiksRenderer->physicsSystem.DrawBodies(fisiksRenderer->drawSettings, &*fisiksRenderer);
	}
	#endif


	void drawAll() {

		beginDraw();

		const RenderState & state = ecs.lookup("RenderState").get<RenderState>();
		const Pipeline& pipeline = state.activePipeline.get<Pipeline>();

		SDL_BindGPUGraphicsPipeline(mainRenderPass, pipeline.pipeline);

		q1.each([&](flecs::entity e, Transform& transform, ModelInstance& model) {

			drawModel(model, transform);

		});


		q2.run([&](flecs::iter& it){
			while (it.next()) {
				
				flecs::entity pipeline_entity = flecs::entity(it.world(), it.group_id());

				const Pipeline* pipelineSecond =  & pipeline_entity.get<Pipeline>();
				SDL_BindGPUGraphicsPipeline(mainRenderPass, pipelineSecond->pipeline);

				for (auto i : it) {

					Transform& transform = *it.field<Transform>(0);
					ModelInstance& model = *it.field<ModelInstance>(1);

					drawModel(model, transform);

				}
			}
		});

		endDraw();
	}

};
