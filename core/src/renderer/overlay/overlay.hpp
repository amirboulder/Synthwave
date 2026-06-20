#pragma once


class Overlay {

    flecs::world& ecs;

    flecs::query<ActorDebugInfo> actorDebugInfo;

public:


	Overlay(flecs::world& ecs)
        :ecs(ecs)
    {

	}

    void init() {

        registerComponents();
        registerQueries();
    }


    void registerComponents() {

    
       std::function<void()> drawFunction =
            [this]() {
            this->drawFPS();
        };

        flecs::entity fpsEntity = ecs.entity("fps")
            .emplace<Draw>(drawFunction)
            .add<OverlayComponent>();


        std::function<void()> drawFunction2 =
            [this]() {
            this->debugInfo();
        };

        flecs::entity ActorDebugInfoEntity = ecs.entity("ActorDebugInfo")
            .emplace<Draw>(drawFunction2)
            .add<OverlayComponent>();

    }

   //For now we can place it wherever
   //TODO RELEASE give player option to place it top left or top right
    void drawFPS() {

       // ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::Begin("HUD", nullptr,          
            ImGuiWindowFlags_NoResize |         // Prevent resizing
            ImGuiWindowFlags_NoCollapse |       // Prevent collapsing
            ImGuiWindowFlags_AlwaysAutoResize | // Auto-fit content
            ImGuiWindowFlags_NoTitleBar        
            | ImGuiWindowFlags_NoDocking

            
        );

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


        ImGui::End();

    }

    void registerQueries() {

        actorDebugInfo = ecs.query_builder<ActorDebugInfo>()
            .build();
    }


    void debugInfo() {

        

        // ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::Begin("HUD", nullptr,
            ImGuiWindowFlags_NoResize |         // Prevent resizing
            ImGuiWindowFlags_NoCollapse |       // Prevent collapsing
            ImGuiWindowFlags_AlwaysAutoResize | // Auto-fit content
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoDocking


        );

        actorDebugInfo.each([&](flecs::entity entity, ActorDebugInfo debugInfo) {

            uint32_t entID = (uint32_t)entity.id();

            std::string name = entity.name().c_str();

            bool visible = debugInfo.playerVisible;

            ImGui::Text("Actor %s player visibility", name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(visible ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), visible ? "VISIBLE" : "HIDDEN");

        });

        ImGui::End();

    }
};