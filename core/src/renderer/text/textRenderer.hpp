#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "../../util/util.hpp"

#include "../pipeline.hpp"
#include "../Shader.hpp"
#include "../renderUtil.hpp"

//TODO move this somewhere else
struct VertexData2 {
    glm::vec3 position; // POSITION
    glm::vec2 texCoords; // TEXCOORD0  
    glm::vec4 color;    // COLOR0
};

struct TextData {

    SDL_GPUBuffer* vertexBuffer = NULL;
    SDL_GPUBuffer* indexBuffer = NULL;

    TTF_GPUAtlasDrawSequence* seq = NULL;

    TTF_Text* text = NULL;
};

class TextRenderer {

public:

  

    std::string fontPath = "assets/fonts/Supermolot Light.otf";

    SDL_GPUGraphicsPipeline* pipeline = NULL;
    
    SDL_GPUShader* vertexShader = NULL;
    SDL_GPUShader* fragmentShader = NULL;

    SDL_GPUSampler* sampler = NULL;

    SDL_GPURenderPass* renderPass = NULL;

    TTF_Font* font = NULL;
    TTF_TextEngine* engine = NULL;

    TextData staticText;
    TextData positionText;
    TextData fpsText ;

    glm::mat4 ortho;

    bool fps = false;


    float f = 0.0f;
    int counter = 12;

    flecs::world& ecs;

    TextRenderer(flecs::world& ecs)
        :ecs(ecs)
    {

      

    }



    //create a custom pipeline for text renderer
    void init() {

        const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

       // shader::generateSpirvShaders("shaders/slang/textShader.slang", "shaders/compiled/text.vert.spv", "shaders/compiled/text.frag.spv");

        RenderUtil::loadShaderSPRIV(rendercontext.device, vertexShader, "shaders/compiled/text.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,0, 1, 0, 0);
        RenderUtil::loadShaderSPRIV(rendercontext.device, fragmentShader, "shaders/compiled/text.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT,1, 0, 0, 0);

       
        SDL_GPUVertexAttribute vertexAttributes[3] = {};

        // Position - location 0 
        vertexAttributes[0].location = 0;
        vertexAttributes[0].buffer_slot = 0;
        vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        vertexAttributes[0].offset = offsetof(VertexData2, position);

       
        // Color - location 1
        vertexAttributes[1].location = 1;
        vertexAttributes[1].buffer_slot = 0;
        vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        vertexAttributes[1].offset = offsetof(VertexData2, color);

        // TexCoords - location 2
        vertexAttributes[2].location = 2;
        vertexAttributes[2].buffer_slot = 0;
        vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        vertexAttributes[2].offset = offsetof(VertexData2, texCoords);

        SDL_GPUVertexBufferDescription vertexBufferDescription = {};
        vertexBufferDescription.slot = 0;
        vertexBufferDescription.pitch = sizeof(VertexData2); // stride
        vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

        SDL_GPUVertexInputState vertexInputState = {};
        vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
        vertexInputState.num_vertex_buffers = 1;
        vertexInputState.vertex_attributes = vertexAttributes;
        vertexInputState.num_vertex_attributes = 3;


        // Create pipeline with proper blending for text
        SDL_GPUColorTargetDescription coldescs = {};
        coldescs.format = SDL_GetGPUSwapchainTextureFormat(rendercontext.device, rendercontext.window);

        // Enable alpha blending for text
        coldescs.blend_state.enable_blend = true;
        coldescs.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        coldescs.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        coldescs.blend_state.color_write_mask = 0xF;
        coldescs.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        coldescs.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        coldescs.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        coldescs.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

        SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
        pipeInfo.vertex_shader = vertexShader;
        pipeInfo.fragment_shader = fragmentShader;
        pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipeInfo.target_info.color_target_descriptions = &coldescs;
        pipeInfo.target_info.num_color_targets = 1;
        pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_INVALID;
        pipeInfo.target_info.has_depth_stencil_target = false;

        pipeInfo.vertex_input_state = vertexInputState;
        pipeInfo.props = 0;

       
       // pipeline = check_error_ptr(SDL_CreateGPUGraphicsPipeline(device, &pipeInfo));
        pipeline = SDL_CreateGPUGraphicsPipeline(rendercontext.device, &pipeInfo);
        if (!pipeline) {
            SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
            return ;
        }

        SDL_ReleaseGPUShader(rendercontext.device, vertexShader);
        SDL_ReleaseGPUShader(rendercontext.device, fragmentShader);

        // Create sampler once
        SDL_GPUSamplerCreateInfo sampler_info = {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
        };
        sampler = check_error_ptr(SDL_CreateGPUSampler(rendercontext.device, &sampler_info));

        SDL_Log("text pipeline created!");

        if (TTF_Init() < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
        }

        font = TTF_OpenFont(fontPath.c_str(), 24);
        if (!font) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,"Couldn't load font: %s", SDL_GetError());
        }

        TTF_SetFontWrapAlignment(font, TTF_HORIZONTAL_ALIGN_LEFT);
        engine = check_error_ptr(TTF_CreateGPUTextEngine(rendercontext.device));

        // Create initial text
        updateText("Hello SDL GPU Text!",staticText, rendercontext.device);

       
    }

    void updateText(const char* newText, TextData & textData, SDL_GPUDevice* device) {
        
       // const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

      //  SDL_Log("TextRenderer::updateText() - Text: '%s'", newText);

        // Clean up old text
        if (textData.text) {
            TTF_DestroyText(textData.text);
        }
        if (textData.vertexBuffer) {
            SDL_ReleaseGPUBuffer(device, textData.vertexBuffer);
            textData.vertexBuffer = NULL;
        }
        if (textData.indexBuffer) {
            SDL_ReleaseGPUBuffer(device, textData.indexBuffer);
            textData.indexBuffer = NULL;
        }

        if (!font || !engine) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Font or engine not initialized");
            return;
        }

        // Create new text
        textData.text = TTF_CreateText(engine, font, newText, 0);
        if (!textData.text) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_CreateText failed: %s", SDL_GetError());
            return;
        }

        //// Get text size for debugging
        //int textWidth, textHeight;
        //if (TTF_GetTextSize(text, &textWidth, &textHeight)) {
        //   // SDL_Log("Text size: %dx%d pixels", textWidth, textHeight);
        //}
        //else {
        //    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_GetTextSize failed: %s", SDL_GetError());
        //}

        textData.seq = TTF_GetGPUTextDrawData(textData.text);
        if (!textData.seq) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_GetGPUTextDrawData returned NULL: %s", SDL_GetError());
            return;
        }

       // SDL_Log("TTF_GetGPUTextDrawData succeeded - vertices: %d, indices: %d", seq->num_vertices, seq->num_indices);

        if (textData.seq->num_vertices == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No vertices generated for text");
            return;
        }

        // Check atlas texture
        if (!textData.seq->atlas_texture) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Atlas texture is NULL");
            return;
        }

        // Build vertices & indices
        std::vector<VertexData2> vertices(textData.seq->num_vertices);
        std::vector<uint32_t> indices(textData.seq->num_indices);

        // Copy vertex data to match shader VertexInput exactly
        bool hasInvalidUV = false;
        for (int i = 0; i < textData.seq->num_vertices; i++) {
            SDL_FPoint pos = textData.seq->xy[i];
            SDL_FPoint uv = textData.seq->uv[i];

            vertices[i].position = { pos.x, pos.y, 0.0f };  // POSITION
            vertices[i].texCoords = { uv.x, uv.y };         // TEXCOORD0
            vertices[i].color = { 1.0f, 1.0f, 1.0f, 1.0f }; // COLOR0

            // Check for invalid UV coordinates
            if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                hasInvalidUV = true;
            }

        }

        if (hasInvalidUV) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "WARNING: Some UV coordinates are outside [0,1] range!");
        }

        std::memcpy(indices.data(), textData.seq->indices, textData.seq->num_indices * sizeof(uint32_t));


        // Upload data to buffers
        RenderUtil::uploadBufferData(device, textData.vertexBuffer, vertices.data(), vertices.size() * sizeof(VertexData2), SDL_GPU_BUFFERUSAGE_VERTEX);
        RenderUtil::uploadBufferData(device, textData.indexBuffer, indices.data(), indices.size() * sizeof(uint32_t), SDL_GPU_BUFFERUSAGE_INDEX);


    }


    void drawAll(int screenWidth, int screenHeight) {

        FrameContext& frameContext = ecs.get_mut<FrameContext>();
        const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

        //todo calculate text bounding box 
        // test if text fall within screen coordiantes

        float nearPlane = 0.0f;
        float farPlane = 1.0f;

        //VULKAN ORTHOGRAPHIC MATRIX
        ortho = glm::orthoRH_ZO(
            0.0f, (float)screenWidth,
            0.0f, (float)screenHeight,  // Y-down for text
            nearPlane, farPlane);


        SDL_GPUColorTargetInfo overlayTarget = {};
        overlayTarget.texture = frameContext.swapchainTexture;
        overlayTarget.load_op = SDL_GPU_LOADOP_LOAD; // LOAD so we don't overwrite the prev render pass
        overlayTarget.store_op = SDL_GPU_STOREOP_STORE;

        renderPass = SDL_BeginGPURenderPass(frameContext.commandBuffer, &overlayTarget, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

        //TODO dont hardcode text positions
        //TODO loop through all text
        DrawText(staticText,50, 50);
        DrawText(positionText, 10, 1000);

        if (fps) {
            DrawText(fpsText, 10, 1070);
        }

        SDL_EndGPURenderPass(renderPass);

    }

    void drawOverlay() {

       // // Start the Dear ImGui frame
       // ImGui_ImplSDLGPU3_NewFrame();
       // ImGui_ImplSDL3_NewFrame();
       // ImGui::NewFrame();

       // 

       // // Our state
       // bool show_demo_window = true;
       // bool show_another_window = false;
       // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

       //// ImGui::ShowDemoWindow(&show_demo_window);

       // // Set window properties for HUD elements
       // ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
       // ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
       // ImGui::SetNextWindowBgAlpha(0.7f);

       // ImGui::Begin("HUD", nullptr,
       //     ImGuiWindowFlags_NoMove |           // Prevent moving
       //     ImGuiWindowFlags_NoResize |         // Prevent resizing
       //     ImGuiWindowFlags_NoCollapse |       // Prevent collapsing
       //     ImGuiWindowFlags_AlwaysAutoResize | // Auto-fit content
       //     ImGuiWindowFlags_NoTitleBar         // Remove title bar (optional)
       // );

       // // Your HUD content
       // ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
       //// ImGui::Text("Position: %.2f, %.2f", player_x, player_y);

       // ImGui::End();

       // ImGui::Render();
       // ImDrawData* draw_data = ImGui::GetDrawData();

       // // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
       // ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, context->commandBuffer);

      

       // SDL_GPUColorTargetInfo overlayTarget = {};
       // overlayTarget.texture = context->swapchainTexture;
       // overlayTarget.load_op = SDL_GPU_LOADOP_LOAD; // LOAD so we don't overwrite the prev render pass
       // overlayTarget.store_op = SDL_GPU_STOREOP_STORE;

       // renderPass = SDL_BeginGPURenderPass(context->commandBuffer, &overlayTarget, 1, nullptr);

       // // Render ImGui
       // ImGui_ImplSDLGPU3_RenderDrawData(draw_data, context->commandBuffer, renderPass);

       // SDL_EndGPURenderPass(renderPass);

    }



    void DrawText(TextData textData,float offsetX, float offsetY) {

        FrameContext& frameContext = ecs.get_mut<FrameContext>();

        if (!textData.seq || textData.seq->num_vertices == 0 || !textData.vertexBuffer || !textData.indexBuffer) {
            SDL_Log("Cannot draw text: missing data");
            return;
        }

        SDL_GPUBufferBinding vertexBufferBinding = { textData.vertexBuffer, 0 };
        SDL_GPUBufferBinding indexBufferBinding = { textData.indexBuffer, 0 };

        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_GPUTextureSamplerBinding samplerBinding = { textData.seq->atlas_texture, sampler };
        SDL_BindGPUFragmentSamplers(renderPass, 0, &samplerBinding, 1);

        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(offsetX, offsetY, 0.0f));
        glm::mat4 screenSpaceMVP = ortho * translationMatrix;
        screenSpaceMVP = glm::transpose(screenSpaceMVP);

        SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 0, &screenSpaceMVP, sizeof(screenSpaceMVP));

        SDL_DrawGPUIndexedPrimitives(renderPass, textData.seq->num_indices, 1, 0, 0, 0);

    }

    //TODO clean up all 
    void cleanup(SDL_GPUDevice* device) {
       // if (text) TTF_DestroyText(text);
        if (engine) TTF_DestroyGPUTextEngine(engine);
        if (font) TTF_CloseFont(font);
        if (sampler) SDL_ReleaseGPUSampler(device, sampler);
        ///if (vertexBuffer) SDL_ReleaseGPUBuffer(device, vertexBuffer);
        //if (indexBuffer) SDL_ReleaseGPUBuffer(device, indexBuffer);
        if (pipeline) SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        TTF_Quit();
    }

};