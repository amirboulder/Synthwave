#pragma once

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include "../RendererConfig.hpp"

class UserInterface {

public:

    flecs::world& ecs;

    flecs::query<HudRender>q1;
    flecs::query<Render,MenuItem,Active>q2;

    UserInterface(flecs::world& ecs)
        :   ecs(ecs)
    {

    }

    // TODO create UI config
    void init() {

        const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();
        const RendererConfig& config = ecs.get<RendererConfig>();

        // Create SDL window graphics context
        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        ImGui::StyleColorsDark();


        // Setup scaling
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = main_scale ;       // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary
        style.WindowRounding = 0.0f;            // Remove rounded corners for more "game-like" feel
        style.FrameRounding = 0.0f;             // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

        //Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLGPU(rendercontext.window);
        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device = rendercontext.device;
        init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(rendercontext.device, rendercontext.window);
        //init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
       // init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
        // Only used in multi-viewports mode. this gets overwritten by RendererConfig when rendering in the same view port
       // init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;  


        ImGui_ImplSDLGPU3_Init(&init_info);

        ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Supermolot Light.otf");
        IM_ASSERT(font != nullptr);

        buildHUDQueries();
    }

    void buildHUDQueries() {

        q1 = ecs.query_builder<HudRender>()
            .cached()
            .build();

        q2 = ecs.query_builder<Render, MenuItem, Active>()
            .cached()
            .build();
    }


    void drawUI() {

        FrameContext& frameContext = ecs.get_mut<FrameContext>();

        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // We're basically injecting game HUD code in here by doing this
      
        q2.each([&](flecs::entity e, Render renderCallback, MenuItem , Active) {

            renderCallback.draw(ecs);

        });
 

        q1.each([&](flecs::entity e, HudRender hud) {

            hud.draw(ecs);

        });

        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();

        // uploads to buffer
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, frameContext.commandBuffer);

        SDL_GPUColorTargetInfo overlayTarget = {};
        overlayTarget.texture = frameContext.swapchainTexture;
        overlayTarget.load_op = SDL_GPU_LOADOP_LOAD; // LOAD so we don't overwrite the prev render pass
        overlayTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(frameContext.commandBuffer, &overlayTarget, 1, nullptr);

        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, frameContext.commandBuffer, renderPass);

        SDL_EndGPURenderPass(renderPass);

    }
};