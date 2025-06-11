#pragma once 

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
//#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <glad/glad.h>
#include <iostream>
#include <fstream>

#include <vector>
#include <array>

//#include "../../../out/build/x64-debug/_deps/slang-src/source/core/slang-string-escape-util.h"
//#include "slang-com-ptr.h"
//#include <slang.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../ecs/components.hpp"




using std::cout;

/*
// Vertex structure
struct Vertex {
    float x, y;     // Position
    float r, g, b;  // Color
};

*/


#define RETURN_ON_FAIL(x) \
    {                     \
        auto _res = x;    \
        if (_res != 0)    \
        {                 \
            return -1;    \
        }                 \
    }


struct Window {

public:


        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;
        SDL_GPUDevice* device = NULL;

        SDL_GPUGraphicsPipeline* pipeline = NULL;
        SDL_GPUBuffer* vertexBuffer = NULL;

        SDL_Texture* texture = NULL;
        TTF_Font* font = NULL;

        Window(int width, int height) {

            //init();
            //createWindow(width,height);
            SDL_AppInit(width, height);

            loadStuff();
        }

        int init() {

            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                std::cerr << "Failed to initialize SDL: " << SDL_GetError() << '\n';
                return 1;
            }

            // Set OpenGL version to 4.6 Core
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        }

        int createWindow(int width, int height) {



            window = SDL_CreateWindow(
                "Synthwave",                  // window title
                width,                               // width, in pixels
                height,                               // height, in pixels
                SDL_WINDOW_OPENGL                  // flags - see below
            );

            if (!window) {
                std::cerr << "Failed to create window: " << SDL_GetError() << '\n';
                return 1;
            }

            SDL_GLContext glContext = SDL_GL_CreateContext(window);
            if (!glContext) {
                std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << '\n';
                return 1;
            }

            // Load OpenGL functions with GLAD
            if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
                std::cerr << "Failed to initialize GLAD\n";
                return 1;
            }

            SDL_SetWindowRelativeMouseMode(window, true);

            return 0;
        }

        SDL_AppResult SDL_AppInit(int width, int height)
        {
            SDL_Color color = { 255, 255, 255, SDL_ALPHA_OPAQUE };
            SDL_Surface* text;

            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                printf("SDL_Init failed: %s\n", SDL_GetError());
                return SDL_APP_FAILURE;
            }

            device = SDL_CreateGPUDevice(
                SDL_GPU_SHADERFORMAT_SPIRV,
                true,
                NULL);

            if (!device)
            {
                SDL_Log("GPUCreateDevice failed");
                return SDL_APP_FAILURE;
            }

            /* Create the window */
            window = SDL_CreateWindow("Synthwave", width, height, SDL_WINDOW_VULKAN);

            if (!SDL_ClaimWindowForGPUDevice(device, window))
            {
                SDL_Log("GPUClaimWindow failed");
                SDL_APP_FAILURE;
            }



            if (!TTF_Init()) {
                SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
                return SDL_APP_FAILURE;
            }
            /* Open the font */
            font = TTF_OpenFont("assets/fonts/Supermolot Light.otf", 43);
            if (!font) {
                SDL_Log("Couldn't open font: %s\n", SDL_GetError());
                return SDL_APP_FAILURE;
            }

            /*
            //Create the text
            text = TTF_RenderText_Blended(font, "Hello World!", 0, color);
            if (text) {
                texture = SDL_CreateTextureFromSurface(renderer, text);
                SDL_DestroySurface(text);
            }
            if (!texture) {
                SDL_Log("Couldn't create text: %s\n", SDL_GetError());
                return SDL_APP_FAILURE;

            }
            */

            return SDL_APP_CONTINUE;

        }
     
        SDL_AppResult loadStuff() {
            
            
            

            SDL_GPUShader* vertexShader = load_VertexShader(device, "shaders/spv/RawTriangle.vert.spv", 0, 0, 0, 0);
            if (vertexShader == NULL)
            {
                SDL_Log("Failed to create vertex shader!");
                return SDL_APP_FAILURE;
            }
           
            
            SDL_GPUShader* fragmentShader = load_FragmentShader(device,"shaders/spv/SolidColor.frag.spv" ,0, 0, 0, 0);
            if (fragmentShader == NULL)
            {
                SDL_Log("Failed to create fragment shader!");
                return SDL_APP_FAILURE;
            }
            
           
            
            
            // Create vertex buffer
            if (!createVertexBuffer("assets/cube.obj")) {
                return SDL_APP_FAILURE;
            }

            // Create graphics pipeline
            if (!createPipeline(vertexShader,fragmentShader)) {
                return SDL_APP_FAILURE;
            }
            


            return SDL_APP_CONTINUE;
        }

        SDL_GPUShader* load_VertexShader(
            SDL_GPUDevice* device,
            const std::string& filename,
            Uint32 sampler_count,
            Uint32 uniform_buffer_count,
            Uint32 storage_buffer_count,
            Uint32 storage_texture_count)
        {
            // Load the SPIR-V bytecode from file
            std::vector<Uint8> spirvCode = loadBinaryFile(filename);
            if (spirvCode.empty()) {
                std::cerr << "Failed to load vertex shader: " << filename << std::endl;
                return nullptr;
            }

            SDL_GPUShaderCreateInfo createinfo = {};
            createinfo.num_samplers = sampler_count;
            createinfo.num_storage_buffers = storage_buffer_count;
            createinfo.num_storage_textures = storage_texture_count;
            createinfo.num_uniform_buffers = uniform_buffer_count;
            createinfo.props = 0;
            createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            createinfo.code = spirvCode.data();
            createinfo.code_size = spirvCode.size();
            createinfo.entrypoint = "main";  // SPIR-V typically uses "main" as entry point
            createinfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

            //SDL_GPUShader* shader = SDL_CreateGPUShader(device, &createinfo);
           

            return SDL_CreateGPUShader(device, &createinfo);
        }

        SDL_GPUShader* load_FragmentShader(
            SDL_GPUDevice* device,
            const std::string& filename,
            Uint32 sampler_count,
            Uint32 uniform_buffer_count,
            Uint32 storage_buffer_count,
            Uint32 storage_texture_count)
        {
            // Load the SPIR-V bytecode from file
            std::vector<Uint8> spirvCode = loadBinaryFile(filename);
            if (spirvCode.empty()) {
                std::cerr << "Failed to load fragment shader: " << filename << std::endl;
                return nullptr;
            }

            SDL_GPUShaderCreateInfo createinfo = {};
            createinfo.num_samplers = sampler_count;
            createinfo.num_storage_buffers = storage_buffer_count;
            createinfo.num_storage_textures = storage_texture_count;
            createinfo.num_uniform_buffers = uniform_buffer_count;
            createinfo.props = 0;
            createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            createinfo.code = spirvCode.data();
            createinfo.code_size = spirvCode.size();
            createinfo.entrypoint = "main";  // SPIR-V typically uses "main" as entry point
            createinfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

            return SDL_CreateGPUShader(device, &createinfo);
        }

        /*
        bool createVertexBuffer() {
            // Define triangle vertices (position + color)
            std::vector<Vertex> vertices = {
                { 0.0f,  0.5f, 1.0f, 0.0f, 0.0f},  // Top vertex - Red
                {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f},  // Bottom left - Green
                { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f}   // Bottom right - Blue
            };

            SDL_GPUBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bufferCreateInfo.size = vertices.size() * sizeof(Vertex);

            vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
            if (!vertexBuffer) {
                std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
                return false;
            }

            // Upload vertex data
            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
            transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size = bufferCreateInfo.size;

            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
            if (!transferBuffer) {
                std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
                return false;
            }

            // Map and copy data
            void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
            if (!mappedData) {
                std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                return false;
            }

            memcpy(mappedData, vertices.data(), vertices.size() * sizeof(Vertex));
            SDL_UnmapGPUTransferBuffer(device, transferBuffer);

            // Copy from transfer buffer to vertex buffer
            SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

            SDL_GPUTransferBufferLocation transferLocation = {};
            transferLocation.transfer_buffer = transferBuffer;
            transferLocation.offset = 0;

            SDL_GPUBufferRegion bufferRegion = {};
            bufferRegion.buffer = vertexBuffer;
            bufferRegion.offset = 0;
            bufferRegion.size = bufferCreateInfo.size;

            SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return true;
        }
        */



        bool createVertexBuffer(const char* filePath) {

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
                return false;
            }

            cout << "processsing model : " << filePath << '\n';

            aiMesh* importedMesh = scene->mMeshes[0];


            std::vector<VertexData> vertices;
            std::vector <unsigned int> indices;

            vertices.reserve(importedMesh->mNumVertices);

            for (int i = 0; i < importedMesh->mNumVertices; i++) {

                vertices.emplace_back();
                VertexData& currentVertex = vertices.back();

                currentVertex.vertex.x = importedMesh->mVertices[i].x;
                currentVertex.vertex.y = importedMesh->mVertices[i].y;
                currentVertex.vertex.z = importedMesh->mVertices[i].z;

                if (importedMesh->HasNormals()) {
                    currentVertex.normal.x = importedMesh->mNormals[i].x;
                    currentVertex.normal.y = importedMesh->mNormals[i].y;
                    currentVertex.normal.z = importedMesh->mNormals[i].z;
                }

                //Mesh can have multiple texture coordinates we're just using the first one for now.
                importedMesh->mTextureCoords;
                if (importedMesh->mTextureCoords[0])
                {
                    currentVertex.texCoords.x = importedMesh->mTextureCoords[0][i].x;
                    currentVertex.texCoords.y = importedMesh->mTextureCoords[0][i].y;
                }

                indices.reserve(importedMesh->mNumFaces * 3);

                for (int i = 0; i < importedMesh->mNumFaces; i++) {


                    for (int j = 0; j < importedMesh->mFaces[i].mNumIndices; j++) {
                        indices.emplace_back(importedMesh->mFaces[i].mIndices[j]);
                    }


                }

            }

          

            SDL_GPUBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            bufferCreateInfo.size = vertices.size() * sizeof(VertexData);

            vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
            if (!vertexBuffer) {
                std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
                return false;
            }

            // Upload vertex data
            SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
            transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferBufferCreateInfo.size = bufferCreateInfo.size;

            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
            if (!transferBuffer) {
                std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
                return false;
            }

            // Map and copy data
            void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
            if (!mappedData) {
                std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
                SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
                return false;
            }

            memcpy(mappedData, vertices.data(), vertices.size() * sizeof(VertexData));
            SDL_UnmapGPUTransferBuffer(device, transferBuffer);

            // Copy from transfer buffer to vertex buffer
            SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

            SDL_GPUTransferBufferLocation transferLocation = {};
            transferLocation.transfer_buffer = transferBuffer;
            transferLocation.offset = 0;

            SDL_GPUBufferRegion bufferRegion = {};
            bufferRegion.buffer = vertexBuffer;
            bufferRegion.offset = 0;
            bufferRegion.size = bufferCreateInfo.size;

            SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return true;
        }


        bool createPipeline(SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
            // Note: In a real application, you would need pre-compiled shaders
            // This is a simplified example showing the structure

            // Vertex input state
            SDL_GPUVertexAttribute vertexAttributes[2] = {};

            // Position attribute
            vertexAttributes[0].location = 0;
            vertexAttributes[0].buffer_slot = 0;
            vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
            vertexAttributes[0].offset = offsetof(VertexData, vertex);

            // Color attribute
            vertexAttributes[1].location = 1;
            vertexAttributes[1].buffer_slot = 0;
            vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
            vertexAttributes[1].offset = offsetof(VertexData, texCoords);

            SDL_GPUVertexBufferDescription vertexBufferDescription = {};
            vertexBufferDescription.slot = 0;
            vertexBufferDescription.pitch = sizeof(VertexData);
            vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

            SDL_GPUVertexInputState vertexInputState = {};
            vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
            vertexInputState.num_vertex_buffers = 1;
            vertexInputState.vertex_attributes = vertexAttributes;
            vertexInputState.num_vertex_attributes = 2;

            // Create pipeline
            SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {};
            pipelineCreateInfo.vertex_input_state = vertexInputState;
            pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

            // Rasterizer state
            SDL_GPURasterizerState rasterizerState = {};
            rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
            rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
            pipelineCreateInfo.rasterizer_state = rasterizerState;

            // Color blend state
            SDL_GPUColorTargetBlendState colorTargetBlendState = {};
            colorTargetBlendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

            SDL_GPUColorTargetDescription colorTargetDescription = {};
            colorTargetDescription.format = SDL_GetGPUSwapchainTextureFormat(device, window);
            colorTargetDescription.blend_state = colorTargetBlendState;

            pipelineCreateInfo.target_info.num_color_targets = 1;
            pipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDescription;


            pipelineCreateInfo.vertex_shader = vertexShader;
            pipelineCreateInfo.fragment_shader = fragmentShader;

            pipeline = SDL_CreateGPUGraphicsPipeline(device,& pipelineCreateInfo);

            return true;
        }

        void render() {
            
            
            
            // Begin command buffer
            SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
            if (!commandBuffer) {
                std::cerr << "Failed to acquire command buffer: " << SDL_GetError() << std::endl;
                return;
            }

            // Acquire swapchain texture
            SDL_GPUTexture* swapchainTexture;
            if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr)) {
                std::cerr << "Failed to acquire swapchain texture: " << SDL_GetError() << std::endl;
                return;
            }

            if (!swapchainTexture) {
                return; // Window is probably minimized
            }


            // Color target info
            SDL_GPUColorTargetInfo colorTargetInfo = {};
            colorTargetInfo.texture = swapchainTexture;
            colorTargetInfo.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black background
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            // Begin render pass
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
                commandBuffer,
                &colorTargetInfo, 1,
                nullptr // No depth target
            );

            if (pipeline) {
                // Bind pipeline and draw (if shaders were properly created)
                SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

                SDL_GPUBufferBinding vertexBufferBinding = {};
                vertexBufferBinding.buffer = vertexBuffer;
                vertexBufferBinding.offset = 0;

                SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
                SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
            }

            // End render pass
            SDL_EndGPURenderPass(renderPass);

            // Submit command buffer
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }



        // Helper function to load binary file data
        std::vector<Uint8> loadBinaryFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::binary | std::ios::ate);

            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filename << std::endl;
                return {};
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<Uint8> buffer(size);
            if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                std::cerr << "Failed to read file: " << filename << std::endl;
                return {};
            }

            return buffer;
        }


        

        void destroy() {

            SDL_DestroyWindow(window);
            SDL_Quit();
        }
};



