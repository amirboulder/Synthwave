#pragma once

#include "imgui.h"

//fps window
void FPSDraw(flecs::world& ecs) {

    // ImGui::ShowDemoWindow(&show_demo_window);
//   const JPH::Vec3 PlayerPos =  ecs.lookup("player").get_mut<Player>().position;

 
    // Set window properties for HUD elements
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::Begin("HUD", nullptr,
        ImGuiWindowFlags_NoMove |           // Prevent moving
        ImGuiWindowFlags_NoResize |         // Prevent resizing
        ImGuiWindowFlags_NoCollapse |       // Prevent collapsing
        ImGuiWindowFlags_AlwaysAutoResize | // Auto-fit content
        ImGuiWindowFlags_NoTitleBar         // Remove title bar (optional)
    );

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
   // ImGui::Text("Player Pos", PlayerPos.GetX(), "  ", PlayerPos.GetY(), "  ", PlayerPos.GetZ());
  //  ImGui::Text("Player Pos %.1f   %.1f  %.1f", PlayerPos.GetX(), PlayerPos.GetY(), PlayerPos.GetZ());

    ImGui::End();

}