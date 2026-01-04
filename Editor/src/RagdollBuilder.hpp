#pragma once

#include "core/src/pch.h"
#include "core/src/physics/ragdoll.hpp"
#include "core/src/util/util.hpp"

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

//TODO change to a map like below
constexpr EnumEntry ShapeTypeMeta[] = {
	{ ShapeType::Capsule, "Capsule", "Primitive" },
	{ ShapeType::Sphere,  "Sphere",  "Primitive" },
	{ ShapeType::Box,     "Box",     "Primitive" },
};


//Used for Attachment dropdown text
static const std::map<Attachment, std::string> AttachmentNames = {
	{Attachment::Top, "Top"},
	{Attachment::Bottom, "Bottom"},
	{Attachment::Left, "Left"},
	{Attachment::Right, "Right"},
	{Attachment::Front, "Front"},
	{Attachment::Back, "Back"}
};

//Used for Constraint dropdown text
static const std::map<JPH::EConstraintSubType, std::string> constraintNames = {
	{JPH::EConstraintSubType::Fixed, "Fixed"},
	{JPH::EConstraintSubType::Hinge, "Hinge"},
	{JPH::EConstraintSubType::SwingTwist, "SwingTwist"},
};


class RagdollBuilder {

public:

	struct State {

		ShapeType selectedShape = ShapeType::Capsule;
		bool creating = false;
		bool modifying = false;
		bool addingChild = false;
		float windowWidth = 0;

		uint32_t skeletonJointIndex = 0;
		BodyPart* root = nullptr;

		BodyPart* selectedPart = nullptr;

		BodyPart* partToDelete = nullptr;

		//Attachment
		Attachment selectedAttachment = Attachment::Right;

		//Constraint
		JPH::EConstraintSubType constraintType = JPH::EConstraintSubType::Fixed;

		//SwingTwist
		int twistAxis = 0;
		float twistAngleRad = 0.0f;
		float normalAngleRad = 0.0f;
		float planeAngleRad = 0.0f;

		//Hinge
		int hingeAxis = 0;
		int hingeNormalAxis = 0;
		float hingeMinAngleRad = 0.0f;
		float hingeMaxAngleRad = 0.0f;

		//Capsule
		float capsuleHeight = 0.3f;
		float capsuleRadius = 0.3f;
		glm::vec3 capsulePos = glm::vec3(1, 3, -3);
		glm::quat capsuleRot = glm::quat(1, 0, 0, 0);

		//Sphere
		float sphereRadius = 0.0f;
		glm::vec3 spherePos = glm::vec3();
		glm::quat sphereRot = glm::quat();

	};
	static State s_state;



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
			drawTree(ecs, s_state.root);
			ImGui::EndChild();

			// Right column (Details / Creation UI)
			ImGui::TableNextColumn();
			ImGui::BeginChild("##DetailsPane", ImVec2(0, 0), true);

			if (s_state.creating)
			{
				//If a part is selected show modify and add child buttons
				if (s_state.selectedPart) {

					drawModifyOrAddChild();
					ImGui::Spacing();

					if (s_state.modifying) {
						drawModifyObject(ecs, s_state.selectedPart);
					}
					else if (s_state.addingChild) {
						drawAddChildPart(ecs, s_state.selectedPart);

					}
				}
				else {
					drawAddRoot(ecs);
				}
			}

			ImGui::EndChild();

			ImGui::EndTable();

			if (s_state.partToDelete) {
				performDeletion(ecs, s_state.partToDelete);
			}
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

	static void drawModifyOrAddChild() {
		float buttonWidth = 140.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalWidth = buttonWidth * 2 + spacing;
		float availWidth = ImGui::GetContentRegionAvail().x;

		// Center both buttons
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - totalWidth) * 0.5f);

		// Highlight if selected
		if (s_state.modifying) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			ImGui::Button("Modify", ImVec2(buttonWidth, 0));
			ImGui::PopStyleColor();  // Pop immediately after
		}
		else {
			ImGui::Button("Modify", ImVec2(buttonWidth, 0));
		}

		if (ImGui::IsItemClicked()) {
			s_state.modifying = true;
			s_state.addingChild = false;
		}

		ImGui::SameLine();

		// Highlight if selected
		if (s_state.addingChild) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			ImGui::Button("AddChild", ImVec2(buttonWidth, 0));
			ImGui::PopStyleColor();  // Pop immediately after
		}
		else {
			ImGui::Button("AddChild", ImVec2(buttonWidth, 0));
		}

		if (ImGui::IsItemClicked()) {
			s_state.modifying = false;
			s_state.addingChild = true;
		}
	}


	static void drawAddRoot(flecs::world& ecs) {

		float windowWidth = ImGui::GetWindowSize().x;
		const char* entTypeTxt = "Shape Type Type:";
		float textWidth = ImGui::CalcTextSize(entTypeTxt).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::SetNextItemWidth(250.0f);
		int typeIndex = static_cast<int>(s_state.selectedShape);
		if (ImGui::BeginCombo("Shape Type", ShapeTypeMeta[typeIndex].name)) {
			for (int i = 0; i < static_cast<int>(ShapeType::COUNT); i++) {
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

			drawCapsuleRootOptions(ecs);
			break;
		case ShapeType::Sphere:

			drawSphereOptions(ecs);
			break;
		case ShapeType::Box:

			drawBoxOptions(ecs);
			break;
		}

	}

	static void drawAddChildPart(flecs::world& ecs, BodyPart* selectedPart) {

		float windowWidth = ImGui::GetWindowSize().x;
		const char* entTypeTxt = " Child Shape Type Type:";
		float textWidth = ImGui::CalcTextSize(entTypeTxt).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::SetNextItemWidth(250.0f);
		int typeIndex = static_cast<int>(s_state.selectedShape);
		if (ImGui::BeginCombo("Shape Type", ShapeTypeMeta[typeIndex].name)) {
			for (int i = 0; i < static_cast<int>(ShapeType::COUNT); i++) {
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

			drawCapsuleChildOptions(ecs, s_state.selectedPart);
			break;
		case ShapeType::Sphere:

			//drawSphereOptions(ecs);
			break;
		case ShapeType::Box:

			// drawBoxOptions(ecs);
			break;
		}

	}

	static void drawModifyObject(flecs::world& ecs, BodyPart* selectedPart) {

		if (!selectedPart) {
			return;
		}

		glm::vec3 rotationEulerRadians = glm::eulerAngles(JPHQuatToGLM(selectedPart->bodyPtr->GetRotation()));
		glm::vec3 rotationEulerDegrees = glm::degrees(rotationEulerRadians);

		// Create button labels with current angles
		char buttonLabelX[64];
		char buttonLabelY[64];
		char buttonLabelZ[64];
		snprintf(buttonLabelX, sizeof(buttonLabelX), "Rotate X +90° (%.1f°)", rotationEulerDegrees.x);
		snprintf(buttonLabelY, sizeof(buttonLabelY), "Rotate Y +90° (%.1f°)", rotationEulerDegrees.y);
		snprintf(buttonLabelZ, sizeof(buttonLabelZ), "Rotate Z +90° (%.1f°)", rotationEulerDegrees.z);

		if (ImGui::Button(buttonLabelX)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));

			glm::quat newRot = JPHQuatToGLM(selectedPart->bodyPtr->GetRotation());
			newRot = delta * newRot;
			rotateSelected(ecs, s_state.selectedPart, newRot);
		}
		if (ImGui::Button(buttonLabelY)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
			glm::quat newRot = JPHQuatToGLM(selectedPart->bodyPtr->GetRotation());
			newRot = delta * newRot;
			rotateSelected(ecs, s_state.selectedPart, newRot);
		}
		if (ImGui::Button(buttonLabelZ)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
			glm::quat newRot = JPHQuatToGLM(selectedPart->bodyPtr->GetRotation());
			newRot = delta * newRot;
			rotateSelected(ecs, s_state.selectedPart, newRot);
		}

	}

	static void rotateSelected(flecs::world& ecs, BodyPart* selectedPart, const glm::quat newRot) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		JPH::Quat joltRotation = GLMQuatToJPH(newRot);
		bodyInterface.SetRotation(selectedPart->bodyPtr->GetID(), joltRotation, JPH::EActivation::DontActivate);
	}


	static void drawCapsuleRootOptions(flecs::world& ecs) {

		ImGui::InputFloat("CapsuleHeight", &s_state.capsuleHeight, 0.1f, 1.0f, "%.3f");
		ImGui::InputFloat("CapsuleRadius", &s_state.capsuleRadius, 0.1f, 1.0f, "%.3f");

		ImGui::InputFloat3("Position", &(s_state.capsulePos.x));

		drawRotationOptions(s_state.capsuleRot);

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			//Add shape to Ragdoll here
			int  selectedShapeInt = static_cast<int>(s_state.selectedShape);
			cout << "Adding : " << ShapeTypeMeta[selectedShapeInt].name << std::endl;

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//TODO Add some checks for size 

			JPH::Vec3 joltPosition(s_state.capsulePos.x, s_state.capsulePos.y, s_state.capsulePos.z);
			JPH::Quat joltRotation(s_state.capsuleRot.x, s_state.capsuleRot.y, s_state.capsuleRot.z, s_state.capsuleRot.w);
			if (!joltRotation.IsNormalized()) {
				joltRotation = joltRotation.Normalized();
			}

			Ref<Shape> capsuleShape = new JPH::CapsuleShape(s_state.capsuleHeight / 2.0f, s_state.capsuleRadius);

			JPH::BodyCreationSettings creationSettings(
				capsuleShape,
				joltPosition,
				joltRotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Capsule";

			s_state.root = new BodyPart;

			s_state.root->bodyPtr = bodyInterface.CreateBody(creationSettings);
			bodyInterface.AddBody(s_state.root->bodyPtr->GetID(), JPH::EActivation::Activate);

			s_state.root->name = name.append(std::to_string(s_state.skeletonJointIndex));
			s_state.root->skeletonJointIndex = s_state.skeletonJointIndex;
			s_state.root->parent = nullptr;
			s_state.root->shape = capsuleShape;
			s_state.selectedPart = s_state.root;

		}
	}


	static void drawCapsuleChildOptions(flecs::world& ecs, BodyPart* parent) {

		if (!parent) {
			cout << "Parent is nullptr!!!\n";
			return;
		}
		if (!s_state.selectedPart) {

			ImGui::Text("Select Part");
			return;
		}

		ImGui::InputFloat("CapsuleHeight", &s_state.capsuleHeight, 0.1f, 1.0f, "%.3f");
		ImGui::InputFloat("CapsuleRadius", &s_state.capsuleRadius, 0.1f, 1.0f, "%.3f");

		drawRotationOptions(s_state.capsuleRot);

		drawAttachmentDropdown();

		drawConstraintDropdown();
		drawConstraintOptions();

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Child", ImVec2(buttonWidth, 0))) {

			//Add shape to Ragdoll here
			int  selectedShapeInt = static_cast<int>(s_state.selectedShape);
			cout << "Adding : " << ShapeTypeMeta[selectedShapeInt].name << std::endl;

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//Add some checks for size 

		   // JPH::Vec3 joltPosition(s_state.capsulePos.x, s_state.capsulePos.y, s_state.capsulePos.z);
			JPH::Quat childRotation(s_state.capsuleRot.x, s_state.capsuleRot.y, s_state.capsuleRot.z, s_state.capsuleRot.w);
			if (!childRotation.IsNormalized()) {
				childRotation = childRotation.Normalized();
			}

			Ref<Shape> capsuleShape = new JPH::CapsuleShape(s_state.capsuleHeight / 2.0f, s_state.capsuleRadius);

			JPH::Vec3  childPlacement = GetAttachmentPos(parent, capsuleShape,
				s_state.capsuleRot, s_state.selectedAttachment);

			JPH::BodyCreationSettings bodySettings(
				capsuleShape,
				childPlacement,
				childRotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			BodyPart* part = new BodyPart;

			string name = "Capsule";
			part->bodyPtr = bodyInterface.CreateBody(bodySettings);
			bodyInterface.AddBody(part->bodyPtr->GetID(), JPH::EActivation::Activate);

			addConstraint(parent, part, physicsSystem);

			s_state.skeletonJointIndex += 1;
			part->skeletonJointIndex = s_state.skeletonJointIndex;
			part->name = name.append(std::to_string(s_state.skeletonJointIndex));
			part->shape = capsuleShape;
			part->parent = parent;

			s_state.selectedPart = part;

			parent->children.push_back(part);

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

		}
	}

	static void drawTree(flecs::world& ecs, BodyPart* part) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		if (!part) {
			return;
		}

		bool hasChildren = part->children.size() > 0;
		if (!hasChildren) {
			flags |= ImGuiTreeNodeFlags_Leaf;  // Use |= instead of +=
		}

		// CHECK SELECTION BEFORE RENDERING
		if (s_state.selectedPart == part) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool nodeOpen = ImGui::TreeNodeEx(part->name.c_str(), flags);

		//!ImGui::IsItemToggledOpen() checks that the click did NOT toggle the node open/closed 
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			s_state.selectedPart = part;
		}

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Copy")) {
				std::cout << "Duplicating: " << std::endl;
			}
			if (ImGui::MenuItem("Delete")) {
				std::cout << "Deleting: " << part->name << std::endl;
				s_state.partToDelete = part;
			}
			ImGui::EndPopup();
		}

		if (nodeOpen) {
			for (int i = 0; i < part->children.size(); i++) {
				drawTree(ecs, part->children[i]);
			}
			ImGui::TreePop();
		}

	}

	static void drawRotationOptions(glm::quat& rotationQuat) {

		// Calculate current rotation angles
		glm::vec3 rotationEulerRadians = glm::eulerAngles(rotationQuat);
		glm::vec3 rotationEulerDegrees = glm::degrees(rotationEulerRadians);

		// Create button labels with current angles
		char buttonLabelX[64];
		char buttonLabelY[64];
		char buttonLabelZ[64];
		snprintf(buttonLabelX, sizeof(buttonLabelX), "Rotate X +90° (%.1f°)", rotationEulerDegrees.x);
		snprintf(buttonLabelY, sizeof(buttonLabelY), "Rotate Y +90° (%.1f°)", rotationEulerDegrees.y);
		snprintf(buttonLabelZ, sizeof(buttonLabelZ), "Rotate Z +90° (%.1f°)", rotationEulerDegrees.z);

		// Use the dynamic labels in buttons
		if (ImGui::Button(buttonLabelX)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
			rotationQuat = delta * rotationQuat;
		}
		ImGui::SameLine();
		if (ImGui::Button(buttonLabelY)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
			rotationQuat = delta * rotationQuat;
		}
		ImGui::SameLine();
		if (ImGui::Button(buttonLabelZ)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
			rotationQuat = delta * rotationQuat;
		}
	}

	static void drawAttachmentDropdown() {

		if (ImGui::BeginCombo("Attachment", AttachmentNames.at(s_state.selectedAttachment).c_str())) {
			for (const auto& [attachment, name] : AttachmentNames) {
				bool isSelected = (s_state.selectedAttachment == attachment);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.selectedAttachment = attachment;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	static void drawConstraintDropdown() {

		// Get the current constraint name safely
		auto it = constraintNames.find(s_state.constraintType);
		const char* currentName = (it != constraintNames.end()) ? it->second.c_str() : "Unknown";

		if (ImGui::BeginCombo("Constraint", currentName)) {
			for (const auto& [constraint, name] : constraintNames) {
				bool isSelected = (s_state.constraintType == constraint);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.constraintType = constraint;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

	}

	static void drawConstraintOptions() {

		float angles[2] = { s_state.hingeMinAngleRad, s_state.hingeMaxAngleRad };

		switch (s_state.constraintType) {
		case JPH::EConstraintSubType::Fixed:

			break;
		case JPH::EConstraintSubType::Hinge:

			ImGui::Text("Hinge Axis");
			ImGui::RadioButton("X-axis##1", &s_state.hingeAxis, 0); ImGui::SameLine();
			ImGui::RadioButton("Y-axis##1", &s_state.hingeAxis, 1); ImGui::SameLine();
			ImGui::RadioButton("Z-axis##1", &s_state.hingeAxis, 2);

			ImGui::Text("Normal Axis");
			ImGui::RadioButton("X-axis##2", &s_state.hingeNormalAxis, 0); ImGui::SameLine();
			ImGui::RadioButton("Y-axis##2", &s_state.hingeNormalAxis, 1); ImGui::SameLine();
			ImGui::RadioButton("Z-axis##2", &s_state.hingeNormalAxis, 2);


			if (ImGui::SliderFloat2("Angle Limits", angles, -180, 180, "%.1f°")) {
				s_state.hingeMinAngleRad = angles[0];
				s_state.hingeMaxAngleRad = angles[1];
				// Ensure min <= max
				if (s_state.hingeMinAngleRad > s_state.hingeMaxAngleRad) {
					std::swap(s_state.hingeMinAngleRad, s_state.hingeMaxAngleRad);
				}
			}

			break;
		case JPH::EConstraintSubType::SwingTwist:

			ImGui::Text("Twist Axis");
			ImGui::RadioButton("X-axis##3", &s_state.twistAxis, 0); ImGui::SameLine();
			ImGui::RadioButton("Y-axis##3", &s_state.twistAxis, 1); ImGui::SameLine();
			ImGui::RadioButton("Z-axis##3", &s_state.twistAxis, 2);

			ImGui::SliderAngle("Twist Angle", &s_state.twistAngleRad, -180.0f, 180.0f); // displayed in degrees
			ImGui::SliderAngle("Normal Angle", &s_state.normalAngleRad, -180.0f, 180.0f);
			ImGui::SliderAngle("Plane Angle", &s_state.planeAngleRad, -180.0f, 180.0f);

			break;
		}

	}

	static void addConstraint(BodyPart* parent, BodyPart* part, JPH::PhysicsSystem& physicsSystem) {

		if (s_state.constraintType == EConstraintSubType::Fixed) {

			FixedConstraintSettings* constraintSettings = new FixedConstraintSettings;

			constraintSettings->mPoint1 = constraintSettings->mPoint2 = getConstraintPos(parent, s_state.selectedAttachment);

			part->constraintSettings = constraintSettings;
			part->constraintType = EConstraintSubType::Fixed;

			part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
			physicsSystem.AddConstraint(part->constraint);

		}
		if (s_state.constraintType == EConstraintSubType::Hinge) {

			HingeConstraintSettings* constraintSettings = new HingeConstraintSettings;

			constraintSettings->mPoint1 = constraintSettings->mPoint2 = getConstraintPos(parent, s_state.selectedAttachment);
			constraintSettings->mHingeAxis1 = constraintSettings->mHingeAxis2 = getAxisFromRadioButton(s_state.hingeAxis);
			constraintSettings->mNormalAxis1 = constraintSettings->mNormalAxis2 = getAxisFromRadioButton(s_state.hingeNormalAxis);
			constraintSettings->mLimitsMin = DegreesToRadians(s_state.hingeMinAngleRad);
			constraintSettings->mLimitsMax = DegreesToRadians(s_state.hingeMaxAngleRad);

			part->constraintSettings = constraintSettings;
			part->constraintType = EConstraintSubType::Hinge;

			part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
			physicsSystem.AddConstraint(part->constraint);

		}

		if (s_state.constraintType == EConstraintSubType::SwingTwist) {

			SwingTwistConstraintSettings* constraintSettings = new SwingTwistConstraintSettings;

			constraintSettings->mPosition1 = constraintSettings->mPosition2 = getConstraintPos(parent, s_state.selectedAttachment);
			constraintSettings->mTwistAxis1 = constraintSettings->mTwistAxis2 = getAxisFromRadioButton(s_state.twistAxis);
			constraintSettings->mPlaneAxis1 = constraintSettings->mPlaneAxis2 = Vec3::sAxisZ();
			constraintSettings->mTwistMinAngle = min(-s_state.twistAngleRad, s_state.twistAngleRad);
			constraintSettings->mTwistMaxAngle = max(-s_state.twistAngleRad, s_state.twistAngleRad);
			constraintSettings->mNormalHalfConeAngle = s_state.normalAngleRad;
			constraintSettings->mPlaneHalfConeAngle = s_state.planeAngleRad;

			part->constraintSettings = constraintSettings;
			part->constraintType = EConstraintSubType::SwingTwist;

			part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
			physicsSystem.AddConstraint(part->constraint);
		}

	}

	static JPH::Vec3 getAxisFromRadioButton(const int axis) {

		if (axis >= 3) {
			cout << "Error in getAxisFromRadioButton invalid input!!!\n";
			return Vec3::sZero();
		}

		if (axis == 0) {return Vec3::sAxisX();}
		if (axis == 1) {return Vec3::sAxisY();}
		if (axis == 2) { return Vec3::sAxisZ();}
	}

	static JPH::Vec3 GetAttachmentPos(const BodyPart* parent, const JPH::Shape* childShape,
		const glm::quat rotation, const Attachment side) {

		JPH::AABox parentAABB = parent->bodyPtr->GetWorldSpaceBounds();

		JPH::Vec3 pos = parent->bodyPtr->GetPosition();

		// Get child half-extents for the relevant axis
		JPH::Vec3 childExtents = GetShapeHalfExtents(childShape, rotation);

		switch (side) {
		case Attachment::Right:
			pos.SetX(parentAABB.mMax.GetX() + childExtents.GetX());
			break;
		case Attachment::Left:
			pos.SetX(parentAABB.mMin.GetX() - childExtents.GetX());
			break;
		case Attachment::Top:
			pos.SetY(parentAABB.mMax.GetY() + childExtents.GetY());
			break;
		case Attachment::Bottom:
			pos.SetY(parentAABB.mMin.GetY() - childExtents.GetY());
			break;
		case Attachment::Front:
			pos.SetZ(parentAABB.mMax.GetZ() + childExtents.GetZ());
			break;
		case Attachment::Back:
			pos.SetZ(parentAABB.mMin.GetZ() - childExtents.GetZ());
			break;
		}

		return pos;
	}

	static JPH::Vec3 GetShapeHalfExtents(const JPH::Shape* childShape, const glm::quat rotation) {
		JPH::AABox localAABB = childShape->GetLocalBounds();

		// Convert glm::quat to JPH::Quat
		JPH::Quat jphRotation = GLMQuatToJPH(rotation);

		// Get the 8 corners of the local AABB
		JPH::Vec3 corners[8];
		corners[0] = JPH::Vec3(localAABB.mMin.GetX(), localAABB.mMin.GetY(), localAABB.mMin.GetZ());
		corners[1] = JPH::Vec3(localAABB.mMax.GetX(), localAABB.mMin.GetY(), localAABB.mMin.GetZ());
		corners[2] = JPH::Vec3(localAABB.mMin.GetX(), localAABB.mMax.GetY(), localAABB.mMin.GetZ());
		corners[3] = JPH::Vec3(localAABB.mMax.GetX(), localAABB.mMax.GetY(), localAABB.mMin.GetZ());
		corners[4] = JPH::Vec3(localAABB.mMin.GetX(), localAABB.mMin.GetY(), localAABB.mMax.GetZ());
		corners[5] = JPH::Vec3(localAABB.mMax.GetX(), localAABB.mMin.GetY(), localAABB.mMax.GetZ());
		corners[6] = JPH::Vec3(localAABB.mMin.GetX(), localAABB.mMax.GetY(), localAABB.mMax.GetZ());
		corners[7] = JPH::Vec3(localAABB.mMax.GetX(), localAABB.mMax.GetY(), localAABB.mMax.GetZ());

		// Rotate all corners and find the new AABB
		JPH::Vec3 rotatedMin = jphRotation * corners[0];
		JPH::Vec3 rotatedMax = rotatedMin;

		for (int i = 1; i < 8; i++) {
			JPH::Vec3 rotatedCorner = jphRotation * corners[i];
			rotatedMin = JPH::Vec3::sMin(rotatedMin, rotatedCorner);
			rotatedMax = JPH::Vec3::sMax(rotatedMax, rotatedCorner);
		}

		// Return half extents of the rotated AABB
		return (rotatedMax - rotatedMin) * 0.5f;
	}

	static JPH::Vec3 getConstraintPos(const BodyPart* parent, const Attachment side) {

		JPH::AABox parentAABB = parent->bodyPtr->GetWorldSpaceBounds();

		JPH::Vec3 pos = parent->bodyPtr->GetPosition();

		switch (side) {
		case Attachment::Right:
			pos.SetX(parentAABB.mMax.GetX());
			break;
		case Attachment::Left:
			pos.SetX(parentAABB.mMin.GetX());
			break;
		case Attachment::Top:
			pos.SetY(parentAABB.mMax.GetY());
			break;
		case Attachment::Bottom:
			pos.SetY(parentAABB.mMin.GetY());
			break;
		case Attachment::Front:
			pos.SetZ(parentAABB.mMax.GetZ());
			break;
		case Attachment::Back:
			pos.SetZ(parentAABB.mMin.GetZ());
			break;
		}

		return pos;
	}

	static void performDeletion(flecs::world& ecs, BodyPart* root) {
		if (!s_state.partToDelete) {
			return;
		}

		// Remove from parent's children vector
		if (s_state.partToDelete->parent) {
			auto& siblings = s_state.partToDelete->parent->children;
			siblings.erase(
				std::remove(siblings.begin(), siblings.end(), s_state.partToDelete),
				siblings.end()
			);
		}

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();


		// Now safe to delete
		deletePart(physicsSystem, bodyInterface, s_state.partToDelete);

		//If deleting root then we set s_state.root because drawTree uses that pointer.
		if (s_state.partToDelete == s_state.root) {
			s_state.root = nullptr;
			s_state.creating = false;
		}

		s_state.partToDelete = nullptr;
	}

	static void deletePart(JPH::PhysicsSystem& physicsSystem, JPH::BodyInterface& bi, BodyPart* part) {
		if (!part) return;

		// Recursively delete children
		for (BodyPart* child : part->children) {
			deletePart(physicsSystem, bi, child);
		}

		//Keeps selecting the parent of the part being deleted if there is one.
		if (s_state.selectedPart == part) {

			if (s_state.selectedPart->parent) {
				s_state.selectedPart = s_state.selectedPart->parent;
			}
			else {
				s_state.selectedPart = nullptr;
			}
		}

		//Remove if there is a constraint, for example root does not have a constraint
		if (part->constraint) {
			physicsSystem.RemoveConstraint(part->constraint);
		}

		if (bi.IsAdded(part->bodyPtr->GetID())) {
			bi.RemoveBody(part->bodyPtr->GetID());
		}

		// Smart pointers (Ref<>) clean themselves up automatically
		delete part;
	}
};

// Initialize static state
RagdollBuilder::State RagdollBuilder::s_state;