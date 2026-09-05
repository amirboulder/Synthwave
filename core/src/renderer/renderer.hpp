#pragma once

#include "../util/util.hpp"

#include "Texture.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

#include "ProceduralMeshes.hpp"

#include "renderUtil.hpp"

#include "GeometryPool.hpp"

#include "../physics/physics.hpp"

#include "UI/UserInterface.hpp"
#include "PipelineLibrary/PipelineLibrary.hpp"

#include "RendererConfig.hpp"
#include "Camera.hpp"
#include "text/textRenderer.hpp"
#include "overlay/overlay.hpp"


#include "pipeline.hpp"

#include "../AssetSystems/Manifest.hpp"
#include "../AssetSystems/AssetManager.hpp"


// One per unique mesh instance — shared by all its submeshes
struct GPUPerMeshData {
	glm::mat4 worldMatrix;
	glm::mat4 normalMatrix;
};

// One per draw call — submesh specific
struct GPUPerSubMeshData {
	uint32_t instanceOffset;   // → indexes into GPUInstanceData buffer
	uint32_t materialIndex;
	uint32_t _pad[2];
};


struct DrawItem {

	uint32_t vertexOffset = UINT32_MAX;
	uint32_t firstIndex = UINT32_MAX;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	uint32_t meshID = 0;
	uint32_t meshEntityID = 0;
	uint32_t materialIndex = 0;
	uint64_t pipelineID = 0;

	glm::mat4 transformMatrix;
	glm::mat4 normalMatrix;
};

struct PipelineBatch {
	uint64_t pipelineID;
	uint32_t firstDrawCommand;  // index into drawCommands
	uint32_t drawCommandCount;
};

//TODO move to its own file
struct GrowableGPUBuffer {
	SDL_GPUBuffer* buffer = nullptr;
	size_t capacity = 0;      // bytes currently allocated on GPU

	// Uploads data, reallocating only if needed
	void upload(SDL_GPUDevice* device, const void* data, size_t size, Uint32 usage) {
		if (size > capacity) {
			if (buffer) SDL_ReleaseGPUBuffer(device, buffer);

			// Grow with headroom (e.g. 1.5x) to reduce future reallocations
			capacity = size + size / 2;
			LogWarn(LOG_RENDER, "GrowableGPUBuffer has grown to new capacity %d", capacity);

			SDL_GPUBufferCreateInfo info = {};
			info.usage = usage;
			info.size = (Uint32)capacity;
			buffer = SDL_CreateGPUBuffer(device, &info);
		}

		// Reuse existing transfer buffer or create one per upload
		// (see batched upload note below)
		SDL_GPUTransferBufferCreateInfo transferInfo = {};
		transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferInfo.size = (Uint32)size;
		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

		void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
		memcpy(mapped, data, size);
		SDL_UnmapGPUTransferBuffer(device, transferBuffer);

		SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
		SDL_GPUTransferBufferLocation src = { transferBuffer, 0 };
		SDL_GPUBufferRegion dst = { buffer, 0, (Uint32)size };
		SDL_UploadToGPUBuffer(pass, &src, &dst, false);
		SDL_EndGPUCopyPass(pass);
		SDL_SubmitGPUCommandBuffer(cmd);
		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
	}

	void release(SDL_GPUDevice* device) {
		if (buffer) { SDL_ReleaseGPUBuffer(device, buffer); buffer = nullptr; capacity = 0; }
	}
};


struct LightDataUniform {
	glm::vec3 ambientColor = glm::vec3(1.0f, 0.96f, 0.88f);
	float ambientIntensity = 0.1f;

	uint32_t numDirectionalLights;
	uint32_t numPointLights;
};

struct LightBatch {
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight>       pointLights;

	GrowableGPUBuffer directionalBuffer;
	GrowableGPUBuffer pointBuffer;

	uint32_t numDirectional = 0;
	uint32_t numPoint = 0;

	GrowableGPUBuffer dummyBufferDir;
	GrowableGPUBuffer dummyBufferPoint;

	//This is needed because in  SDL_GPU you cannot bind a null buffer, it will error.
	// You need something valid in the slot.
	void initDummyBuffers(flecs::world& ecs) {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		DirectionalLight directionalDummy{};
		dummyBufferDir.upload(renderContext.device, &directionalDummy, sizeof(directionalDummy), SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);

		DirectionalLight pointDummy{};
		dummyBufferPoint.upload(renderContext.device, &pointDummy, sizeof(pointDummy), SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);
	}
};



struct Renderer {

	Uint32 swapchainWidth, swapchainHeight;

	SDL_GPUSampler* linearSampler = nullptr;
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

	std::vector<PipelineBatch> pipelineBatches;

	SDL_GPUGraphicsPipeline* defaultPipeline = nullptr;

	std::vector<GPUPerMeshData> perMeshData;
	std::vector<GPUPerSubMeshData> perSubMeshData;

	//List of DrawItems after culling before duplicates are instanced
	std::vector<DrawItem> drawItems;

	std::vector<SDL_GPUIndexedIndirectDrawCommand> drawCommands;//Instanced list of drawCommands
	std::vector<Material> materialDatas;


	GrowableGPUBuffer perMeshDataBuffer;
	GrowableGPUBuffer perSubMeshDataBuffer;
	GrowableGPUBuffer allMaterialsBuffer;
	GrowableGPUBuffer drawCommandBuffer;
	uint32_t numDrawCalls = 0 ;

	LightBatch lightBatch;

	flecs::world& ecs;

	flecs::system createDrawBatchesSys;

	flecs::entity renderPhase;

	flecs::query<Transform, MeshComponent>queryEntID;

	flecs::query<Light, DirectionalLight>dirLightQuery;
	flecs::query<Light, PointLight>pointLightQuery;

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
		createAndClaimGPUVulkan();
		createRenderTargets();

		createSamplers();

		ui.init();

		buildRenderQueries();

		registerPhase();
		registerSystems();

		pipelineLib.init();


		getDefaultPipeline();

		LogSuccess(LOG_RENDER, "Renderer Initialized");
	}


	void initSubSystems() {

		initPhysicsRenderer();

		overlay.init();

		//needed so SDL_GPU don't complain about empty buffer being uploaded
		lightBatch.initDummyBuffers(ecs);

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

		//TODO REMOVE HARDCODED path
		RenderConfig::loadRendererConfigINIFile(ecs, "config/renderConfig.ini");
	}

	bool createWindow() {

		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
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


	bool createAndClaimGPUVulkan() {

		//TODO debug mode should come from a config file
		bool debugMode = true;

		// Enable shaderDrawParameters via Vulkan 1.1 features struct.
		VkPhysicalDeviceVulkan11Features features11 = {};
		features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		features11.pNext = NULL;
		features11.shaderDrawParameters = VK_TRUE;

		const char* requiredDeviceExtensions[] = {
			VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME,
		};

		SDL_GPUVulkanOptions vulkanProps = { 0 };
		vulkanProps.vulkan_api_version = VK_MAKE_API_VERSION(0, 1, 3, 0); //Vulkan 1.3
		vulkanProps.feature_list = &features11;
		vulkanProps.device_extension_count = SDL_arraysize(requiredDeviceExtensions);
		vulkanProps.device_extension_names = requiredDeviceExtensions;

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
		RenderConfig& config = ecs.get_mut<RenderConfig>();

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


		if (!SDL_WindowSupportsGPUPresentMode(renderContext.device, renderContext.window, config.presentMode)) {
			LogWarn(LOG_RENDER, "PresentMode unsupported, falling back to VSYNC");
			config.presentMode = SDL_GPU_PRESENTMODE_VSYNC;
		}

		// SDL_GPU_PRESENTMODE_IMMEDIATE for uncapped fps
		// SDL_GPU_PRESENTMODE_VSYNC for VSYNC
		if (!SDL_SetGPUSwapchainParameters(renderContext.device, renderContext.window,
			config.colorspace, config.presentMode)) {

			LogError(LOG_RENDER, "SDL_SetGPUSwapchainParameters failed: %s", SDL_GetError());
			return false;
		}

		LogInfo(LOG_RENDER, "created And Claimed GPU using Vulkan");

		return true;
	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity renderPhaseDependency = ecs.entity("RenderPhaseDependency");

		renderPhase = ecs.entity("RenderPhase").add(flecs::Phase).depends_on(renderPhaseDependency);
	}

	void registerSystems() {

		createDrawBatchesSystem();

		LogInfo(LOG_RENDER, "registered Rendering Systems");

	}

	//TODO remove default texture from here it should be a part of assetLib
	bool createSamplers() {

		const RenderContext& renderContext = ecs.get<RenderContext>();

		SDL_GPUSamplerCreateInfo samplerCreateInfo{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		};

		linearSampler = SDL_CreateGPUSampler(renderContext.device, &samplerCreateInfo);

		if (!linearSampler) {
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

		if (!nearestSampler) {
			LogError(LOG_RENDER, "Could not create nearestSampler !");
			return false;
		}

		LogDebug(LOG_RENDER, "created Samplers");

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

		LogDebug(LOG_RENDER, "created Render Targets");

		return true;
	}

	void getDefaultPipeline() {

		flecs::entity pipelineEntity = ecs.entity("pipelinePhong");
		const Pipeline* pipeline = &pipelineEntity.get<Pipeline>();

		defaultPipeline = pipeline->pipelineMS;
	}


	void buildRenderQueries() {

		queryEntID = ecs.query_builder<Transform, MeshComponent>()
		.build();

		dirLightQuery = ecs.query_builder<Light, DirectionalLight>()
			.build();

		pointLightQuery = ecs.query_builder<Light, PointLight>()
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

		drawLit(frameContext);

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


		// Maybe have an observer that runs each time camera is switched so that we don't query which cam is active every frame
		ecs.query<Camera, ActiveCamera>()
			.each([&](Camera& cam, ActiveCamera) {

			uniforms.view = cam.generateView();
			uniforms.projection = cam.generateProj();
			uniforms.viewProjection = cam.generateViewProj();
			//Transpose because matrix layout in memory for Slang is is row-major
			uniforms.viewProjection = glm::transpose(uniforms.viewProjection);
			uniforms.cameraPos = cam.position;

		});

		//Sending the frame data uniforms to both vertex and fragment shaders
		SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 0, &uniforms, sizeof(uniforms));
		SDL_PushGPUFragmentUniformData(frameContext.commandBuffer, 0, &uniforms, sizeof(uniforms));
	}



	/// <summary>
	/// This system creates all the buffer and batches needed for rendering every frame.
	/// It performs culling first (TODO).
	/// Then all each surviving mesh will be place in a drawItem (There are duplicates at this point)
	/// it then calls createDrawCommands() to sort and place all meshes buckets based on meshID (instancing)
	/// Then light batches are created
	/// </summary>
	void createDrawBatchesSystem() {

		ecs.system<SubMeshComponent, flecs::Parent>("CreateDrawBatchesSys")
			.kind(renderPhase)
			.run([&](flecs::iter& it) {

			const RenderContext& renderContext = ecs.get<RenderContext>();

			//clear previous batch
			perMeshData.clear();
			perSubMeshData.clear();
			drawCommands.clear();
			pipelineBatches.clear();
			drawItems.clear();
			materialDatas.clear();
			numDrawCalls = 0;

			while (it.next()) {

				auto subMeshes = it.field<SubMeshComponent>(0);
				auto parents = it.field<flecs::Parent>(1);

				flecs::entity parentEntPrev = ecs.entity(0);

				glm::mat4 worldMatrix;
				glm::mat4 normalMatrix;

				for (auto i : it) {

					flecs::entity parentEnt = it.world().entity(parents[i].value);
					const MeshComponent& parentMeshComp = parentEnt.get<MeshComponent>();

					if (!parentMeshComp.visible) {
						continue;
					}

					//If parent is different then fetch its data an cache it
					if (parentEntPrev != parentEnt) {

						parentEntPrev = parentEnt;
						worldMatrix = parentEnt.get<WorldMatrix>().matrix;

						// Compute normal matrix BEFORE transposing localMat for Slang.
						// Normal matrix = inverse transpose of the upper 3x3 of the model matrix.
						// Stored as glm::mat4 with 4th column zeroed to satisfy GPU 16-byte row alignment.
						glm::mat3 normalMat3 = glm::mat3(glm::transpose(glm::inverse(worldMatrix)));
						glm::mat4 normalMat4 = glm::mat4(normalMat3); // expands to mat4, 4th col = (0,0,0,1)
						normalMat4[3] = glm::vec4(0.0f);       // zero out 4th column explicitly

						// transpose matrices to row-major for Slang
						//TODO switch slang to COL-major.
						normalMatrix = glm::transpose(normalMat4);
						worldMatrix = glm::transpose(worldMatrix);
					}

					const SubMeshComponent& submeshComp = subMeshes[i];


					//const LODComponent& activeLOD = submeshComp.LODs[parentMeshComp.activeLOD];

					DrawItem drawItem;

					drawItem.firstIndex = submeshComp.firstIndex;
					drawItem.indexCount = submeshComp.indexCount;

					drawItem.vertexOffset = submeshComp.vertexOffset;
					drawItem.vertexCount = submeshComp.vertexCount;
					drawItem.meshID = parentMeshComp.index;
					drawItem.meshEntityID = static_cast<uint32_t>(parentEnt.id()); // we just need the lower 32bits
					drawItem.materialIndex = submeshComp.materialIndex;
					drawItem.pipelineID = submeshComp.pipelineID;

					drawItem.normalMatrix = normalMatrix;
					drawItem.transformMatrix = worldMatrix;

					drawItems.push_back(std::move(drawItem));

				}
			}

			createDrawCommands();
			createLightBatches();

			//upload everything to gpu
			if (!perMeshData.empty()){

				perMeshDataBuffer.upload(renderContext.device,
					perMeshData.data(),
					perMeshData.size() * sizeof(GPUPerMeshData),
					SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);

				perSubMeshDataBuffer.upload(renderContext.device,
					perSubMeshData.data(),
					perSubMeshData.size() * sizeof(GPUPerSubMeshData),
					SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);

				allMaterialsBuffer.upload(renderContext.device,
					materialDatas.data(),
					materialDatas.size() * sizeof(Material),
					SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ);

				drawCommandBuffer.upload(renderContext.device,
					drawCommands.data(),
					drawCommands.size() * sizeof(SDL_GPUIndexedIndirectDrawCommand),
					SDL_GPU_BUFFERUSAGE_INDIRECT);
			}

			});
	}


	void createDrawCommands() {

		std::sort(drawItems.begin(), drawItems.end(),
			[](const DrawItem& a, const DrawItem& b) {

			if (a.pipelineID != b.pipelineID) {   // primary: group by pipeline
				return a.pipelineID < b.pipelineID;
			}
			if (a.meshID != b.meshID) {           // secondary: keep instancing buckets
				return a.meshID < b.meshID;
			}
			return a.firstIndex < b.firstIndex;   // tertiary: submesh order
		});

		uint32_t lastMeshEntID = UINT32_MAX;
		uint32_t meshIndex = 0;
		for (uint32_t i = 0; i < drawItems.size(); i++) {

			const DrawItem& item = drawItems[i];

			bool newPipeline = (i == 0) || (drawItems[i - 1].pipelineID != item.pipelineID);
			bool newBatch = newPipeline
				|| (drawItems[i - 1].firstIndex != item.firstIndex);


			if (newBatch) {
				SDL_GPUIndexedIndirectDrawCommand cmd{};
				cmd.num_indices = item.indexCount;
				cmd.num_instances = 1;
				cmd.first_index = item.firstIndex;
				cmd.vertex_offset = item.vertexOffset;
				cmd.first_instance = 0;
				drawCommands.push_back(cmd);

				perSubMeshData.emplace_back(meshIndex, item.materialIndex);

				if (newPipeline) {
					pipelineBatches.push_back({ item.pipelineID, numDrawCalls, 0 });
				}
				pipelineBatches.back().drawCommandCount++;

				numDrawCalls++;
			}
			else {
				drawCommands.back().num_instances++;
			}


			if (drawItems[i].meshEntityID != lastMeshEntID) {

				lastMeshEntID = drawItems[i].meshID;

				GPUPerMeshData inst;
				inst.worldMatrix = item.transformMatrix;
				inst.normalMatrix = item.normalMatrix;
				perMeshData.push_back(inst);
				meshIndex++;
			}
		
		}

		// Materials
		AssetManager* am = ecs.get<AssetManagerRef>().assetManager;
		for (const Material& m : am->materials) {
			materialDatas.push_back(m);
		}
	}

	/// <summary>
	/// Creates a batch for each light type
	/// </summary>
	/// TODO see if there is a benefit to making this a system
	void createLightBatches() {

		lightBatch.directionalLights.clear();
		lightBatch.pointLights.clear();

		const RenderContext& renderContext = ecs.get<RenderContext>();

		dirLightQuery.each([&](flecs::entity e, const Light& light, DirectionalLight dirLight) {

				dirLight.direction = glm::normalize(dirLight.direction);
				lightBatch.directionalLights.emplace_back(dirLight);
		});

		pointLightQuery.each([&](flecs::entity e, const Light& light, PointLight pointLight) {

			lightBatch.pointLights.emplace_back(pointLight);
		});


		lightBatch.numDirectional = (uint32_t)lightBatch.directionalLights.size();
		lightBatch.numPoint = (uint32_t)lightBatch.pointLights.size();

		// Upload — no-ops if vectors are empty, no realloc if count didn't grow
		if (lightBatch.numDirectional > 0) {
			lightBatch.directionalBuffer.upload(
				renderContext.device,
				lightBatch.directionalLights.data(),
				lightBatch.numDirectional * sizeof(DirectionalLight),
				SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
			);
		}
		if (lightBatch.numPoint > 0) {
			lightBatch.pointBuffer.upload(
				renderContext.device,
				lightBatch.pointLights.data(),
				lightBatch.numPoint * sizeof(PointLight),
				SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ
			);
		}

	}

	
	/// <summary>
	/// Draw the scene with lighting 
	/// </summary>
	void drawLit(FrameContext& frameContext) {

	AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;
		GeometryPool& geometryPool = assetManger->geometryPool;


		if (geometryPool.numMeshes < 1 || perMeshData.size() < 1) {
			return;
		}

		//Bind default pipeline once , pipelines are sorted so that other pipeline come after this.
		SDL_BindGPUGraphicsPipeline(activeRenderPass, defaultPipeline);

		// Geometry mega buffers, bind once
		SDL_GPUBufferBinding vbBinding{ .buffer = geometryPool.megaVertexBuffer, .offset = 0 };
		SDL_GPUBufferBinding ibBinding{ .buffer = geometryPool.megaIndexBuffer,  .offset = 0 };
		SDL_BindGPUVertexBuffers(activeRenderPass, 0, &vbBinding, 1);
		SDL_BindGPUIndexBuffer(activeRenderPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		// Bind all transforms, NormalMatrices, and MaterialData
		SDL_BindGPUVertexStorageBuffers(activeRenderPass, 0, &perMeshDataBuffer.buffer, 1);
		SDL_BindGPUVertexStorageBuffers(activeRenderPass, 1, &perSubMeshDataBuffer.buffer, 1);

		//Push the ambient light data so there is always some light in the scene.
		LightDataUniform lightDataUniform;
		lightDataUniform.numDirectionalLights = lightBatch.numDirectional;
		lightDataUniform.numPointLights = lightBatch.numPoint;
		SDL_PushGPUFragmentUniformData(frameContext.commandBuffer, 1, &lightDataUniform, sizeof(lightDataUniform));


		SDL_GPUTextureSamplerBinding binding0 = { assetManger->textureArrays.diffuseTextures.textureArray, nearestSampler };
		SDL_BindGPUFragmentSamplers(activeRenderPass, 0, &binding0, 1);

		SDL_GPUTextureSamplerBinding binding1 = { assetManger->textureArrays.metallicRoughnessTextures.textureArray, nearestSampler };
		SDL_BindGPUFragmentSamplers(activeRenderPass, 1, &binding1, 1);

		SDL_GPUTextureSamplerBinding binding2 = { assetManger->textureArrays.normalTextures.textureArray, nearestSampler };
		SDL_BindGPUFragmentSamplers(activeRenderPass, 2, &binding2, 1);

		//Separate binding set for Samplers StorageBuffers so they both start binding at slot 0
		SDL_BindGPUFragmentStorageBuffers(activeRenderPass, 0, &allMaterialsBuffer.buffer, 1);

		// Always bind something — dummy buffer when empty
		//TODO we should just have a dummy light in the buffers instead
		if (lightBatch.numDirectional > 0) { 
			SDL_BindGPUFragmentStorageBuffers(activeRenderPass, 1, &lightBatch.directionalBuffer.buffer, 1);
		}
		else {
			SDL_BindGPUFragmentStorageBuffers(activeRenderPass, 1, &lightBatch.dummyBufferDir.buffer, 1);

		}

		if (lightBatch.numPoint > 0) { 
			SDL_BindGPUFragmentStorageBuffers(activeRenderPass, 2, &lightBatch.pointBuffer.buffer, 1);
		}
		else {
			SDL_BindGPUFragmentStorageBuffers(activeRenderPass, 2, &lightBatch.dummyBufferPoint.buffer, 1);
		}

		//TODO rest of the lights

		for (const PipelineBatch& batch : pipelineBatches) {

			if (batch.pipelineID > 0) {
				flecs::entity pe = ecs.entity(batch.pipelineID);            // 0 → resolve to pipelinePhong
				const Pipeline& p = pe.get<Pipeline>();
				SDL_BindGPUGraphicsPipeline(activeRenderPass, p.pipelineMS);
			}

			SDL_PushGPUVertexUniformData(frameContext.commandBuffer, /*slot*/1,
				&batch.firstDrawCommand, sizeof(uint32_t));

			SDL_DrawGPUIndexedPrimitivesIndirect(
				activeRenderPass,
				drawCommandBuffer.buffer,
				batch.firstDrawCommand * sizeof(SDL_GPUIndexedIndirectDrawCommand), // byte offset
				batch.drawCommandCount);
		}
	}


	void initPhysicsRenderer() {
#ifdef JPH_DEBUG_RENDERER

		ecs.emplace<fisiksDebugRenderer>(ecs);

		const RenderConfig& config = ecs.get<RenderConfig>();

		if (!config.RenderPhysics) {
			ecs.entity<fisiksDebugRenderer>().disable<fisiksDebugRenderer>();

		}

		// Part of physics so they are registered here.
		createPhysicsBatchesSystem();
		renderPhysicsSystem();
#endif
	}


	void createPhysicsBatchesSystem() {

#ifdef	JPH_DEBUG_RENDERER

		//This system is Part of physics phase because it happens first
		//allows systems to push data to the batches without them being cleared before rendering
		flecs::entity physicsPhase = ecs.lookup("PhysicsPhase");

		flecs::system createPhysicsBatchesSys = ecs.system<fisiksDebugRenderer>("CreatePhysicsBatchesSys")
			//.with<fisiksDebugRenderer>()
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(physicsPhase)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {

			//Clear all the old data
			fisiksRenderer.clearBatches();

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

			// Does not actually draw it just puts all render batches in vector so they can be drawn during a render pass
			physicsSystem.DrawBodies(fisiksRenderer.drawSettings, &fisiksRenderer);

			//physicsSystem.DrawConstraints(&fisiksRenderer);
			//physicsSystem.DrawConstraintLimits(&fisiksRenderer);
			//physicsSystem.DrawConstraintReferenceFrame(&fisiksRenderer);

			fisiksRenderer.createLineBatch();

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
	

	/// <summary>
	/// Draws entity ID, assumes entity is in the GeometryPool.
	/// </summary>
	void drawMeshWithID(const FrameContext& frameContext, const GeometryPool& geometryPool, const MeshComponent& mesh, uint32_t entID, glm::mat4 modelMat) {

		//TODO change this later once the shader is updated to take in transforms and inverseMatrices buffer
		//SDL_BindGPUVertexStorageBuffers(activeRenderPass, 0, &allTransformsBuffer.buffer, 1);

		AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;

		modelMat = glm::transpose(modelMat);
		// Reversed multiplication order (Mᵀ × VPᵀ) because both matrices are pre-transposed for Slang's row-major layout.
		// This is equivalent to (VP × M)ᵀ, which the GPU interprets correctly as model transform followed by view-projection.
		glm::mat4 mvp = modelMat * uniforms.viewProjection;


		SDL_GPUBufferBinding vbBinding{ .buffer = geometryPool.megaVertexBuffer, .offset = 0 };
		SDL_GPUBufferBinding ibBinding{ .buffer = geometryPool.megaIndexBuffer,  .offset = 0 };
		SDL_BindGPUVertexBuffers(activeRenderPass, 0, &vbBinding, 1);
		SDL_BindGPUIndexBuffer(activeRenderPass, &ibBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 1, &mvp, sizeof(mvp));
		SDL_PushGPUFragmentUniformData(frameContext.commandBuffer, 1, &entID, sizeof(entID));


		//TODO no need to draw one at a time here since the data is in the geomtery buffer 
		//FIX FIX FIX !!!
		for (size_t i = 0; i < mesh.subMeshCount; i++) {

			const SubMeshComponent& subMesh = assetManger->subMeshes[mesh.firstSubMeshIndex + i];

			SDL_DrawGPUIndexedPrimitives(
				activeRenderPass,
				subMesh.indexCount,   // indexCount
				1,                 // instanceCount
				subMesh.firstIndex,   // firstIndex  — element index into megaIndexBuffer
				subMesh.vertexOffset,   // vertexOffset — element index into megaVertexBuffer
				0                  // firstInstance
			);
		}
	}

	/// <summary>
	/// Draws entity ids to entIdColorTarget.
	/// </summary>
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

		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;
		
		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID");
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();


		SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline.pipeline);

		//This function should be updated to use the megabuffer but it also needs to render all the
		// non (game) renderable meshes (camera, physics triggers etc.) to the entIdColorTarget but those are not in the mega buffer.
		//TODO create a second megabuffer for all of those
		//TODO replace MeshComponent with MeshAsset
		queryEntID.each([&](flecs::entity entity, Transform & transform, MeshComponent & meshComponent) {

			uint32_t entID = (uint32_t)entity.id();

			//calculate Ent model matrix
			glm::mat4 modelMat = createModelMatrix(transform);

			drawMeshWithID(frameContext, assetManager->geometryPool, meshComponent, entID, modelMat);

		});
		
		SDL_EndGPURenderPass(activeRenderPass);
	}

	
	/// <summary>
	/// Draws selected Entity to selectedEntColorTarget an calls applyOutlineComputePass.
	/// </summary> 
	/// TODO handle multiple selected entities
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

		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;

		flecs::entity pipelineEnt = ecs.lookup("pipelineEntID"); //TODO maybe use a different pipeline
		const Pipeline& pipeline = pipelineEnt.get<Pipeline>();

		SDL_BindGPUGraphicsPipeline(activeRenderPass, pipeline.pipeline);

		//AssetLibrary * assetLib = ecs.get<AssetLibRef>().assetLib;

		const Transform& transform = selectedEnt.get<Transform>();
		//replace MeshComponent with MeshAsset
		const MeshComponent& meshcomponent = selectedEnt.get<MeshComponent>();

		glm::mat4 modelMat = createModelMatrix(transform);

		uint32_t entID = (uint32_t)selectedEnt.id();


		drawMeshWithID(frameContext, assetManager->geometryPool, meshcomponent, entID, modelMat);
		
		SDL_EndGPURenderPass(activeRenderPass);

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

			const MeshStandalone & meshInstance  = e.get<MeshStandalone>();
			
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

				SDL_DrawGPUIndexedPrimitives(activeRenderPass, meshInstance.indexCount, 1, 0, 0, 0);

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


	void PrintGPUDeviceInfo(SDL_GPUDevice* device)
	{
		//
		SDL_SetLogPriority(LOG_RENDER, SDL_LOG_PRIORITY_INFO);

		if (!device) {
			LogError(LOG_RENDER, "Cannot print GPU info: device is null");
			return;
		}

		SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);
		if (!props) {
			LogError(LOG_RENDER, "SDL_GetGPUDeviceProperties failed: %s", SDL_GetError());
			return;
		}

		const char* deviceName = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_NAME_STRING, "Unknown");
		const char* driverName = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, "Unknown");
		const char* driverVersion = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, "Unknown");
		const char* driverInfo = SDL_GetStringProperty(props, SDL_PROP_GPU_DEVICE_DRIVER_INFO_STRING, "");

		LogInfo(LOG_RENDER, "=== GPU Device Information ===");
		LogInfo(LOG_RENDER, "Device Name      : %s", deviceName);
		LogInfo(LOG_RENDER, "Driver           : %s", driverName);
		LogInfo(LOG_RENDER, "Driver Version   : %s", driverVersion);

		if (driverInfo && driverInfo[0] != '\0') {
			LogInfo(LOG_RENDER, "Driver Info      : %s", driverInfo);
		}


		SDL_SetLogPriority(LOG_RENDER, SDL_LOG_PRIORITY_WARN);
	}
};
