#pragma once

#include "core/src/pch.h"

namespace EditorNS {

    void SceneTreeDraw(flecs::world& ecs) {
        // Find active game
        //TODO make sure there is only 1
        flecs::entity activeGameEntity;
        bool foundActive = false;

        auto q = ecs.query<Game, const IsActive>();
        q.each([&](flecs::entity e, Game, const IsActive) {
            activeGameEntity = e;
            foundActive = true;
        });

        if (!foundActive) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::Begin("Scene Tree", nullptr,
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoTitleBar
        );

        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Text("Scene Tree");

        static flecs::entity selectedEntity;

        // Recursive lambda to draw tree nodes
        std::function<void(flecs::entity)> drawEntityTree = [&](flecs::entity entity) {
            std::string name = entity.name().c_str();

            // Add icons based on component type
            const char* icon = "  ";
            if (entity.has<Game>()) icon = "▶ ";      // U+25B6
            else if (entity.has<_Scene>()) icon = "◆ "; // U+25C6
            else if (entity.has<StaticEnt>()) icon = "■ "; // U+25A0
            else if (entity.has<DynamicEnt>()) icon = "● "; // U+25CF

            std::string displayName = icon + name;

            //check if entity has any children
            bool hasChildren = false;
            ecs_iter_t it = ecs_each_pair(ecs, EcsChildOf, entity);
            if (ecs_iter_is_true(&it)) {
                hasChildren = true;
            }


            if (hasChildren) {
                // TreeNode for parents
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |  
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanAvailWidth;

               
                if (selectedEntity == entity) {
                    flags |= ImGuiTreeNodeFlags_Selected;  // This provides the highlight
                }

                bool nodeOpen = ImGui::TreeNodeEx(displayName.c_str(), flags);

                // Handle single click selection
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    selectedEntity = entity;
                }

                // Handle double click - custom behavior per type
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (entity.has<_Scene>()) {
                        // Load scene
                        std::cout << "Loading scene: " << name << std::endl;
                        // Your scene loading logic here
                        //LoadScene(entity);
                    }
                    else if (entity.has<Game>()) {
                        // Maybe expand/collapse or show game settings
                        std::cout << "Game settings for: " << name << std::endl;
                    }
                }

                if (nodeOpen) {
                    entity.children([&](flecs::entity child) {
                        drawEntityTree(child);
                    });
                    ImGui::TreePop();
                }
            }
            else {
                // Selectable for leaf nodes (objects)
                ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
                bool isSelected = (selectedEntity == entity);

                if (ImGui::Selectable(displayName.c_str(), isSelected, flags)) {
                    selectedEntity = entity;
                }

                // Handle double click for objects
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (entity.has<StaticEnt>() || entity.has<DynamicEnt>()) {
                        // Open properties panel
                        std::cout << "Opening properties for: " << name << std::endl;
                        //OpenPropertiesPanel(entity);
                    }
                }

                // Optional: Right-click context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Delete")) {
                        // Delete entity
                        std::cout << "Deleting: " << name << std::endl;
                    }
                    if (ImGui::MenuItem("Duplicate")) {
                        // Duplicate entity
                        std::cout << "Duplicating: " << name << std::endl;
                    }
                    ImGui::EndPopup();
                }
            }
        };

        // Draw the tree starting from active game
        drawEntityTree(activeGameEntity);

        // Show selected entity info
        if (selectedEntity) {
            ImGui::Separator();
            ImGui::Text("Selected: %s", selectedEntity.name().c_str());
        }

        ImGui::End();
    }

    // Helper functions you'd implement
    void LoadScene(flecs::entity sceneEntity) {
        // Your scene loading logic
    }

    void OpenPropertiesPanel(flecs::entity objectEntity) {
        // Show properties panel, maybe set a global variable or state
        // that another ImGui window reads to display properties
    }


}