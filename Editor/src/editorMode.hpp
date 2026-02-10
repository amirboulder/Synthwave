#pragma once

#include "core/src/pch.h"

enum class EditorMode {
	Selection,
    Translate,
    Rotate,
    Scale,
    None,

};

class EditorModeSelector {

public:

    struct State {
        
        //Always init in None mode
        EditorMode mode = EditorMode::None;

    };
    static State s_state;


    static void draw(flecs::world& ecs)
    {
        ImGui::Begin("Editor Selection", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoDocking
        );

        float buttonSize = 60.0f;
        ImGui::SetWindowFontScale(1.5f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));


        // Select
        if (s_state.mode == EditorMode::Selection) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::Button("🖑", ImVec2(buttonSize, buttonSize));
            ImGui::PopStyleColor();
        }
        else {
            if (ImGui::Button("🖑", ImVec2(buttonSize, buttonSize))) {
                updateEditorMode(ecs, EditorMode::Selection);
            }
        }

        // Translate
        if (s_state.mode == EditorMode::Translate) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::Button("✥", ImVec2(buttonSize, buttonSize));
            ImGui::PopStyleColor();
        }
        else {
            if (ImGui::Button("✥", ImVec2(buttonSize, buttonSize))) {
                updateEditorMode(ecs, EditorMode::Translate);
            }
        }


        // Rotate
        if (s_state.mode == EditorMode::Rotate) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::Button("🗘", ImVec2(buttonSize, buttonSize));
            ImGui::PopStyleColor();
        }
        else {
            if (ImGui::Button("🗘", ImVec2(buttonSize, buttonSize))) {
                updateEditorMode(ecs, EditorMode::Rotate);
            }
        }
        
        // Scale
        if (s_state.mode == EditorMode::Scale) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::Button("◱", ImVec2(buttonSize, buttonSize));
            ImGui::PopStyleColor();
        }
        else {
            if (ImGui::Button("◱", ImVec2(buttonSize, buttonSize))) {
                updateEditorMode(ecs, EditorMode::Scale);
            }
        }
        
        ImGui::PopStyleVar(2);

        ImGui::End();
    }


    static void updateEditorMode(flecs::world& ecs, EditorMode newMode) {

        s_state.mode = newMode;

        ecs.set<EditorMode>({ newMode });

    }
};

// Initialize static state
EditorModeSelector::State EditorModeSelector::s_state;