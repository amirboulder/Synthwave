#pragma once

#include "core/src/pch.h"

#include "../RendererConfig.hpp"

class UserInterface {

public:

    flecs::world& ecs;

    flecs::system HudRenderSys;
    flecs::system MenuRenderSys;
    flecs::system EditorUIRenderSys;
    flecs::system OverlayRenderSys;

    UserInterface(flecs::world& ecs)
        :   ecs(ecs)
    {

    }

    // TODO create UI config
    void init() {

        const RenderContext& renderContext = ecs.get<RenderContext>();
        const RenderConfig& config = ecs.get<RenderConfig>();

        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
       
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
       // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
        ImGui_ImplSDL3_InitForSDLGPU(renderContext.window);
        ImGui_ImplSDLGPU3_InitInfo init_info = {};
        init_info.Device = renderContext.device;
        init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(renderContext.device, renderContext.window);
        //init_info.MSAASamples = config.sampleCountMSAA;                      // Only used in multi-viewports mode.
        //init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
        // Only used in multi-viewports mode. this gets overwritten by RendererConfig when rendering in the same view port
        //init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;  


        ImGui_ImplSDLGPU3_Init(&init_info);


        //The main font Orbitron just has the alphabet so we merge two font with it to get emojis and symbols.
        static ImFontConfig cfg;
        float fontSize = 24.0f;

        ImFont* mainFont = io.Fonts->AddFontFromFileTTF(
            "assets/fonts/Orbitron-VariableFont_wght.ttf",
            fontSize, &cfg
        );
        IM_ASSERT(mainFont != nullptr);

        cfg.MergeMode = true;

        cfg.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
        ImFont* emojiFont =  io.Fonts->AddFontFromFileTTF(
            "assets/fonts/NotoColorEmoji-Regular.ttf",
            fontSize,
            &cfg
        );
        IM_ASSERT(emojiFont != nullptr);


        ImFont* symbolsFont =  io.Fonts->AddFontFromFileTTF(
            "assets/fonts/NotoSansSymbols2-Regular.ttf",
            48.0f,
            &cfg
        );
        IM_ASSERT(emojiFont != nullptr);

        // Build font atlas
        io.Fonts->Build();

        registerSystems();
    }

    // manually called 
    void registerSystems() {

        // We're injecting game code in here by doing this
        HudRenderSys = ecs.system<HudRender>("HudRenderSys")
            .kind(0)
            .each([&](flecs::entity e, HudRender hud) {

            hud.draw(ecs);

        });

        MenuRenderSys = ecs.system<Render, MenuComponent>("MenuRenderSys")
            .kind(0)
            //.immediate()
            .each([&](flecs::entity e, Render renderCallback, MenuComponent) {

            renderCallback.draw(ecs);

        });


        EditorUIRenderSys = ecs.system<Render, EditorUIComponent>("EditorUIRenderSys")
            .kind(0)
            .each([&](flecs::entity e, Render renderCallback, EditorUIComponent) {

            renderCallback.draw(ecs);

        });

        OverlayRenderSys = ecs.system<Draw,OverlayComponent>("OverlayRenderSys")
            .kind(0)
            .each([&](flecs::entity e, Draw drawFunction, OverlayComponent) {

            drawFunction.draw();

        });

    }



    void drawUI() {

        FrameContext& frameContext = ecs.get_mut<FrameContext>();

        // Start the Dear ImGui frame
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Create a dock space over the main viewport
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        HudRenderSys.run();
        MenuRenderSys.run();
        EditorUIRenderSys.run();
        OverlayRenderSys.run();


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

        //Needeed for multiview ports
        /* ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }*/

    }
};