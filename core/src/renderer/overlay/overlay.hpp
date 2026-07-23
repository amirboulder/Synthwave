#pragma once

#include "../UI/UITemplates.hpp"

class Overlay {

    flecs::world& ecs;

    flecs::query<ActorDebugInfo> actorDebugInfo;
    flecs::query<JoltRagdoll> ragdollquery;
    flecs::query<EnemyState> enemyStateQuery;

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


        std::function<void()> drawFunction3 =
            [this]() {
            this->stateController();
        };

        flecs::entity EnemyStateEntity = ecs.entity("EnemyState")
            .emplace<Draw>(drawFunction3)
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

        ragdollquery = ecs.query_builder<JoltRagdoll>()
            .build();

        enemyStateQuery = ecs.query_builder<EnemyState>()
            .build();
    }


    void debugInfo() {

        

        // ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowBgAlpha(1.0f);

        ImGui::Begin("ActorDebugInfo", nullptr,
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

            ImGui::Text("Actor : %s player visibility", name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(visible ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), visible ? "VISIBLE" : "HIDDEN");

        });


        ragdollquery.each([&](flecs::entity entity, JoltRagdoll joltRagdoll) {

            ImGui::NewLine();
            ImGui::SeparatorText("--------------");

            uint32_t entID = (uint32_t)entity.id();
            std::string name = entity.name().c_str();

            //joltRagdoll.ragdollPtr->getCo

           // iterateConstraints(joltRagdoll.ragdollPtr);

        });

        ImGui::End();
    }

    void stateController() {

        enemyStateQuery.each([&](flecs::entity entity, EnemyState & state) {

            ImGui::NewLine();
            ImGui::SeparatorText("--------------");

            uint32_t entID = (uint32_t)entity.id();
            std::string name = entity.name().c_str();

            name.append(" State : ");

            std::optional<EnemyState> newState = ImGui::EnumCombo(name.c_str(), &state);

            if (newState.has_value()) {

                entity.set<EnemyState>(newState.value());
            }

        });


    }

    enum class MotorType {

        Hinge,
        Twist,
        Swing,
        TranslationX,
        TranslationY,
        TranslationZ,

        RotationX,
        RotationY,
        RotationZ,

        Slider,
        Path,

        Count

    };

    struct MotorInfo {

        float mMinForceLimit = 0;
        float mMaxForceLimit = 0;
        float mMinTorqueLimit = 0;
        float mMaxTorqueLimit = 0;

        MotorType type = MotorType::Count;
    };

    


    void iterateConstraints(JPH::Ragdoll* ragdoll) {


        size_t constraintCount = ragdoll->GetConstraintCount();

        for (int i = 0; i < constraintCount; i++) {

            JPH::TwoBodyConstraint* genericConstraint = ragdoll->GetConstraint(i);

            genericConstraint->GetBody1();
            genericConstraint->GetBody2();

            EConstraintSubType subType = genericConstraint->GetSubType();

            std::vector<MotorInfo> motorInfos;

            std::string typeName;


            switch (subType)
            {
            case JPH::EConstraintSubType::Fixed:
            {
                typeName = "Fixed";
                JPH::FixedConstraint* fixedConstraint = dynamic_cast<JPH::FixedConstraint*>(genericConstraint);
                //MotorSettings& motor = fixedConstraint->GetMotorSettings();
                break;
            }

            case JPH::EConstraintSubType::Point:
            {
                typeName = "Point";
                JPH::PointConstraint* constraint = dynamic_cast<JPH::PointConstraint*>(genericConstraint);
                break;

            }
            case JPH::EConstraintSubType::Hinge:
            {
                typeName = "Hinge";
                Ref<JPH::HingeConstraint> constraint = dynamic_cast<JPH::HingeConstraint*>(genericConstraint);
                const MotorSettings& motor = constraint->GetMotorSettings();
                MotorInfo& motorInfo = motorInfos.emplace_back();
                GetMotorInfo(motor, motorInfo, MotorType::Hinge);
                break;
            }

            case JPH::EConstraintSubType::Slider:
            {
                typeName = "Slider";
                Ref<JPH::SliderConstraint> constraint = dynamic_cast<JPH::SliderConstraint*>(genericConstraint);
                const MotorSettings& motor = constraint->GetMotorSettings();
                MotorInfo& motorInfo = motorInfos.emplace_back();
                GetMotorInfo(motor, motorInfo, MotorType::Slider);
                break;
            }

            case JPH::EConstraintSubType::Distance:
            {
                typeName = "Distance";
                Ref<JPH::DistanceConstraint> constraint = dynamic_cast<JPH::DistanceConstraint*>(genericConstraint);
                break;

            }
            case JPH::EConstraintSubType::Cone:
            {
                typeName = "Cone";
                Ref<JPH::ConeConstraint> constraint = dynamic_cast<JPH::ConeConstraint*>(genericConstraint);
                break;
            }
            case JPH::EConstraintSubType::SwingTwist:
            {
                typeName = "SwingTwist";
                Ref<JPH::SwingTwistConstraint> constraint = dynamic_cast<JPH::SwingTwistConstraint*>(genericConstraint);

                const MotorSettings& twistMotor = constraint->GetTwistMotorSettings();
                MotorInfo& motorInfo1 = motorInfos.emplace_back();
                GetMotorInfo(twistMotor, motorInfo1, MotorType::Twist);

                const MotorSettings& rotationMotor = constraint->GetSwingMotorSettings();
                MotorInfo& rotationMotorInfo = motorInfos.emplace_back();
                GetMotorInfo(rotationMotor, rotationMotorInfo, MotorType::Swing);

                break;
            }
            case JPH::EConstraintSubType::SixDOF:
            {
                typeName = "SixDOF";
                Ref<JPH::SixDOFConstraint> constraint = dynamic_cast<JPH::SixDOFConstraint*>(genericConstraint);

                MotorSettings rotMotorX = constraint->GetMotorSettings(SixDOFConstraint::EAxis::RotationX);
                MotorInfo& rotMotorXInfo = motorInfos.emplace_back();
                GetMotorInfo(rotMotorX, rotMotorXInfo, MotorType::RotationX);

                MotorSettings rotMotorY = constraint->GetMotorSettings(SixDOFConstraint::EAxis::RotationY);
                MotorInfo& rotMotorYInfo = motorInfos.emplace_back();
                GetMotorInfo(rotMotorY, rotMotorYInfo, MotorType::RotationY);

                MotorSettings rotMotorZ = constraint->GetMotorSettings(SixDOFConstraint::EAxis::RotationZ);
                MotorInfo& rotMotorZInfo = motorInfos.emplace_back();
                GetMotorInfo(rotMotorZ, rotMotorZInfo, MotorType::RotationZ);


                MotorSettings transMotorX = constraint->GetMotorSettings(SixDOFConstraint::EAxis::TranslationX);
                MotorInfo& transMotorXInfo = motorInfos.emplace_back();
                GetMotorInfo(transMotorX, transMotorXInfo, MotorType::TranslationX);

                MotorSettings transMotorY = constraint->GetMotorSettings(SixDOFConstraint::EAxis::TranslationY);
                MotorInfo& transMotorYInfo = motorInfos.emplace_back();
                GetMotorInfo(transMotorY, transMotorYInfo, MotorType::TranslationY);

                MotorSettings transMotorZ = constraint->GetMotorSettings(SixDOFConstraint::EAxis::TranslationZ);
                MotorInfo& transMotorZInfo = motorInfos.emplace_back();
                GetMotorInfo(transMotorZ, transMotorZInfo, MotorType::TranslationZ);

                break;
            }
            case JPH::EConstraintSubType::Path:
            {
                typeName = "Path";
                Ref<JPH::PathConstraint> constraint = dynamic_cast<JPH::PathConstraint*>(genericConstraint);
                const MotorSettings& motor = constraint->GetPositionMotorSettings();
                MotorInfo& motorInfo = motorInfos.emplace_back();
                GetMotorInfo(motor, motorInfo, MotorType::Path);
                break;
            }
            case JPH::EConstraintSubType::Vehicle:
                break;
            case JPH::EConstraintSubType::RackAndPinion:
                break;
            case JPH::EConstraintSubType::Gear:
                break;
            case JPH::EConstraintSubType::Pulley:
                break;
            default:
                break;
            }

            ImGui::Text("Constraint %d has type %s",i, typeName.c_str());
            displayMotorInfo(motorInfos);
            ImGui::SeparatorText("--------------");
        }

    }

    void GetMotorInfo(const MotorSettings & motor, MotorInfo& motorInfo, MotorType type) {

        //TODO account for NAN
        motorInfo.mMinForceLimit = motor.mMinForceLimit;
        motorInfo.mMaxForceLimit = motor.mMaxForceLimit;
        motorInfo.mMinTorqueLimit = motor.mMinTorqueLimit;
        motorInfo.mMaxTorqueLimit = motor.mMaxTorqueLimit;

        motorInfo.type = type;
    }

    void displayMotorInfo(const std::vector<MotorInfo> & motorInfos) {

        for (const MotorInfo& motorInfo : motorInfos) {

            ImGui::Text("Motor %s mMinTorqueLimit: %f", magic_enum::enum_name(motorInfo.type).data(), motorInfo.mMinTorqueLimit);
            ImGui::Text("Motor %d mMaxTorqueLimit: %f", magic_enum::enum_name(motorInfo.type).data(),  motorInfo.mMaxTorqueLimit);
            //ImGui::Text("Motor %d mMinForceLimit: %f", motorInfo.type,  motorInfo.mMinForceLimit);
            //ImGui::Text("Motor %d mMaxForceLimit: %f", motorInfo.type,  motorInfo.mMaxForceLimit);
            ImGui::SeparatorText("");
        }
    }
};