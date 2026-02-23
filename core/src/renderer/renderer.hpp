#pragma once


#include "Model.hpp"
#include "../physics/physics.hpp"

#include "UI/UserInterface.hpp"
#include "PipelineLibrary/PipelineLibrary.hpp"

#include "Grid.hpp"
#include "RendererConfig.hpp"
#include "Camera.hpp"
#include "text/textRenderer.hpp"
#include "overlay/overlay.hpp"

#include "../util/util.hpp"

#include "renderUtil.hpp"

#include "pipeline.hpp"


struct Renderer {

	Uint32 swapchainWidth, swapchainHeight;

	SDL_GPUTexture* defaultTexture = NULL;
	SDL_GPUSampler* defaultSampler = NULL;
	SDL_GPUTextureSamplerBinding defaultSamplerBinding;

	SDL_GPUSampler* nearestSampler = NULL;

	SDL_GPUTexture* mainDepthStencilTexture = NULL;
	
	SDL_GPUTexture* mainColorTarget = NULL;
	SDL_GPUTexture* mainResolveTarget = NULL;

	SDL_GPUTexture* entIdColorTarget = NULL;
	SDL_GPUTexture* entIdDepthTexture = NULL;
	SDL_GPUTexture* selectedEntColorTarget = NULL;

	SDL_GPUTexture* editorVisualsDepthStencilTexture = NULL;

	SDL_GPURenderPass* mainRenderPass;

	FrameDataUniforms uniforms;

	flecs::world& ecs;

	flecs::query<Transform, ModelInstance >renderQuery;
	flecs::query<EditorVisuals>editorVisualsQuery;

	flecs::system drawPhysicsBodiesSys;

	UserInterface ui;
	PipelineLibrary pipelineLib;
	Overlay overlay;

	Renderer(flecs::world& ecs)
		: ecs(ecs), ui(ecs), pipelineLib(ecs), overlay(ecs)
	{

		createComponents();

		loadConfig();

		createWindow();
		createAndClaimGPU();
		createRenderTargets();

		createSamplerAndDefaultTexture();

		ui.init();

		buildRenderQueries();

		registerSystems();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "Renderer Initialized" RESET);
	}


	void initSubSystems() {

		pipelineLib.init();

		initPhysicsRenderer();

		overlay.init();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "Renderer SubSystems Initialized" RESET);

	}

	//Create singleton components
	void createComponents() {

		//They are all singleton entities
		//TODO maybe replace ecs.component with ecs.emplace 

		ecs.component<RenderContext>();
		ecs.set<RenderContext>({});

		ecs.component<FrameContext>();
		ecs.set<FrameContext>({});

		ecs.component<RenderConfig>();
		ecs.set<RenderConfig>({});

		// emplaced later
		ecs.component<fisiksDebugRenderer>("fisiksDebugRenderer").add(flecs::CanToggle);
	}



	void loadConfig() {

		//TODO MOVE HARDCODED path
		RenderConfig::loadRendererConfigINIFile(ecs, "config/renderConfig.ini");

	}

	bool createWindow() {

		if (!SDL_Init(SDL_INIT_VIDEO)) {
			printf("SDL_Init failed: %s\n", SDL_GetError());
			return false;
		}

		RenderContext& renderContext = ecs.get_mut<RenderContext>();
		RenderConfig& config = ecs.get_mut<RenderConfig>();


		//TODO have the option or rendering in other displays
		float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
		//TODO make window resizable maybe
		//SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | 
		//SDL_WindowFlags window_flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;


		renderContext.window = SDL_CreateWindow("Synthwave", config.windowWidth, config.windowHeight, SDL_WINDOW_VULKAN);
		if (!renderContext.window) {
			SDL_Log("Failed to create window!");
			return false;
		}

		SDL_SetWindowPosition(renderContext.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		//show cursor
		SDL_SetWindowRelativeMouseMode(renderContext.window, false);

		return true;
	}

	bool createAndClaimGPU() {

		bool debugMode = true;

		SDL_GPUVulkanOptions vulkanProps = { 0 };
		vulkanProps.vulkan_api_version = 4206592; // Vulkan 1.3.0

		SDL_PropertiesID props =  SDL_CreateProperties();

		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debugMode);

		// Get verbose debug output 
		if (debugMode) {
			SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN, true);
		}

		//anisotropic filtering
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, true);
		//Require hardware acceleration
		SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN, true);

		SDL_SetPointerProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER, &vulkanProps);

		RenderContext& renderContext = ecs.get_mut<RenderContext>();

		renderContext.device = SDL_CreateGPUDeviceWithProperties(props);
		if (!renderContext.device)
		{
			SDL_Log("GPUCreateDevice failed: %s", SDL_GetError());
			return false;
		}

		if (!SDL_ClaimWindowForGPUDevice(renderContext.device, renderContext.window))
		{
			SDL_Log("GPUClaimWindow failed");
			return false;
		}

		//cleanup
		SDL_DestroyProperties(props);

		// SDL_GPU_PRESENTMODE_IMMEDIATE for uncapped fps
		// SDL_GPU_PRESENTMODE_VSYNC for VSYNC
		SDL_SetGPUSwapchainParameters(renderContext.device, renderContext.window,
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

		return true;
	}


	bool createSamplerAndDefaultTexture() {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		SDL_GPUSamplerCreateInfo samplerCreateInfo{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		};

		defaultSampler = SDL_CreateGPUSampler(renderContext.device, &samplerCreateInfo);

		if (!defaultSampler) {
			SDL_Log("Could not create GPU sampler!");
			return false;
		}

		//Used in outlineComputeShader
		SDL_GPUSamplerCreateInfo nearestSamplerInfo = {
		.min_filter = SDL_GPU_FILTER_NEAREST,  // NOT LINEAR!
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		};
		nearestSampler = SDL_CreateGPUSampler(renderContext.device, &nearestSamplerInfo);

		if (!defaultSampler) {
			SDL_Log("Could not create nearestSampler !");
			return false;
		}

		// Load the image
		SDL_Surface* imageData1 = RenderUtil::LoadImage("assets/checkerboard.bmp", 4);
		if (imageData1 == NULL)
		{
			SDL_Log("Could not load first image data!");
			return false;
		}

		// Set up texture data
		const Uint32 imageSizeInBytes = imageData1->w * imageData1->h * 4;

		SDL_GPUTextureCreateInfo textureCreateInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width = static_cast<Uint32>(imageData1->w),
			.height = static_cast<Uint32>(imageData1->h),
			.layer_count_or_depth = 1, 
			.num_levels = 1,

		};
		defaultTexture = SDL_CreateGPUTexture(renderContext.device, &textureCreateInfo);

		if (!defaultTexture) {
			SDL_Log("Could not create GPU texture");
			return false;
		}

		SDL_SetGPUTextureName(
			renderContext.device,
			defaultTexture,
			"Default Texture"
		);

		// Set up buffer data
		SDL_GPUTransferBufferCreateInfo transferBufferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = imageSizeInBytes
		};
		SDL_GPUTransferBuffer* textureTransferBuffer = SDL_CreateGPUTransferBuffer(renderContext.device, &transferBufferInfo);

		void* textureTransferPtr = SDL_MapGPUTransferBuffer(renderContext.device, textureTransferBuffer, false);

		SDL_memcpy(textureTransferPtr, imageData1->pixels, imageSizeInBytes);

		SDL_UnmapGPUTransferBuffer(renderContext.device, textureTransferBuffer);

		// Upload the transfer data to the GPU resources
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(renderContext.device);
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

		return true;
	}


	bool createRenderTargets() {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig& config = ecs.get<RenderConfig>();

		// Main color target
		SDL_GPUTextureCreateInfo colorTextureInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = config.sampleCount,
		};

		mainColorTarget = SDL_CreateGPUTexture(renderContext.device, &colorTextureInfo);
		if (!mainColorTarget) {
			SDL_Log("Failed to create main color target: %s", SDL_GetError());
			return false;
		}

		SDL_GPUTextureCreateInfo resolveTextureInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1,
		};

		mainResolveTarget = SDL_CreateGPUTexture(renderContext.device, &resolveTextureInfo);
		if (!mainResolveTarget) {
			SDL_Log("Failed to create main resolve target: %s", SDL_GetError());
			return false;
		}

		// DepthStencil target used by mainRenderPass.
		//Needs to match sampleCount used by mainColor Target
		//Cannot be resolved by SDL_GPU at the moment
		// maybe it will be different in SDL 3.4.0 which will allow us to specify vulkan version
		//Stencil cannot be separated from depth to be used in shader!!!
		SDL_GPUTextureCreateInfo depthTextureCreateInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, 
			.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = config.sampleCount, // must match the sample count of mainColorTarget
		};
		mainDepthStencilTexture = SDL_CreateGPUTexture(renderContext.device, &depthTextureCreateInfo);

		if (!mainDepthStencilTexture) {
			SDL_Log("Failed to create depth texture: %s", SDL_GetError());
			return false;
		}

		
		// Entity ID target
		SDL_GPUTextureCreateInfo entIDTextureInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1
		};

		entIdColorTarget = SDL_CreateGPUTexture(renderContext.device, &entIDTextureInfo);
		if (!entIdColorTarget) {
			SDL_Log("Failed to create entIdColorTarget : %s", SDL_GetError());
			return false;
		}

		SDL_GPUTextureCreateInfo entIdDepthTextureInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT, // don't need stencil
			.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1, // must match the sample count of mainColorTarget
		};
		entIdDepthTexture = SDL_CreateGPUTexture(renderContext.device, &entIdDepthTextureInfo);

		if (!entIdDepthTexture) {
			SDL_Log("Failed to create depth texture: %s", SDL_GetError());
			return false;
		}

		// Selected Ent target
		SDL_GPUTextureCreateInfo selectedEntColorInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R32_UINT,
			.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1
		};

		selectedEntColorTarget = SDL_CreateGPUTexture(renderContext.device, &selectedEntColorInfo);
		if (!selectedEntColorTarget) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create selectedEntColorTarget : %s", SDL_GetError());
			return false;
		}


		SDL_GPUTextureCreateInfo editorVisualsDepthInfo = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT,
			.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET ,
			.width = config.windowWidth,
			.height = config.windowHeight,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1,
		};
		editorVisualsDepthStencilTexture = SDL_CreateGPUTexture(renderContext.device, &editorVisualsDepthInfo);

		if (!editorVisualsDepthStencilTexture) {
			SDL_Log("Failed to create depth texture: %s", SDL_GetError());
			return false;
		}

		return true;
	}


	void buildRenderQueries() {
		
		// Queries that use cascade(), group_by() or order_by() are cached
		renderQuery = ecs.query_builder<Transform, ModelInstance>()
			.with<RenderPipeline>(flecs::Wildcard)
			.group_by<RenderPipeline>()
			.build();

		editorVisualsQuery = ecs.query_builder<EditorVisuals>()
			//.with<RenderPipeline>(flecs::Wildcard)
			//.group_by<RenderPipeline>()
			.build();
	}

	void registerSystems() {

		createPhysicsBatchesSystem();
		renderPhysicsSystem();
		
	}

	void drawAll() {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		FrameContext& frameContext = ecs.get_mut<FrameContext>();
		const RenderConfig& config = ecs.get<RenderConfig>();

		beginRenderPass(renderContext, frameContext);

		drawModels(frameContext);

		//Keeping this part of the main render Pass because they look/work better
#if defined(JPH_DEBUG_RENDERER)
		drawPhysicsBodiesSys.run();
#endif

		SDL_EndGPURenderPass(mainRenderPass);

		drawEditorVisuals(frameContext, renderContext, config);

		blitToSwapchain(frameContext, config);

		ui.drawUI();

		SDL_SubmitGPUCommandBuffer(frameContext.commandBuffer);
	}


	void beginRenderPass(const RenderContext& renderContext,FrameContext& frameContext) {

		frameContext.commandBuffer = check_error_ptr(SDL_AcquireGPUCommandBuffer(renderContext.device));

		check_error_bool(SDL_WaitAndAcquireGPUSwapchainTexture(frameContext.commandBuffer, renderContext.window, &frameContext.swapchainTexture, &swapchainWidth, &swapchainHeight));

		if (!frameContext.swapchainTexture) {
			return; // Window is probably minimized
		}

		SDL_GPUColorTargetInfo targetInfos[] = {

			{
			.texture = mainColorTarget,
			.clear_color = { 0.0f, 0.0f, 1.0f, 1.0f },
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_RESOLVE,
			.resolve_texture = mainResolveTarget
			},
			
		};

		SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {};
		depthStencilTargetInfo.texture = mainDepthStencilTexture;
		depthStencilTargetInfo.clear_depth = 1.0f;
		depthStencilTargetInfo.clear_stencil = 0;                       
		depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		depthStencilTargetInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR; 
		depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_STORE;
		depthStencilTargetInfo.cycle = false;

		// Begin render pass
		mainRenderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			targetInfos, 1,
			&depthStencilTargetInfo
		);

		defaultSamplerBinding = { .texture = defaultTexture, .sampler = defaultSampler };

		// Maybe have an observer that runs each time camera is switched so that we don't query which cam is active every frame
		ecs.query<Camera, ActiveCamera>()
			.each([&](Camera& cam, ActiveCamera) {

			uniforms.view = cam.generateview();
			uniforms.projection = cam.generateProj();
			uniforms.viewProjection = cam.generateviewProj();

		});

		SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 0, &uniforms, sizeof(uniforms));
	}

	//Draws model Using whatever pipeline is bound. Uses models diffuse texture if it has any otherwise it will use default checkerboard
	//This function assumes its part of the main render pass ,which is not necessarily true. 
	void drawModel(const uint32_t entID,const ModelInstance& model, const Transform& transform,SDL_GPUCommandBuffer* cmdBuffer) {

		
		glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
		glm::mat4 modelRotation = glm::toMat4(transform.rotation);
		glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
		glm::mat4 modelMat = modelTranslation * modelRotation * modelScale;

		for (int j = 0; j < model.meshes.size(); j++) {

			const MeshInstance& mesh = model.meshes[j];

			//If mesh has a texture then 
			if (mesh.diffuseTexture != nullptr) {
				SDL_GPUTextureSamplerBinding sampler = { .texture = mesh.diffuseTexture, .sampler = defaultSampler };
				SDL_BindGPUFragmentSamplers(mainRenderPass, 0, &sampler, 1);

			}
			//else use the default texture
			//TODO look into binding this once per frame or just once ???
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
			//transpose because slang expect col major matrices
			PerModelUniforms modelUnifroms;
			modelUnifroms.mvp = glm::transpose(mvp);
			modelUnifroms.model = glm::transpose(meshMat);

			SDL_PushGPUVertexUniformData(cmdBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));

			SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &entID, sizeof(entID));

			SDL_DrawGPUIndexedPrimitives(mainRenderPass, mesh.size, 1, 0, 0, 0);
		}
	}

	void drawModels(const FrameContext& frameContext) {

		renderQuery.run([&](flecs::iter& it) {
			while (it.next()) {
				auto transforms = it.field<Transform>(0);
				auto models = it.field<ModelInstance>(1);
				flecs::entity pipeline_entity = flecs::entity(it.world(), it.group_id());

				//TODO FIX TO HANDLE NON-MULTI SAMPLED PIPELINES!!!
				const Pipeline* pipelineSecond = &pipeline_entity.get<Pipeline>();
				SDL_BindGPUGraphicsPipeline(mainRenderPass, pipelineSecond->pipelineMS);

				// Process all entities in this group
				for (auto i : it) {
					flecs::entity entity = it.entity(i);
					drawModel((uint32_t)entity.id(),models[i], transforms[i], frameContext.commandBuffer);

				}
			}
		});

	}


	void initPhysicsRenderer() {
#ifdef JPH_DEBUG_RENDERER

		ecs.emplace<fisiksDebugRenderer>(ecs);

		const RenderConfig& config = ecs.get<RenderConfig>();

		if (!config.RenderPhysics) {
			ecs.entity<fisiksDebugRenderer>().disable<fisiksDebugRenderer>();

		}

#endif
	}

	void createPhysicsBatchesSystem() {

#ifdef	JPH_DEBUG_RENDERER

		flecs::system createPhysicsBatchesSys = ecs.system<fisiksDebugRenderer>("CreatePhysicsBatchesSys")
			//.with<fisiksDebugRenderer>()
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(flecs::PostFrame)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {

			//Clear all the old data
			fisiksRenderer.batches.clear();
			fisiksRenderer.modelMatrices.clear();
			fisiksRenderer.lines.clear();

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

			// Does not actually draw it just puts all render batches in vector so they can be drawn during a render pass
			physicsSystem.DrawBodies(fisiksRenderer.drawSettings, &fisiksRenderer);

			physicsSystem.DrawConstraints(&fisiksRenderer);

			physicsSystem.DrawConstraintLimits(&fisiksRenderer);
			//physicsSystem.DrawConstraintReferenceFrame(&fisiksRenderer);


		});

#endif
	}

	void renderPhysicsSystem() {

#if defined(JPH_DEBUG_RENDERER)
		drawPhysicsBodiesSys = ecs.system<fisiksDebugRenderer>("DrawPhysicsBodiesSys")
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(0)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {

			ecs.query<Camera, ActiveCamera>()
				.each([&](Camera& cam, ActiveCamera) {

				RVec3Arg camPos(cam.position.x, cam.position.y, cam.position.z);

				fisiksRenderer.setCameraUniforms(camPos, cam.generateview(), cam.generateProj());

			});

			// The following are updated every frame so we pass them to fisiksRenderer here instead of during construction
			fisiksRenderer.renderPass = mainRenderPass;

			// actually draws
			fisiksRenderer.drawAll();

		});
#endif

	}

	void drawEntID(const FrameContext& frameContext,
		const RenderContext& renderContext,
		const RenderConfig& config) {

		SDL_GPUColorTargetInfo colorTargetInfo = {
		.texture = entIdColorTarget,
		.clear_color = { 0.0f, 0.0f, 1.0f, 1.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		};


		SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {};
		depthStencilTargetInfo.texture = entIdDepthTexture;
		depthStencilTargetInfo.clear_depth = 1.0f;
		depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);


		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID");
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();

		SDL_BindGPUGraphicsPipeline(mainRenderPass, pipeline.pipeline);

		renderQuery.run([&](flecs::iter& it) {
			while (it.next()) {
				auto transforms = it.field<Transform>(0);
				auto models = it.field<ModelInstance>(1);
				flecs::entity pipeline_entity = flecs::entity(it.world(), it.group_id());

				// Process all entities in this group
				for (auto i : it) {
					flecs::entity entity = it.entity(i);
					drawModel((uint32_t)entity.id(), models[i], transforms[i], frameContext.commandBuffer);

				}
			}
		});

		//add query for all item with editor-only proxy

		SDL_EndGPURenderPass(renderPass);

	}


	//A draw selected ent to selectedEntColorTarget and call applyOutlineComputePass
	void drawSelectedEnt(const FrameContext& frameContext,
		const RenderContext& renderContext,
		const RenderConfig& config) {

		flecs::entity selectedEnt = ecs.get<HighlightedEntRef>().ent;
		if (!selectedEnt) {
			return;
		}

		SDL_GPUColorTargetInfo colorTargetInfo = {
		.texture = selectedEntColorTarget,
		.clear_color = { 0.0f, 0.0f, 1.0f, 1.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {};
		depthStencilTargetInfo.texture = entIdDepthTexture;
		depthStencilTargetInfo.clear_depth = 1.0f;
		depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);

		//TODO add ablity to draw models that don't have a model
		const ModelInstance& model = selectedEnt.get<ModelInstance>();
		const Transform& transform = selectedEnt.get<Transform>();
		
		// Write highlighted object to stencil buffer 
		
		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID");
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline.pipeline);

		drawModel((uint32_t)selectedEnt.id(),model, transform, frameContext.commandBuffer);

		SDL_EndGPURenderPass(renderPass);

		applyOutlineComputePass(frameContext, renderContext, config);

	}

	//Send entIDStencilTarget to read from
	//Send the colorTarget to write to 
	//Send selected EntID, color, and thickness of outline as uniforms
	void applyOutlineComputePass(const FrameContext& frameContext,
		const RenderContext& renderContext,
		const RenderConfig& config) {
		
		//TODO check some other way
		flecs::entity selectedEnt = ecs.get<HighlightedEntRef>().ent;
		if (!selectedEnt) {
			return;
		}

		SDL_GPUStorageTextureReadWriteBinding outputBinding = {
		.texture = mainResolveTarget,
		.mip_level = 0,
		.layer = 0,
		};


		SDL_GPUComputePass* computePass = SDL_BeginGPUComputePass(
			frameContext.commandBuffer,
			&outputBinding, 
			1,
			nullptr, // No storage buffer bindings
			0
		);

		// Bind compute pipeline
		flecs::entity outlineComputePipelineEnt = ecs.lookup("pipelineOutlineCompute");
		const ComputePipeline& outlineComputePipeline = outlineComputePipelineEnt.get<ComputePipeline>();
		SDL_BindGPUComputePipeline(computePass, outlineComputePipeline.pipeline);

		// Bind textures

		//SDL_GPUTextureSamplerBinding inputBinding = {
		//	.texture = selectedEntColorTarget,
		//	.sampler = nearestSampler
		//};
		//SDL_BindGPUComputeSamplers(computePass, 0, &inputBinding, 1);

		SDL_BindGPUComputeStorageTextures(computePass, 0, &selectedEntColorTarget, 1);

		// Push outline color uniform
		struct OutlineParams {
			glm::vec4 color;
			float thickness;
			uint32_t entId;
			uint32_t screenWidth;   
			uint32_t screenHeight;  
		} params;

		params.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		params.thickness = 2.0f;
		params.entId = static_cast<uint32_t>(selectedEnt.id());
		params.screenWidth = config.windowWidth;
		params.screenHeight = config.windowHeight;

		SDL_PushGPUComputeUniformData(frameContext.commandBuffer, 0, &params, sizeof(params));

		// Dispatch compute shader (8x8 thread groups)
		uint32_t groupsX = (config.windowWidth + 7) / 8;
		uint32_t groupsY = (config.windowHeight + 7) / 8;
		SDL_DispatchGPUCompute(computePass, groupsX, groupsY, 1);

		SDL_EndGPUComputePass(computePass);

	}

	void drawEditorVisuals(const FrameContext& frameContext,
		const RenderContext& renderContext,
		const RenderConfig& config) {

		const EditorState* editorState = ecs.try_get<EditorState>();
		if (!editorState) {
			return;
		}
		if (*editorState != EditorState::Enabled) {
			return;
		}

		drawEntID(frameContext, renderContext, config);
		drawSelectedEnt(frameContext, renderContext, config);
		editorVisualsPass(frameContext, renderContext, config);

		
	}

	void editorVisualsPass(const FrameContext& frameContext,
		const RenderContext& renderContext,
		const RenderConfig& config) {

		SDL_GPUColorTargetInfo colorTargetInfo = {
		.texture = mainResolveTarget,
		.clear_color = { 0.0f, 0.0f, 1.0f, 1.0f },
		.load_op = SDL_GPU_LOADOP_LOAD, // load everything from main pass
		.store_op = SDL_GPU_STOREOP_STORE,
		};


		SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {};
		depthStencilTargetInfo.texture = editorVisualsDepthStencilTexture;
		depthStencilTargetInfo.clear_depth = 1.0f;
		depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);


		editorVisualsQuery.each([&](flecs::entity e, EditorVisuals visualElement) {

			flecs::entity pipelineEnt = e.target<RenderPipeline>();
			const Pipeline pipeline = pipelineEnt.get<Pipeline>();

			SDL_BindGPUGraphicsPipeline(renderPass, pipeline.pipeline);

			SDL_GPUBufferBinding vertexBufferBinding = {};
			vertexBufferBinding.buffer = e.get<VertexBuffer>().handle;
			vertexBufferBinding.offset = 0;
			SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

			SDL_DrawGPUPrimitives(renderPass, e.get<LineVertices>().data.size(), 1, 0, 0);

		});

		SDL_EndGPURenderPass(renderPass);

	}

	//TODO maybe do this more efficiently
	uint32_t readFromTexture(int mouseX, int mouseY) const {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig& config = ecs.get<RenderConfig>();

		SDL_GPUTransferBuffer* entityIDTransferBuffer;

		SDL_GPUTransferBufferCreateInfo transferInfo = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,  
			.size = sizeof(uint32_t)  // Just one pixel
		};

		entityIDTransferBuffer = SDL_CreateGPUTransferBuffer(
			renderContext.device,
			&transferInfo
		);


		// Bounds check
		if (mouseX < 0 || mouseY < 0 ||
			mouseX >= config.windowWidth || mouseY >= config.windowHeight) {
			return 0;  // Invalid background
		}

		SDL_GPUCommandBuffer* copyCommandBuffer = SDL_AcquireGPUCommandBuffer(renderContext.device);

		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(copyCommandBuffer);

		// Define the texture region to read (single pixel)
		SDL_GPUTextureRegion textureRegion = {
			.texture = entIdColorTarget,
			.mip_level = 0,
			.layer = 0,
			.x = static_cast<uint32_t>(mouseX),
			.y = static_cast<uint32_t>(mouseY),
			.w = 1,
			.h = 1,
			.d = 1
		};


		SDL_GPUTextureTransferInfo textureTransferInfo = {
			.transfer_buffer = entityIDTransferBuffer,
			.offset = 0,
			.pixels_per_row = 0, 
			.rows_per_layer = 0  
		};

		// Queue the download
		SDL_DownloadFromGPUTexture(
			copyPass,
			&textureRegion,
			&textureTransferInfo
		);


		SDL_EndGPUCopyPass(copyPass);

		SDL_GPUFence* fence =  SDL_SubmitGPUCommandBufferAndAcquireFence(copyCommandBuffer);

		// TODO Maybe do this in a background thread
		SDL_WaitForGPUFences(
			renderContext.device,
			true, 
			&fence,
			1
		);

		uint32_t* data = (uint32_t*)SDL_MapGPUTransferBuffer(
			renderContext.device,
			entityIDTransferBuffer,
			false  // false = read-only
		);

		// Read the entity ID
		uint32_t entityID = data[0];

		// Unmap the buffer
		SDL_UnmapGPUTransferBuffer(renderContext.device, entityIDTransferBuffer);

		return entityID;

	}

	void blitToSwapchain(const FrameContext& frameContext, const RenderConfig& config) {

		SDL_GPUBlitInfo blitInfo = {};
		blitInfo.source.texture = mainResolveTarget;
		blitInfo.source.x = 0;
		blitInfo.source.y = 0;
		blitInfo.source.w = config.windowWidth;
		blitInfo.source.h = config.windowHeight;
		blitInfo.destination.texture = frameContext.swapchainTexture;
		blitInfo.destination.x = 0;
		blitInfo.destination.y = 0;
		blitInfo.destination.w = swapchainWidth;
		blitInfo.destination.h = swapchainHeight;
		blitInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		blitInfo.filter = SDL_GPU_FILTER_LINEAR;
		SDL_BlitGPUTexture(frameContext.commandBuffer, &blitInfo);

	}

};
