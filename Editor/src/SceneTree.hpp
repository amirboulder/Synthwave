#pragma once

#include "core/src/pch.h"

class SceneTree {
private:

    // State management
    //TODO make part of the editor or ecs
    struct State {
        flecs::entity selectedEntity;
        flecs::entity contextEntity;  // Entity that opened the context menu
        char childNameBuffer[128] = "";
        bool showAddChildPopup = false;
        EntityType selectedType = EntityType::Empty;
    };

    static State s_state;

    static const char* GetEntityTypeName(EntityType type) {
        switch (type) {
        case EntityType::Empty: return "Empty";
        case EntityType::Actor: return "Actor";
        case EntityType::Capsule: return "Capsule";
        case EntityType::Sphere: return "Sphere";
        case EntityType::Cube: return "Cube";
        case EntityType::Light: return "Light";
        case EntityType::Camera: return "Camera";
        default: return "Unknown";
        }
    }

    // Helper functions
    //TODO fix these by adding the imgui icons and or / Glyphs/Emojis
    static const char* GetEntityIcon(flecs::entity entity) {
        if (entity.has<Game>()) return "▶ ";
        if (entity.has<_Scene>()) return "◆ ";
        if (entity.has<StaticEnt>()) return "■ ";
        if (entity.has<DynamicEnt>()) return "● ";
        return "  ";
    }

    static bool HasChildren(flecs::world& ecs, flecs::entity entity) {
        ecs_iter_t it = ecs_each_pair(ecs, EcsChildOf, entity);
        return ecs_iter_is_true(&it);
    }

    static void DrawEntityNode(flecs::world& ecs, flecs::entity entity);
    static void DrawContextMenu(flecs::world& ecs, flecs::entity entity);
    static void drawAddChildPopup(flecs::world& ecs);
    static void HandleEntitySelection(flecs::entity entity);
    static void HandleEntityDoubleClick(flecs::entity entity);

    //The the entity creation functions
    static void createCapsuleChild(flecs::world& ecs);
    static void createActorChild(flecs::world& ecs);

public:
    static void SceneTreeDraw(flecs::world& ecs);
    static void LoadScene(flecs::entity sceneEntity);
    static void OpenPropertiesPanel(flecs::entity objectEntity);
};

// Initialize static state
SceneTree::State SceneTree::s_state;

void SceneTree::SceneTreeDraw(flecs::world& ecs) {
    ImGui::Begin("Scene Tree", nullptr, ImGuiWindowFlags_NoCollapse);

    // Find active game entity
    flecs::entity activeGameEntity;
    bool foundActive = false;

    auto q = ecs.query<Game, const IsActive>();
    q.each([&](flecs::entity e, Game, const IsActive) {
        activeGameEntity = e;
        foundActive = true;
    });

    if (!foundActive) {
        ImGui::Text("No active game found");
        ImGui::End();
        return;
    }

    // Draw the entity tree
    DrawEntityNode(ecs, activeGameEntity);

    // Show selected entity info
    if (s_state.selectedEntity) {
        ImGui::Separator();
        ImGui::Text("Selected: %s", s_state.selectedEntity.name().c_str());
    }

    // must be called every frame
    drawAddChildPopup(ecs);

    ImGui::End();
}

void SceneTree::DrawEntityNode(flecs::world& ecs, flecs::entity entity) {
    std::string name = entity.name().c_str();
    std::string displayName = std::string(GetEntityIcon(entity)) + name + " " + std::to_string(entity.id());
    bool hasChildren = HasChildren(ecs, entity);

    if (hasChildren) {
        // Parent node with children
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (s_state.selectedEntity == entity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool nodeOpen = ImGui::TreeNodeEx(displayName.c_str(), flags);

        HandleEntitySelection(entity);
        HandleEntityDoubleClick(entity);
        DrawContextMenu(ecs, entity);

        if (nodeOpen) {
            entity.children([&](flecs::entity child) {
                DrawEntityNode(ecs, child);
            });
            ImGui::TreePop();
        }
    }
    else {
        // Leaf node (no children)
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;
        bool isSelected = (s_state.selectedEntity == entity);

        if (ImGui::Selectable(displayName.c_str(), isSelected, flags)) {
            s_state.selectedEntity = entity;
        }

        HandleEntityDoubleClick(entity);
        DrawContextMenu(ecs, entity);
    }
}

void SceneTree::HandleEntitySelection(flecs::entity entity) {
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        s_state.selectedEntity = entity;
    }
}

void SceneTree::HandleEntityDoubleClick(flecs::entity entity) {
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        if (entity.has<_Scene>()) {
            std::cout << "Loading scene: " << entity.name().c_str() << std::endl;
            LoadScene(entity);
        }
        else if (entity.has<Game>()) {
            std::cout << "Game settings for: " << entity.name().c_str() << std::endl;
        }
        else if (entity.has<StaticEnt>() || entity.has<DynamicEnt>()) {
            std::cout << "Opening properties for: " << entity.name().c_str() << std::endl;
            OpenPropertiesPanel(entity);
        }
    }
}

void SceneTree::DrawContextMenu(flecs::world& ecs, flecs::entity entity) {
    if (ImGui::BeginPopupContextItem()) {
        if (entity.has<_Scene>()) {
            // Scene-specific menu
            if (ImGui::MenuItem("Add Child")) {
                s_state.contextEntity = entity;
                s_state.showAddChildPopup = true;
                ImGui::CloseCurrentPopup();
            }
           
            ImGui::Separator();
        }

        // Common menu items
        if (ImGui::MenuItem("Delete")) {
            std::cout << "Deleting: " << entity.name().c_str() << std::endl;
            // TODO: Implement deletion
        }

        if (ImGui::MenuItem("Duplicate")) {
            std::cout << "Duplicating: " << entity.name().c_str() << std::endl;
            // TODO: Implement duplication
        }

        ImGui::EndPopup();
    }
}

void SceneTree::drawAddChildPopup(flecs::world& ecs) {
    // Open popup if flagged
    if (s_state.showAddChildPopup) {
        ImGui::OpenPopup("Add Child Entity");
        s_state.showAddChildPopup = false;
        s_state.selectedType = EntityType::Empty;  // Reset to default
    }

    // Render popup modal
    if (ImGui::BeginPopupModal("Add Child Entity", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Entity Type:");
        ImGui::SetNextItemWidth(300.0f);

        // Dropdown for entity type
        if (ImGui::BeginCombo("##entitytype", GetEntityTypeName(s_state.selectedType))) {
            for (int i = 0; i < static_cast<int>(EntityType::COUNT); i++) {
                EntityType type = static_cast<EntityType>(i);
                bool isSelected = (s_state.selectedType == type);

                if (ImGui::Selectable(GetEntityTypeName(type), isSelected)) {
                    s_state.selectedType = type;
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Text("Enter entity name:");

        ImGui::SetNextItemWidth(300.0f);
        bool enterPressed = ImGui::InputText("##childname", s_state.childNameBuffer,
            IM_ARRAYSIZE(s_state.childNameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Create button (or Enter key)
        if (ImGui::Button("Create", ImVec2(140, 0)) || enterPressed) {
            if (strlen(s_state.childNameBuffer) > 0) {
                std::cout << "Creating " << GetEntityTypeName(s_state.selectedType)
                    << " entity '" << s_state.childNameBuffer
                    << "' under " << s_state.contextEntity.name().c_str() << std::endl;

                
                 switch (s_state.selectedType) {
                     case EntityType::Empty:

                         SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Empty not yet implemented");
                         break;
                     case EntityType::Actor:

                         createActorChild(ecs);
                         break;
                     case EntityType::Capsule:

                         createCapsuleChild(ecs);
                         break;
                     case EntityType::Sphere:
                         SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Sphere not yet implemented");
                         break;
                     case EntityType::Cube:
                         SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Cube not yet implemented");
                         break;
                     case EntityType::Light:
                         SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, " Adding Camera not yet implemented");
                         break;
                     case EntityType::Camera:
                         SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, " Adding Camera not yet implemented");
                         break;
                 }

                s_state.childNameBuffer[0] = '\0';
                s_state.contextEntity = flecs::entity();
                s_state.selectedType = EntityType::Empty;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        // Cancel button (or ESC key)
        if (ImGui::Button("Cancel", ImVec2(140, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            s_state.childNameBuffer[0] = '\0';
            s_state.contextEntity = flecs::entity();
            s_state.selectedType = EntityType::Empty;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}


void SceneTree::createCapsuleChild(flecs::world& ecs) {

    Transform capsule1Transform;
    capsule1Transform.position = glm::vec3(1.0f, 5.0f, 0.0f);
    capsule1Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    capsule1Transform.scale = glm::vec3(1.0f);
    EntityFactory::createCapsuleEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "CapsuleModel", capsule1Transform);

}

void SceneTree::createActorChild(flecs::world& ecs) {

    Transform actorTransform;
    actorTransform.position = glm::vec3(4.0f, 17.0f, 0.0f);
    actorTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    actorTransform.scale = glm::vec3(1.0f);

    // Character settings
    JPH::CharacterSettings settings;
    settings.mShape = new CapsuleShape(2.0f, 1.0f);
    settings.mMass = 2000.0f;
    settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
    settings.mLayer = Layers::MOVING;
    settings.mGravityFactor = 1;
    EntityFactory::createActorEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "ActorModel", actorTransform, settings, actor1Update);

}

void SceneTree::LoadScene(flecs::entity sceneEntity) {
    // TODO Implement scene loading
}

void SceneTree::OpenPropertiesPanel(flecs::entity objectEntity) {
    // TODO Implement properties panel
}