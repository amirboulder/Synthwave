#pragma once 

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <vector>
#include <array>

#include "slang-com-ptr.h"
#include <slang.h>
#include "slang-com-helper.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../ecs/components.hpp"


using std::cout;



//#define RETURN_ON_FAIL(x) \
//    {                     \
//        auto _res = x;    \
//        if (_res != 0)    \
//        {                 \
//            return -1;    \
//        }                 \
//    }
//
//
//
//void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
//{
//    if (diagnosticsBlob != nullptr)
//    {
//        std::cout << (const char*)diagnosticsBlob->getBufferPointer() << std::endl;
//    }
//}
//
//
//struct Window {
//
//public:
//
//	MeshSource mesh;
//
//	SDL_Window* window = NULL;
//	SDL_Renderer* renderer = NULL;
//	SDL_GPUDevice* device = NULL;
//
//	SDL_GPUGraphicsPipeline* pipeline = NULL;
//	SDL_GPUBuffer* vertexBuffer = NULL;
//	SDL_GPUBuffer* indexBuffer = NULL;
//
//	SDL_GPUTexture* depthTexture = NULL;
//
//	SDL_Texture* texture = NULL;
//	TTF_Font* font = NULL;
//
//	int width;
//	int height;
//
//	float rotation = 0.0f;
//	float counter = 0;
//
//	Window(int width, int height)
//		: width(width), height(height)
//	{
//
//		init(width, height);
//
//
//	}
//
//	bool init(int width, int height)
//	{
//
//		if (!SDL_Init(SDL_INIT_VIDEO)) {
//			printf("SDL_Init failed: %s\n", SDL_GetError());
//			return 1;
//		}
//
//		window = SDL_CreateWindow("Synthwave", width, height, SDL_WINDOW_VULKAN);
//		SDL_GetWindowSizeInPixels(window, &width, &height);
//
//		device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
//		if (!device)
//		{
//			SDL_Log("GPUCreateDevice failed: %s", SDL_GetError());
//			return false;
//		}
//
//		if (!SDL_ClaimWindowForGPUDevice(device, window))
//		{
//			SDL_Log("GPUClaimWindow failed");
//			return false;
//		}
//
//
//		//int testw, testh;
//		//cout << width << "  " << height << '\n';
//		//SDL_GetWindowSize(window, &testw, &testh);
//		//cout << "window width : " << testw << " window height " << testh << "\n";
//
//
//
//		//if (!TTF_Init()) {
//		//    SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
//		//    return SDL_APP_FAILURE;
//		//}
//		//font = TTF_OpenFont("assets/fonts/Supermolot Light.otf", 43);
//		//if (!font) {
//		//    SDL_Log("Couldn't open font: %s\n", SDL_GetError());
//		//    return SDL_APP_FAILURE;
//		//}
//
//		/*
//		//Create the text
//		text = TTF_RenderText_Blended(font, "Hello World!", 0, color);
//		if (text) {
//			texture = SDL_CreateTextureFromSurface(renderer, text);
//			SDL_DestroySurface(text);
//		}
//		if (!texture) {
//			SDL_Log("Couldn't create text: %s\n", SDL_GetError());
//			return SDL_APP_FAILURE;
//
//		}
//		*/
//
//		// const int numGpus = SDL_GetNumGPUDrivers();
//		 //SDL_Log("Found %d GPU drivers.", numGpus);
//
//		return true;
//
//	}
//
//
//
//	bool loadStuff() {
//
//
//		generateSpirvShaders();
//
//
//		SDL_GPUShader* vertexShader = load_VertexShader(device, "shaders/compiled/VertexShader.spv", 0, 2, 0, 0);
//		if (vertexShader == NULL) {
//			SDL_Log("Failed to create vertex shader!");
//			return false;
//		}
//
//		SDL_GPUShader* fragmentShader = load_FragmentShader(device, "shaders/compiled/FragmentShader.spv", 0, 0, 0, 0);
//		if (fragmentShader == NULL) {
//			SDL_Log("Failed to create fragment shader!");
//			return false;
//		}
//
//		SDL_Log("Successfully loaded the shaders");
//
//
//
//		// Create vertex buffer
//		if (!createVertexBuffer("assets/capsule4.gls")) {
//			return false;
//		}
//		SDL_Log("Successfully created the vertex buffer");
//
//	
//		if (!createPipeline(vertexShader, fragmentShader)) {
//			return false;
//		}
//		SDL_Log("Successfully created graphics pipeline");
//
//
//
//		return true;
//	}
//
//
//	SDL_GPUShader* load_VertexShader(
//		SDL_GPUDevice* device,
//		const std::string& filename,
//		Uint32 sampler_count,
//		Uint32 uniform_buffer_count,
//		Uint32 storage_buffer_count,
//		Uint32 storage_texture_count)
//	{
//		// Load the SPIR-V bytecode from file
//		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
//		if (spirvCode.empty()) {
//			std::cerr << "Failed to load vertex shader: " << filename << std::endl;
//			return nullptr;
//		}
//
//		SDL_GPUShaderCreateInfo createinfo = {};
//		createinfo.num_samplers = sampler_count;
//		createinfo.num_storage_buffers = storage_buffer_count;
//		createinfo.num_storage_textures = storage_texture_count;
//		createinfo.num_uniform_buffers = uniform_buffer_count;
//		createinfo.props = 0;
//		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
//		createinfo.code = spirvCode.data();
//		createinfo.code_size = spirvCode.size();
//		createinfo.entrypoint = "main";
//		createinfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
//
//		//SDL_GPUShader* shader = SDL_CreateGPUShader(device, &createinfo);
//
//
//		return SDL_CreateGPUShader(device, &createinfo);
//	}
//
//	SDL_GPUShader* load_FragmentShader(
//		SDL_GPUDevice* device,
//		const std::string& filename,
//		Uint32 sampler_count,
//		Uint32 uniform_buffer_count,
//		Uint32 storage_buffer_count,
//		Uint32 storage_texture_count)
//	{
//		// Load the SPIR-V bytecode from file
//		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
//		if (spirvCode.empty()) {
//			std::cerr << "Failed to load fragment shader: " << filename << std::endl;
//			return nullptr;
//		}
//
//		SDL_GPUShaderCreateInfo createinfo = {};
//		createinfo.num_samplers = sampler_count;
//		createinfo.num_storage_buffers = storage_buffer_count;
//		createinfo.num_storage_textures = storage_texture_count;
//		createinfo.num_uniform_buffers = uniform_buffer_count;
//		createinfo.props = 0;
//		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
//		createinfo.code = spirvCode.data();
//		createinfo.code_size = spirvCode.size();
//		createinfo.entrypoint = "main";
//		createinfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
//
//		return SDL_CreateGPUShader(device, &createinfo);
//	}
//
//
//	bool createVertexBuffer(const char* filePath) {
//
//		// std::vector<glm::vec3> vertices;
//		 //std::vector <unsigned int> indices;
//
//		Assimp::Importer importer;
//		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
//
//		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//		{
//			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
//			return false;
//		}
//		cout << "processsing model : " << filePath << '\n';
//
//		if (scene->mNumMeshes == 0) {
//			std::cerr << "No meshes found in file." << std::endl;
//			return false;
//		}
//
//		aiMesh* importedMesh = scene->mMeshes[0];
//
//		mesh.vertices.reserve(importedMesh->mNumVertices);
//
//		float r = 0.0f;
//		float g = 0.0f;
//		float b = 0.0f;
//
//		int count = 1;
//
//		for (int i = 0; i < importedMesh->mNumVertices; i++) {
//
//			mesh.vertices.emplace_back();
//			VertexData & currentVertex = mesh.vertices.back();
//
//			currentVertex.vertex.x = importedMesh->mVertices[i].x;
//			currentVertex.vertex.y = importedMesh->mVertices[i].y;
//			currentVertex.vertex.z = importedMesh->mVertices[i].z;
//
//			if (importedMesh->HasNormals()) {
//				currentVertex.normal.x = importedMesh->mNormals[i].x;
//				currentVertex.normal.y = importedMesh->mNormals[i].y;
//				currentVertex.normal.z = importedMesh->mNormals[i].z;
//			}
//
//			//Mesh can have multiple texture coordinates we're just using the first one for now.
//			//cout << "texture coords" << importedMesh->HasTextureCoords(0) << '\n';
//			if (importedMesh->HasTextureCoords(0) || importedMesh->mTextureCoords[0])
//			{
//				currentVertex.texCoords.x = importedMesh->mTextureCoords[0][i].x;
//				currentVertex.texCoords.y = importedMesh->mTextureCoords[0][i].y;
//			}
//
//			if (count > 3) {
//				count = 1;
//				r = 0;
//				g = 0;
//				b = 0;
//
//			}
//
//			if (count ==  1) {
//				r = 1.0;
//				g = 0.0;
//				b = 0.0;
//			}
//
//			if (count == 2) {
//				r = 0.0;
//				g = 1.0;
//				b = 0.0;
//			}
//
//			if (count  == 3) {
//				r = 0.0;
//				g = 0.0;
//				b = 1.0;
//			}
//
//			currentVertex.color.x = r;
//			currentVertex.color.y = g;
//			currentVertex.color.z = b;
//			currentVertex.color.w = 1.0f;
//
//			count++;
//		}
//
//
//		mesh.indices.reserve(importedMesh->mNumFaces * 3);
//
//		for (int i = 0; i < importedMesh->mNumFaces; i++) {
//
//			for (int j = 0; j < importedMesh->mFaces[i].mNumIndices; j++) {
//				mesh.indices.emplace_back(importedMesh->mFaces[i].mIndices[j]);
//			}
//
//		}
//
//
//		cout << "size of vertices : " << mesh.vertices.size() << "\n";
//		cout << "size of indices : " << mesh.indices.size() << "\n";
//
//		//create vertex buffer
//		SDL_GPUBufferCreateInfo bufferCreateInfo = {};
//		bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
//		bufferCreateInfo.size = mesh.vertices.size() * sizeof(VertexData);
//
//		vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
//		if (!vertexBuffer) {
//			std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
//			return false;
//		}
//
//		//create index buffer
//		SDL_GPUBufferCreateInfo idxCreateInfo = {};
//		idxCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
//		idxCreateInfo.size = mesh.indices.size() * sizeof(unsigned int);
//
//		indexBuffer = SDL_CreateGPUBuffer(device, &idxCreateInfo);
//		if (!indexBuffer) {
//			std::cerr << "Failed to create index buffer: " << SDL_GetError() << std::endl;
//			return false;
//		}
//
//		// Upload vertex data
//		SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
//		transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
//		transferBufferCreateInfo.size = bufferCreateInfo.size;
//
//		// upload index data:
//		SDL_GPUTransferBufferCreateInfo idxTransferInfo = {};
//		idxTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
//		idxTransferInfo.size = idxCreateInfo.size;
//
//
//		SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
//		if (!transferBuffer) {
//			std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
//			return false;
//		}
//
//		SDL_GPUTransferBuffer* idxTransfer = SDL_CreateGPUTransferBuffer(device, &idxTransferInfo);
//		void* idxMap = SDL_MapGPUTransferBuffer(device, idxTransfer, false);
//		memcpy(idxMap, mesh.indices.data(), idxCreateInfo.size);
//		SDL_UnmapGPUTransferBuffer(device, idxTransfer);
//
//		// Copy into GPU buffer
//		auto cmdBufIndex = SDL_AcquireGPUCommandBuffer(device);
//		auto copyPassIndex = SDL_BeginGPUCopyPass(cmdBufIndex);
//		SDL_GPUTransferBufferLocation srcLoc{ idxTransfer, 0 };
//		SDL_GPUBufferRegion dstRegion{ indexBuffer, 0, idxCreateInfo.size };
//		SDL_UploadToGPUBuffer(copyPassIndex, &srcLoc, &dstRegion, false);
//		SDL_EndGPUCopyPass(copyPassIndex);
//		SDL_SubmitGPUCommandBuffer(cmdBufIndex);
//		SDL_ReleaseGPUTransferBuffer(device, idxTransfer);
//
//
//		// Map and copy data
//		void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
//		if (!mappedData) {
//			std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
//			SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
//			return false;
//		}
//
//		memcpy(mappedData, mesh.vertices.data(), mesh.vertices.size() * sizeof(VertexData));
//		SDL_UnmapGPUTransferBuffer(device, transferBuffer);
//
//		// Copy from transfer buffer to vertex buffer
//		SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
//		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);
//
//		SDL_GPUTransferBufferLocation transferLocation = {};
//		transferLocation.transfer_buffer = transferBuffer;
//		transferLocation.offset = 0;
//
//		SDL_GPUBufferRegion bufferRegion = {};
//		bufferRegion.buffer = vertexBuffer;
//		bufferRegion.offset = 0;
//		bufferRegion.size = bufferCreateInfo.size;
//
//		SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
//		SDL_EndGPUCopyPass(copyPass);
//		SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
//
//		SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
//		return true;
//	}
//
//
//	int loadVertexBuffer() {
//
//	}
//
//	bool createPipeline(SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
//
//
//		// Vertex input state
//		SDL_GPUVertexAttribute vertexAttributes[4] = {};
//
//		// Position attribute
//		vertexAttributes[0].location = 0;
//		vertexAttributes[0].buffer_slot = 0;
//		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
//		vertexAttributes[0].offset = 0;
//
//		// normal attribute
//		vertexAttributes[1].location = 1;
//		vertexAttributes[1].buffer_slot = 0;
//		vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
//		vertexAttributes[1].offset = sizeof(glm::vec3);
//
//		// texture Coordinates attribute
//		vertexAttributes[2].location = 2;
//		vertexAttributes[2].buffer_slot = 0;
//		vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
//		vertexAttributes[2].offset = sizeof(glm::vec3)  + sizeof(glm::vec3);
//
//		// Color attribute
//		vertexAttributes[3].location = 3;
//		vertexAttributes[3].buffer_slot = 0;
//		vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
//		vertexAttributes[3].offset = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2);
//
//		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
//		vertexBufferDescription.slot = 0;
//		vertexBufferDescription.pitch = sizeof(VertexData);
//		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
//
//		SDL_GPUVertexInputState vertexInputState = {};
//		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
//		vertexInputState.num_vertex_buffers = 1;
//		vertexInputState.vertex_attributes = vertexAttributes;
//		vertexInputState.num_vertex_attributes = 4;
//
//
//
//		// Create pipeline
//		SDL_GPUColorTargetDescription coldescs = {};
//		coldescs.format = SDL_GetGPUSwapchainTextureFormat(device, window);
//
//		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
//		SDL_zero(pipeInfo);
//		pipeInfo.vertex_shader = vertexShader;
//		pipeInfo.fragment_shader = fragmentShader;
//		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
//
//		pipeInfo.target_info.color_target_descriptions = &coldescs;
//		pipeInfo.target_info.num_color_targets = 1;
//		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
//		pipeInfo.target_info.has_depth_stencil_target = true;
//
//		pipeInfo.depth_stencil_state = {
//		.compare_op = SDL_GPU_COMPAREOP_LESS,
//		.enable_depth_test = true,
//		.enable_depth_write = true,
//		};
//
//		pipeInfo.vertex_input_state = vertexInputState;
//
//		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
//		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
//		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
//		pipeInfo.props = 0;
//
//		pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
//		if (!pipeline) {
//			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
//			return false;
//		}
//
//		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
//		SDL_GPUGraphicsPipeline* pipelineLine = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
//		if (!pipelineLine) {
//			SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
//			return false;
//		}
//
//		SDL_Log("Built pipelines");
//
//		SDL_GPUTextureCreateInfo depthTextureCreateInfo = {
//		 .type = SDL_GPU_TEXTURETYPE_2D,
//		 .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
//		 .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
//		 .width = static_cast<uint32_t>(width),
//		 .height = static_cast<uint32_t>(height),
//		 .layer_count_or_depth = 1,
//		 .num_levels = 1,
//		 // .sample_count = msaaSampleCount, // Must match color target sample count
//		};
//		depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);
//
//		return true;
//	}
//
//
//	void simplePipeline(SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
//		// Vertex input state
//		SDL_GPUVertexAttribute vertexAttributes[1] = {};
//		vertexAttributes[0].location = 0;
//		vertexAttributes[0].buffer_slot = 0;
//		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
//		vertexAttributes[0].offset = 0;
//
//		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
//		vertexBufferDescription.slot = 0;
//		vertexBufferDescription.pitch = sizeof(glm::vec3);
//		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
//
//		SDL_GPUVertexInputState vertexInputState = {};
//		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
//		vertexInputState.num_vertex_buffers = 1;
//		vertexInputState.vertex_attributes = vertexAttributes;
//		vertexInputState.num_vertex_attributes = 1;
//
//		SDL_GPUColorTargetDescription coldescs = {};
//		coldescs.format = SDL_GetGPUSwapchainTextureFormat(device, window);
//
//		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
//		SDL_zero(pipeInfo);
//		pipeInfo.vertex_shader = vertexShader;
//		pipeInfo.fragment_shader = fragmentShader;
//		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
//
//		pipeInfo.target_info.color_target_descriptions = &coldescs;
//		pipeInfo.target_info.num_color_targets = 1;
//		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
//		pipeInfo.target_info.has_depth_stencil_target = true;
//
//		pipeInfo.depth_stencil_state = {
//		.compare_op = SDL_GPU_COMPAREOP_LESS,
//		.enable_depth_test = true,
//		.enable_depth_write = true,
//		};
//
//		pipeInfo.vertex_input_state = vertexInputState;
//
//		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
//		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
//		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
//		pipeInfo.props = 0;
//
//		pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
//		if (!pipeline) {
//			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
//			return;
//		}
//
//		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
//		SDL_GPUGraphicsPipeline* pipelineLine = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
//		if (!pipelineLine) {
//			SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
//			return;
//		}
//
//		SDL_Log("Built pipelines");
//
//		SDL_GPUTextureCreateInfo depthTextureCreateInfo = {
//		 .type = SDL_GPU_TEXTURETYPE_2D,
//		 .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
//		 .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
//		 .width = static_cast<uint32_t>(width),
//		 .height = static_cast<uint32_t>(height),
//		 .layer_count_or_depth = 1,
//		 .num_levels = 1,
//		 // .sample_count = msaaSampleCount, // Must match color target sample count
//		};
//		depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);
//
//
//	}
//
//
//
//
//
//	void render() {
//
//		// Begin command buffer
//		SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
//		if (!commandBuffer) {
//			std::cerr << "Failed to acquire command buffer: " << SDL_GetError() << std::endl;
//			return;
//		}
//
//		// Acquire swapchain texture
//		SDL_GPUTexture* swapchainTexture;
//		if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr)) {
//			std::cerr << "Failed to acquire swapchain texture: " << SDL_GetError() << std::endl;
//			return;
//		}
//
//		if (!swapchainTexture) {
//			return; // Window is probably minimized
//		}
//
//		// Render the 3D Scene (Color and Depth pass)
//		float nearPlane = 0.1f;
//		float farPlane = 100.0f;
//
//		float fov = 45.0f;
//		float aspect = static_cast<float>(width) / static_cast<float>(height);
//
//		glm::mat4 model = glm::mat4(1.0f);
//		model = glm::translate(model, glm::vec3(1.0f, 1.0f, 12.0f + counter));
//		model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 1.0f));
//
//		rotation = fmod(rotation + 0.6f, 360.0f);
//
//		counter += 0.01f;
//
//		glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
//		glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 1.0f);
//		glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
//
//
//		glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
//		glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(fov), aspect, nearPlane, farPlane);
//
//	    proj[1][1] *= -1; // Flip Y for Vulkan
//
//
//
//		glm::mat4 MVP = proj * view * model;
//
//		//glm::mat4x4 inverseTransposeModelTransform = inverse(transpose(MVP));
//
//		MVP = glm::transpose(MVP);
//
//
//
//
//		// Color target info
//		SDL_GPUColorTargetInfo colorTargetInfo = {};
//		colorTargetInfo.texture = swapchainTexture;
//		colorTargetInfo.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black background
//		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
//		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
//
//		SDL_GPUDepthStencilTargetInfo depthTargetInfo = { 0 };
//		depthTargetInfo.texture = depthTexture;
//		depthTargetInfo.clear_depth = 1.0f;
//		depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
//		depthTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
//
//
//		// Begin render pass
//		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
//			commandBuffer,
//			&colorTargetInfo, 1,
//			&depthTargetInfo // No depth target
//		);
//
//		// Bind pipeline and draw (if shaders were properly created)
//		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
//
//
//
//		SDL_GPUBufferBinding vertexBufferBinding = {};
//		vertexBufferBinding.buffer = vertexBuffer;
//		vertexBufferBinding.offset = 0;
//
//		SDL_GPUBufferBinding indexBufferBinding = {};
//		indexBufferBinding.buffer = indexBuffer;
//		indexBufferBinding.offset = 0;
//
//		SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
//		SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
//
//		SDL_PushGPUVertexUniformData(commandBuffer, 0, &counter, sizeof(counter));
//
//		SDL_PushGPUVertexUniformData(commandBuffer, 1, &MVP, sizeof(MVP));
//
//
//		// SDL_DrawGPUPrimitives(renderPass, vertexCount, 1, 0, 0);
//
//		SDL_DrawGPUIndexedPrimitives(renderPass, mesh.indices.size(), 1, 0, 0, 0);
//
//		// End render pass
//		SDL_EndGPURenderPass(renderPass);
//
//		// Submit command buffer
//		SDL_SubmitGPUCommandBuffer(commandBuffer);
//	}
//
//
//
//	// Helper function to load binary file data
//	std::vector<Uint8> loadBinaryFile(const std::string& filename) {
//		std::ifstream file(filename, std::ios::binary | std::ios::ate);
//
//		if (!file.is_open()) {
//			std::cerr << "Failed to open file: " << filename << std::endl;
//			return {};
//		}
//
//		std::streamsize size = file.tellg();
//		file.seekg(0, std::ios::beg);
//
//		std::vector<Uint8> buffer(size);
//		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
//			std::cerr << "Failed to read file: " << filename << std::endl;
//			return {};
//		}
//
//		return buffer;
//	}
//
//	int generateSpirvShaders() {
//
//		// STEP 1: Create the Global Session
//		Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
//		RETURN_ON_FAIL(slang::createGlobalSession(slangGlobalSession.writeRef()));
//		printf("Created slangGlobalSession \n");
//
//
//		// STEP 2: Configure the Compilation Session
//		slang::SessionDesc sessionDesc = {};
//
//		// TargetDesc specifies the output format and version
//		slang::TargetDesc targetDesc = {};
//		targetDesc.format = SLANG_SPIRV;              // Output SPIRV bytecode (for Vulkan)
//		targetDesc.profile = slangGlobalSession->findProfile("spirv_1_3");
//		targetDesc.flags = 0;
//
//
//
//		// Link the target to the session
//		sessionDesc.targets = &targetDesc;
//		sessionDesc.targetCount = 1;
//		sessionDesc.compilerOptionEntryCount = 0;
//		// Create the actual compilation session
//		Slang::ComPtr<slang::ISession> session;
//		RETURN_ON_FAIL(slangGlobalSession->createSession(sessionDesc, session.writeRef()));
//
//		printf("Created compilation session \n");
//
//
//
//		Slang::ComPtr<slang::IBlob> diagnosticsBlob;
//		const char* shaderPath = "shaders/slang/shaders.slang";
//		slang::IModule* slangModule =
//			session->loadModule(shaderPath, diagnosticsBlob.writeRef());
//		diagnoseIfNeeded(diagnosticsBlob);
//		if (!slangModule)
//			return SLANG_FAIL;
//
//		printf("Created module \n");
//
//
//
//		// STEP 4: Find Entry Points
//		// An entry point is a specific function that serves as the "main" for a shader stage
//		Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint;
//		RETURN_ON_FAIL(
//			slangModule->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef()));
//		//
//		Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint;
//		RETURN_ON_FAIL(
//			slangModule->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef()));
//
//
//		// STEP 5: Compose the Program
//		// Combine the module and entry point into a composite program
//		// This creates the final executable shader program
//		Slang::ComPtr<slang::IComponentType> composedProgram;
//		slang::IComponentType* components[] = { slangModule, vertexEntryPoint,fragmentEntryPoint };
//		RETURN_ON_FAIL(session->createCompositeComponentType(
//			components,
//			3,  // number of components
//			composedProgram.writeRef()
//		));
//
//
//		// STEP 6: Generate SPIRV Bytecode
//		// Extract the compiled SPIRV code from the composed program
//		Slang::ComPtr<slang::IBlob> spirvVertexShaderCode;
//		Slang::ComPtr<slang::IBlob> diagnosticsVertexShader;
//
//		Slang::ComPtr<slang::IBlob> spirvFragmentShaderCode;
//		Slang::ComPtr<slang::IBlob> diagnostics2;
//
//		SlangResult resultVertex = composedProgram->getEntryPointCode(
//			0,                      // entry point index (we only have one)
//			0,                      // target index (we only have one target)
//			spirvVertexShaderCode.writeRef(),
//			diagnosticsVertexShader.writeRef()
//		);
//
//		SlangResult resultFragment = composedProgram->getEntryPointCode(
//			1,                      // entry point index (we only have one)
//			0,                      // target index (we only have one target)
//			spirvFragmentShaderCode.writeRef(),
//			diagnostics2.writeRef()
//		);
//
//		// Check for code generation errors
//		if (diagnosticsVertexShader && diagnosticsVertexShader->getBufferSize() > 0) {
//			printf("Code generation diagnostics:\n%s\n",
//				(const char*)diagnosticsVertexShader->getBufferPointer());
//		}
//
//		if (SLANG_FAILED(resultVertex) || SLANG_FAILED(resultFragment)) {
//			printf("Failed to generate SPIRV code\n");
//			return -1;
//		}
//
//		if (!spirvFragmentShaderCode || spirvFragmentShaderCode->getBufferSize() == 0) {
//			printf("No SPIRV fragment shader code generated\n");
//			return -1;
//		}
//
//		if (!spirvVertexShaderCode || spirvVertexShaderCode->getBufferSize() == 0) {
//			printf("No SPIRV code generated\n");
//			return -1;
//		}
//
//		// STEP 7: Output the SPIRV Bytecode
//		printf("Successfully generated SPIRV code: %zu bytes\n", spirvVertexShaderCode->getBufferSize());
//		printf("Successfully generated SPIRV fragment shader code: %zu bytes\n", spirvFragmentShaderCode->getBufferSize());
//
//		const char* outputPathVS = "shaders/compiled/Vertexshader.spv";
//		const char* outputPathFS = "shaders/compiled/Fragmentshader.spv";
//
//
//		std::ofstream outFileVS(outputPathVS, std::ios::out | std::ios::binary);
//		std::ofstream outFileFS(outputPathFS, std::ios::out | std::ios::binary);
//
//		// Ensure the directory exists
//		if (!std::filesystem::exists(outputPathVS) || !std::filesystem::exists(outputPathFS)) {
//			// std::filesystem::create_directories(dir);
//			cout << "directory " << outputPathVS << " does not exis\n";
//			cout << "directory " << outputPathFS << " does not exis\n";
//		}
//
//
//		// Write SPIRV binary data to files
//		outFileVS.write(
//			static_cast<const char*>(spirvVertexShaderCode->getBufferPointer()),
//			spirvVertexShaderCode->getBufferSize()
//		);
//
//		outFileFS.write(
//			static_cast<const char*>(spirvFragmentShaderCode->getBufferPointer()),
//			spirvFragmentShaderCode->getBufferSize()
//		);
//
//
//		printf("Successfully created spriv files!\n");
//
//		return 0;
//	}
//
//
//
//	void destroy() {
//
//		SDL_DestroyWindow(window);
//		SDL_Quit();
//	}
//};
//


