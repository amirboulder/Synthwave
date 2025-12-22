#pragma once

#include "core/src/pch.h"
#include "core/src/physics/ragdoll.hpp"

enum class ShapeType
{
    Capsule,
    Sphere,
    Box,
    COUNT,
};

struct EnumEntry
{
    ShapeType value;
    const char* name;
    const char* category;
};

constexpr EnumEntry ShapeTypeMeta[] = {
    { ShapeType::Capsule, "Capsule", "Primitive" },
    { ShapeType::Sphere,  "Sphere",  "Primitive" },
    { ShapeType::Box,     "Box",     "Primitive" },
};


class RagdollBuilder {

public:

    struct State {

        bool s_showingWindow = true;

        //Camera
        glm::vec3 camPos = glm::vec3(10.0f, 10.0f, 10.0f);
        bool camPosSet = false;

        ShapeType selectedShape = ShapeType::Capsule;
        bool creating = false;
        float windowWidth = 0;

        std::vector<JPH::BodyID> bodies;
        std::vector<BodyPart> parts;


        //Capsule
        float capsuleHeight = 0.0f;
        float capsuleRadius = 0.0f;
        glm::vec3 capsulePos = glm::vec3();
        glm::quat capsuleRot = glm::quat();

        //Sphere
        float sphereRadius = 0.0f;
        glm::vec3 spherePos = glm::vec3();
        glm::quat sphereRot = glm::quat();

    };
    static State s_state;


    static void setCamPos(flecs::world& ecs) {

        flecs::entity freeCamEnt = ecs.lookup("FreeCam");
        freeCamEnt.get_mut<Camera>().position = s_state.camPos;
        s_state.camPosSet = true;
    }

    static void draw(flecs::world& ecs)
    {
        ImGui::Begin("Ragdoll Builder", nullptr, ImGuiWindowFlags_NoCollapse);

        s_state.windowWidth = ImGui::GetWindowSize().x;

        beginCreation(ecs);

        if (ImGui::BeginTable("RagdollLayout", 2,
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingStretchProp))
        {
            // Left column (Tree)
            ImGui::TableNextColumn();
            ImGui::BeginChild("##TreePane", ImVec2(0, 0), true);
            drawTree(ecs);
            ImGui::EndChild();

            // Right column (Details / Creation UI)
            ImGui::TableNextColumn();
            ImGui::BeginChild("##DetailsPane", ImVec2(0, 0), true);

            if (s_state.creating)
            {
                drawAddPart(ecs);
            }

            ImGui::EndChild();

            ImGui::EndTable();
        }

        ImGui::End();
    }


    static void beginCreation(flecs::world& ecs) {

        if (s_state.creating) {
            return;
        }

        float buttonWidth = 140.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - totalWidth) * 0.5f);
        if (ImGui::Button("Begin", ImVec2(buttonWidth, 0))) {

            s_state.creating = true;

        }

    }


    static void drawAddRoot(flecs::world& ecs) {

        float windowWidth = ImGui::GetWindowSize().x;
        const char* baseTypTxt = " Root Shape Type Type:";
        float textWidth = ImGui::CalcTextSize(baseTypTxt).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

        ImGui::SetNextItemWidth(250.0f);
        if (ImGui::BeginCombo("Shape Type", ShapeTypeMeta[0].name))
        {
            for (int i = 0; i < static_cast<int>(ShapeType::COUNT); i++)
            {
                ShapeType type = static_cast<ShapeType>(i);
                bool is_selected = (s_state.selectedShape == type);
                if (ImGui::Selectable(ShapeTypeMeta[i].name, is_selected))
                    s_state.selectedShape = type;

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();

                }


            }
            ImGui::EndCombo();
        }
    }

    static void drawAddPart(flecs::world& ecs) {

        float windowWidth = ImGui::GetWindowSize().x;
        const char* entTypeTxt = "Shape Type Type:";
        float textWidth = ImGui::CalcTextSize(entTypeTxt).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);


        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::SetNextItemWidth(250.0f);
        int typeIndex = static_cast<int>(s_state.selectedShape);
        if (ImGui::BeginCombo("Shape Type", ShapeTypeMeta[typeIndex].name)) {
            for (int i = 0; i < static_cast<int>(ShapeType::COUNT); i++){
                ShapeType type = static_cast<ShapeType>(i);
                bool is_selected = (s_state.selectedShape == type);

                if (ImGui::Selectable(ShapeTypeMeta[i].name, is_selected))
                    s_state.selectedShape = type;

                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        switch (s_state.selectedShape) {
        case ShapeType::Capsule:

            //SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Empty not yet implemented");
            drawCapsuleOptions(ecs);
            break;
        case ShapeType::Sphere:

            drawSphereOptions(ecs);
            break;
        case ShapeType::Box:

            drawBoxOptions(ecs);
            break;
        }


       
    }

    static void drawCapsuleOptions(flecs::world& ecs) {

       // ImGui::SetCursorPosX((s_state.windowWidth - 300.0f) * 0.5f);
        //ImGui::SetNextItemWidth(300.0f);

        ImGui::InputFloat("CapsuleHeight", &s_state.capsuleHeight, 0.1f, 1.0f, "%.3f");
        ImGui::InputFloat("CapsuleRadius", &s_state.capsuleRadius, 0.1f, 1.0f, "%.3f");

        ImGui::InputFloat3("local Position", &(s_state.capsulePos.x));
        ImGui::InputFloat4("local rotation", &(s_state.capsuleRot.x));


        float buttonWidth = 140.0f;
        if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

            //Add shape to Ragdoll here
            int  selectedShapeInt = static_cast<int>(s_state.selectedShape);
            cout << "Adding : " << ShapeTypeMeta[selectedShapeInt].name << std::endl;

            JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

            JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

            //Add some checks for size 
            JPH::Vec3 joltPosition(s_state.capsulePos.x, s_state.capsulePos.y, s_state.capsulePos.z);
            JPH::Quat joltRotation(s_state.capsuleRot.x, s_state.capsuleRot.y, s_state.capsuleRot.z, s_state.capsuleRot.w);
            if (!joltRotation.IsNormalized()) {
                joltRotation = joltRotation.Normalized();
            }

            Ref<Shape> capsuleShape = new JPH::CapsuleShape(s_state.capsuleHeight /2.0f , s_state.capsuleRadius);

            JPH::BodyCreationSettings pillSettings(
                capsuleShape,
                joltPosition,
                joltRotation,
                JPH::EMotionType::Dynamic,
                Layers::MOVING
            );

            // Create and add body
            s_state.bodies.push_back(bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::DontActivate));

            BodyPart part;
            part.name = "Capsule";
            part.skeletonJointIndex = 0;
            part.position = joltPosition;
            part.rotation = joltRotation;
            part.parent = nullptr;

            s_state.parts.push_back(part);
        }
    }

    static void drawSphereOptions(flecs::world& ecs) {

        ImGui::InputFloat("Radius", &s_state.sphereRadius, 0.0f, 0.0f, "%.3f");

        ImGui::InputFloat3("local Position", &(s_state.spherePos.x));
        ImGui::InputFloat4("local rotation", &(s_state.sphereRot.x));


        float buttonWidth = 140.0f;
        if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {


            int  selectedShapeInt = static_cast<int>(s_state.selectedShape);
            cout << "Adding : " << ShapeTypeMeta[selectedShapeInt].name << std::endl;

            JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

            JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

            //Add some checks for size 
            JPH::Vec3 joltPosition(s_state.spherePos.x, s_state.spherePos.y, s_state.spherePos.z);
            JPH::Quat joltRotation(s_state.sphereRot.x, s_state.sphereRot.y, s_state.sphereRot.z, s_state.sphereRot.w);
            if (!joltRotation.IsNormalized()) {
                joltRotation = joltRotation.Normalized();
            }

            Ref<Shape> shape = new JPH::SphereShape(s_state.sphereRadius);

            JPH::BodyCreationSettings settings(
                shape,
                joltPosition,
                joltRotation,
                JPH::EMotionType::Dynamic,
                Layers::MOVING
            );

            // Create and add body
            s_state.bodies.push_back(bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate));
        }
    }

    static void drawBoxOptions(flecs::world& ecs) {

        ImGui::InputFloat("Radius", &s_state.sphereRadius, 0.0f, 0.0f, "%.3f");

        ImGui::InputFloat3("local Position", &(s_state.spherePos.x));
        ImGui::InputFloat4("local rotation", &(s_state.sphereRot.x));


        float buttonWidth = 140.0f;
        if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {


            int  selectedShapeInt = static_cast<int>(s_state.selectedShape);
            cout << "Adding : " << ShapeTypeMeta[selectedShapeInt].name << std::endl;

            JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

            JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

            //Add some checks for size 
            JPH::Vec3 joltPosition(s_state.spherePos.x, s_state.spherePos.y, s_state.spherePos.z);
            JPH::Quat joltRotation(s_state.sphereRot.x, s_state.sphereRot.y, s_state.sphereRot.z, s_state.sphereRot.w);
            if (!joltRotation.IsNormalized()) {
                joltRotation = joltRotation.Normalized();
            }

            Ref<Shape> shape = new JPH::SphereShape(s_state.sphereRadius);

            JPH::BodyCreationSettings settings(
                shape,
                joltPosition,
                joltRotation,
                JPH::EMotionType::Dynamic,
                Layers::MOVING
            );

            // Create and add body
            s_state.bodies.push_back(bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate));
        }
    }

    static void  drawTree(flecs::world& ecs) {

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        for (int i = 0; i < s_state.bodies.size(); i++) {

            bool nodeOpen = ImGui::TreeNodeEx(std::to_string(s_state.bodies[i].GetIndex()).c_str(), flags);
            if (nodeOpen)
            {
                ImGui::TreePop();
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                cout << "IsMouseDoubleClicked\n;";
            }
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                cout << "IsItemClicked\n;";
            }

            if (ImGui::BeginPopupContextItem()) {


                if (ImGui::MenuItem("Copy")) {
                    std::cout << "Duplicating: " << std::endl;
                    // TODO: Implement duplication
                }
                // Common menu items
                if (ImGui::MenuItem("Delete")) {
                    std::cout << "Deleting: " << std::endl;
                    // TODO: Implement deletion
                }

                

                ImGui::EndPopup();
            }
        }

       
    }

    static void HandleEntitySelection(flecs::entity entity) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
           // s_state.selectedEntity = entity;
        }
    }

};

// Initialize static state
RagdollBuilder::State RagdollBuilder::s_state;