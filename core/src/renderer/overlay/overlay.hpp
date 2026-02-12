#pragma once


class Overlay {

    flecs::world& ecs;


public:


	Overlay(flecs::world& ecs)
        :ecs(ecs)
    {

	}

    void init() {

        registerComponents();
    }


    void registerComponents() {

    
       std::function<void()> drawFunction =
            [this]() {
            this->drawFPS();
        };
        flecs::entity entity = ecs.entity("fps")
            .emplace<Draw>(drawFunction)
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
};