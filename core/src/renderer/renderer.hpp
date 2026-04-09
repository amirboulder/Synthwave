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

#include "../AssetLibrary/AssetLibrary.hpp"

struct DrawBatch {
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	SDL_GPUBuffer* transformsBuffer = nullptr;
	uint32_t              numIndices = 0;
	std::vector<glm::mat4> transforms;

	SDL_GPUTexture* diffuseTexture = nullptr;
};

struct PipelineBatch {
	std::unordered_map<uint32_t, DrawBatch> meshBatches; // keyed by mesh asset index
};

struct Renderer {

	Uint32 swapchainWidth, swapchainHeight;

	SDL_GPUTexture* defaultTexture = nullptr;
	SDL_GPUSampler* defaultSampler = nullptr;
	SDL_GPUTextureSamplerBinding defaultSamplerBinding;

	SDL_GPUSampler* nearestSampler = nullptr;

	SDL_GPUTexture* mainDepthStencilTexture = nullptr;

	SDL_GPUTexture* mainColorTarget = nullptr;
	SDL_GPUTexture* mainResolveTarget = nullptr;

	SDL_GPUTexture* entIdColorTarget = nullptr;
	SDL_GPUTexture* entIdDepthTexture = nullptr;
	SDL_GPUTexture* selectedEntColorTarget = nullptr;

	SDL_GPUTexture* editorVisualsDepthStencilTexture = nullptr;

	SDL_GPURenderPass* activeRenderPass = nullptr;

	FrameDataUniforms uniforms;

	std::unordered_map<uint32_t, PipelineBatch> pipelineBatches; // keyed by pipeline entity id

	flecs::world& ecs;

	flecs::system buildRenderBatchesSys;

	flecs::entity renderPhase;

	flecs::query<Transform, MeshComponent>queryEntID;
	flecs::query<EditorMesh>editorVisualsQuery;

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

		registerPhase();
		registerSystems();

		pipelineLib.init();

		LogSuccess(LOG_RENDER, "Renderer Initialized");
	}


	void initSubSystems() {

		initPhysicsRenderer();

		overlay.init();

		LogSuccess(LOG_RENDER, "Renderer SubSystems Initialized");

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
			LogError(LOG_RENDER, "SDL_Init failed: %s\n", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create window!");
			return false;
		}

		SDL_SetWindowPosition(renderContext.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		//show cursor
		SDL_SetWindowRelativeMouseMode(renderContext.window, false);

		return true;
	}


	bool createAndClaimGPU() {

		//TODO debug mode should come from a config file
		bool debugMode = true;

		// Enable shaderDrawParameters via Vulkan 1.1 features struct
		VkPhysicalDeviceVulkan11Features features11 = {};
		features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		features11.pNext = NULL; 
		features11.shaderDrawParameters = VK_TRUE;

		SDL_GPUVulkanOptions vulkanProps = { 0 };
		vulkanProps.vulkan_api_version = VK_MAKE_API_VERSION(0, 1, 3, 0); //Vulkan 1.3
		vulkanProps.feature_list = &features11;

		SDL_PropertiesID props = SDL_CreateProperties();

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
			LogError(LOG_RENDER, "SDL_CreateGPUDeviceWithProperties failed: %s", SDL_GetError());
			return false;
		}

		if (!SDL_ClaimWindowForGPUDevice(renderContext.device, renderContext.window))
		{
			LogError(LOG_RENDER, "SDL_ClaimWindowForGPUDevice failed");
			return false;
		}

		//cleanup
		SDL_DestroyProperties(props);

		// SDL_GPU_PRESENTMODE_IMMEDIATE for uncapped fps
		// SDL_GPU_PRESENTMODE_VSYNC for VSYNC
		SDL_SetGPUSwapchainParameters(renderContext.device, renderContext.window,
			SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

		return true;
	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity renderPhaseDependency = ecs.entity("RenderPhaseDependency");

		renderPhase = ecs.entity("RenderPhase")
			.add(flecs::Phase)
			.depends_on(renderPhaseDependency);

	}

	void registerSystems() {

		createRenderBatchesSystem();
		createPhysicsBatchesSystem();
		renderPhysicsSystem();

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
			LogError(LOG_RENDER, "Could not create GPU sampler!");
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
			LogError(LOG_RENDER, "Could not create nearestSampler !");
			return false;
		}

		// Load the image
		SDL_Surface* imageData1 = RenderUtil::LoadImage("assets/checkerboard.bmp", 4);
		if (imageData1 == NULL)
		{
			LogError(LOG_RENDER, "Could not load checkerboard.bmp image data!");
			return false;
		}

		MaterialLoader::createGPUTexture(imageData1, defaultTexture, "defaultTexture", renderContext.device);


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
			LogError(LOG_RENDER, "Failed to create main color target: %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create main resolve target: %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create depth texture: %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create entIdColorTarget : %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create depth texture: %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create selectedEntColorTarget : %s", SDL_GetError());
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
			LogError(LOG_RENDER, "Failed to create depth texture: %s", SDL_GetError());
			return false;
		}

		return true;
	}


	void buildRenderQueries() {

		queryEntID = ecs.query_builder<Transform, MeshComponent>()
		.build();

		editorVisualsQuery = ecs.query_builder<EditorMesh>()
			//.with<RenderPipeline>(flecs::Wildcard)
			//.group_by<RenderPipeline>()
			.build();
	}



	void drawAll() {

		const RenderContext& renderContext = ecs.get<RenderContext>();
		FrameContext& frameContext = ecs.get_mut<FrameContext>();
		const RenderConfig& config = ecs.get<RenderConfig>();

		beginRenderPass(renderContext, frameContext);

		drawAllBatches();

		//Keeping this part of the main render Pass because they look/work better
#if defined(JPH_DEBUG_RENDERER)
		drawPhysicsBodiesSys.run();
#endif

		SDL_EndGPURenderPass(activeRenderPass);

		drawEditorVisuals(frameContext, renderContext, config);

		blitToSwapchain(frameContext, config);

		ui.drawUI();

		SDL_SubmitGPUCommandBuffer(frameContext.commandBuffer);
	}


	void beginRenderPass(const RenderContext& renderContext, FrameContext& frameContext) {

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
		activeRenderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			targetInfos, 1,
			&depthStencilTargetInfo
		);

		defaultSamplerBinding = { .texture = defaultTexture, .sampler = defaultSampler };

		// Maybe have an observer that runs each time camera is switched so that we don't query which cam is active every frame
		ecs.query<Camera, ActiveCamera>()
			.each([&](Camera& cam, ActiveCamera) {

			uniforms.view = cam.generateView();
			uniforms.projection = cam.generateProj();
			uniforms.viewProjection = cam.generateViewProj();
			//Transpose because matrix layout in memory for Slang is is row-major
			uniforms.viewProjection = glm::transpose(uniforms.viewProjection);

		});

		SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 0, &uniforms, sizeof(uniforms));
	}

	
	void createRenderBatchesSystem() {

		buildRenderBatchesSys = ecs.system<Transform, MeshComponent, Renderable>("BuildRenderBatchesSys")
			.with(ecs.id<RenderPipeline>(), flecs::Wildcard)
			.group_by<RenderPipeline>()
			.kind(renderPhase)
			.run([&](flecs::iter& it) {

			AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
			const RenderContext& renderContext = ecs.get<RenderContext>();

			//clear previous batch
			for (auto& [pipelineId, pipelineBatch] : pipelineBatches) {
				for (auto& [meshIndex, drawBatch] : pipelineBatch.meshBatches) {
					drawBatch.transforms.clear();
					drawBatch.numIndices = 0;
				}
			}

			while (it.next()) {

				auto transforms = it.field<Transform>(0);
				auto meshComponents = it.field<MeshComponent>(1);
				flecs::entity pipelineEntity = flecs::entity(it.world(), it.group_id());

				//For each entity put all of it meshes in a batch
				for (auto i : it) {

					//calculate Ent model matrix
					glm::mat4 modelMat = createModelMatrix(transforms[i]);

					//put this outside of this loop
					PipelineBatch& pipelineBatch = pipelineBatches[(uint32_t)pipelineEntity.id()];

					for (uint32_t index : meshComponents[i].MeshAssetIndices) {

						MeshAsset& asset = assetLib->meshRegistry[index];

						glm::mat4 localMat = createModelMatrix(asset.transform);
						localMat = modelMat * localMat;
						//Transpose because matrix layout in memory for Slang is is row-major
						localMat = glm::transpose(localMat);

						// batch key is a combination of pipelineEntity ID and index of meshComponent
						DrawBatch& batch = pipelineBatch.meshBatches[index];

						batch.vertexBuffer = asset.vertexBuffer;
						batch.indexBuffer = asset.indexBuffer;
						batch.numIndices = asset.numIndices;

						batch.diffuseTexture = asset.diffuseTexture;

						// Accumulate transforms across all entities
						batch.transforms.push_back(localMat);
					}
				}
			}

			//upload buffer data
			for (auto& [pipelineId, pipelineBatch] : pipelineBatches) {

				for (auto& [meshIndex, drawBatch] : pipelineBatch.meshBatches) {

					if (drawBatch.transforms.empty()) continue;

					//Frees prevoius frames buffer.(prevents memory leak)
					if (drawBatch.transformsBuffer != nullptr) {
						SDL_ReleaseGPUBuffer(renderContext.device, drawBatch.transformsBuffer);
						drawBatch.transformsBuffer = nullptr;
					}

					//TODO maybe one buffer for the entire transforms and
					RenderUtil::uploadBufferData(
						renderContext.device,
						drawBatch.transformsBuffer,
						drawBatch.transforms.data(),
						drawBatch.transforms.size() * sizeof(glm::mat4),
						SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
					);

				}
			}
		});

	}

	void drawAllBatches() {
		for (auto& [pipelineId, pipelineBatch] : pipelineBatches) {

			// Bind pipeline ONCE
			flecs::entity pipelineEntity = ecs.entity(pipelineId);
			const Pipeline* pipeline = &pipelineEntity.get<Pipeline>();
			SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline->pipelineMS);
			SDL_BindGPUFragmentSamplers(activeRenderPass, 0, &defaultSamplerBinding, 1);

			// Draw each mesh batch under this pipeline
			for (auto& [meshIndex, batch] : pipelineBatch.meshBatches) {

				//If mesh has a texture then 
				if (batch.diffuseTexture != nullptr) {
					SDL_GPUTextureSamplerBinding sampler = { .texture = batch.diffuseTexture, .sampler = defaultSampler };
					SDL_BindGPUFragmentSamplers(activeRenderPass, 0, &sampler, 1);
				}
				//else use the default texture
				else {
					SDL_BindGPUFragmentSamplers(activeRenderPass, 0, &defaultSamplerBinding, 1);
				}

				SDL_GPUBufferBinding vbBinding{ .buffer = batch.vertexBuffer, .offset = 0 };
				SDL_GPUBufferBinding ibBinding{ .buffer = batch.indexBuffer,  .offset = 0 };
				SDL_BindGPUVertexBuffers(activeRenderPass, 0, &vbBinding, 1);
				SDL_BindGPUIndexBuffer(activeRenderPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
				SDL_BindGPUVertexStorageBuffers(activeRenderPass, 0, &batch.transformsBuffer, 1);

				SDL_DrawGPUIndexedPrimitives(
					activeRenderPass,
					batch.numIndices,
					batch.transforms.size(), // all instances of this mesh
					0, 0, 0
				);
			}
		}
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
			.kind(renderPhase)
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

				fisiksRenderer.setCameraUniforms(camPos, cam.generateView(), cam.generateProj());

			});

			// The following are updated every frame so we pass them to fisiksRenderer here instead of during construction
			fisiksRenderer.renderPass = activeRenderPass;

			// actually draws
			fisiksRenderer.drawAll();

		});
#endif

	}

	void drawMeshWithID(const FrameContext& frameContext, MeshAsset& mesh, uint32_t entID, glm::mat4& modelMat) {

		glm::mat4 localMat = createModelMatrix(mesh.transform);

		localMat = modelMat * localMat;
		localMat = glm::transpose(localMat);

		// Reversed multiplication order (Mᵀ × VPᵀ) because both matrices are pre-transposed for Slang's row-major layout.
		// This is equivalent to (VP × M)ᵀ, which the GPU interprets correctly as model transform followed by view-projection.
		glm::mat4 mvp = localMat * uniforms.viewProjection;

		SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 1, &mvp, sizeof(mvp));

		SDL_GPUBufferBinding vbBinding{ .buffer = mesh.vertexBuffer, .offset = 0 };
		SDL_GPUBufferBinding ibBinding{ .buffer = mesh.indexBuffer,  .offset = 0 };
		SDL_BindGPUVertexBuffers(activeRenderPass, 0, &vbBinding, 1);
		SDL_BindGPUIndexBuffer(activeRenderPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		SDL_PushGPUFragmentUniformData(frameContext.commandBuffer, 0, &entID, sizeof(entID));

		SDL_DrawGPUIndexedPrimitives(
			activeRenderPass,
			mesh.numIndices,
			1, // all instances of this mesh
			0, 0, 0
		);

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

		activeRenderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);

		
		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID");
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();

		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;

		SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline.pipeline);

		queryEntID.each([&](flecs::entity entity, Transform & transform, MeshComponent & meshComponent) {

			uint32_t entID = (uint32_t)entity.id();

			//calculate Ent model matrix
			glm::mat4 modelMat = createModelMatrix(transform);

			for (uint32_t index : meshComponent.MeshAssetIndices) {

				MeshAsset& mesh = assetLib->meshRegistry[index];

				drawMeshWithID(frameContext, mesh, entID, modelMat);
			}

		});
		
		SDL_EndGPURenderPass(activeRenderPass);
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

		activeRenderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);

		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID"); //TODO maybe use a different pipeline
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();

		SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline.pipeline);

		AssetLibrary * assetLib = ecs.get<AssetLibRef>().assetLib;

		const Transform& transform = selectedEnt.get<Transform>();
		const MeshComponent& meshcomponent = selectedEnt.get<MeshComponent>();

		glm::mat4 modelMat = createModelMatrix(transform);

		uint32_t entID = (uint32_t)selectedEnt.id();

		for (uint32_t meshIndex : meshcomponent.MeshAssetIndices) {

			MeshAsset& mesh = assetLib->meshRegistry[meshIndex];

			drawMeshWithID(frameContext, mesh, entID, modelMat);

			SDL_EndGPURenderPass(activeRenderPass);
		}
		
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


		SDL_BindGPUComputeStorageTextures(computePass, 0, &selectedEntColorTarget, 1);

		// Push outline color uniform
		struct OutlineParams {
			glm::vec4 color;
			float thickness;
			uint32_t entId;
			uint32_t screenWidth;   
			uint32_t screenHeight;  
		} params;

		params.color = glm::vec4(1.0f, 0.70f, 0.0f, 1.0f); //orange
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

		activeRenderPass = SDL_BeginGPURenderPass(
			frameContext.commandBuffer,
			&colorTargetInfo, 1,
			&depthStencilTargetInfo
		);

		//TODO group by pipeline.
		editorVisualsQuery.each([&](flecs::entity e, EditorMesh em) {

			flecs::entity pipelineEnt = e.target<RenderPipeline>();
			const Pipeline pipeline = pipelineEnt.get<Pipeline>();

			SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline.pipeline);

			const MeshInstance & meshInstance  = e.get<MeshInstance>();
			
			SDL_GPUBufferBinding vbBinding{ .buffer = meshInstance.vertexBuffer, .offset = 0 };
			SDL_BindGPUVertexBuffers(activeRenderPass, 0, &vbBinding, 1);

			//edge case for xyz lines
			if (e.try_get<LineVertices>()) {
				SDL_DrawGPUPrimitives(activeRenderPass, e.get<LineVertices>().data.size(), 1, 0, 0);

			}
			else {

				const Transform & transform =  e.get<Transform>();

				glm::mat4 localMat = createModelMatrix(meshInstance.transform);
				glm::mat4 modelMat = createModelMatrix(transform);

				localMat = modelMat * localMat;
				localMat = glm::transpose(localMat);

				// Reversed multiplication order (Mᵀ × VPᵀ) because both matrices are pre-transposed for Slang's row-major layout.
				// This is equivalent to (VP × M)ᵀ, which the GPU interprets correctly as model transform followed by view-projection.
				glm::mat4 mvp = localMat * uniforms.viewProjection;

				SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 1, &mvp, sizeof(mvp));

				SDL_GPUBufferBinding ibBinding{ .buffer = meshInstance.indexBuffer,  .offset = 0 };
				SDL_BindGPUIndexBuffer(activeRenderPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

				SDL_DrawGPUIndexedPrimitives(activeRenderPass, meshInstance.numIndices, 1, 0, 0, 0);

			}

		});

		SDL_EndGPURenderPass(activeRenderPass);

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
		SDL_DownloadFromGPUTexture(copyPass, &textureRegion, &textureTransferInfo);


		SDL_EndGPUCopyPass(copyPass);

		SDL_GPUFence* fence =  SDL_SubmitGPUCommandBufferAndAcquireFence(copyCommandBuffer);

		// TODO Maybe do this in a background thread
		SDL_WaitForGPUFences(renderContext.device,true, &fence, 1);

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

	glm::mat4 createModelMatrix(const Transform& transform) {

		glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
		glm::mat4 modelRotation = glm::toMat4(transform.rotation);
		glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
		return modelTranslation * modelRotation * modelScale;
	}

};
