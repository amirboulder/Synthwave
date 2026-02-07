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


//Used for shapeType dropdown text
static const std::map<ShapeType, std::string> ShapeTypeNames = {
	{ShapeType::Capsule, "Capsule"},
	{ShapeType::Sphere, "Sphere"},
	{ShapeType::Box, "Box"},
};

//Used for Attachment dropdown text
static const std::map<Attachment, std::string> AttachmentNames = {
	{Attachment::Top, "Top"},
	{Attachment::Bottom, "Bottom"},
	{Attachment::Left, "Left"},
	{Attachment::Right, "Right"},
	{Attachment::Front, "Front"},
	{Attachment::Back, "Back"},
	{Attachment::None, "None"},
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


		BodyPart* root = nullptr;

		BodyPart* selectedPart = nullptr;

		BodyPart* partToDelete = nullptr;

		float minBodySize = 0.1f;
		float maxBodySize = 10.0f;
		float sliderStep = 0.1f;

		//Attachment
		Attachment attachmentPrimaryAxis = Attachment::Right;
		Attachment attachmentSecondaryAxis = Attachment::None;
		Attachment attachmentTertiaryAxis = Attachment::None;

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

		//Root Position
		glm::vec3 rootPos = glm::vec3(1.0f, 3.0f, -3.0f);

		//Rotation
		glm::quat rotInput = glm::quat(1, 0, 0, 0);

		//Capsule
		float capsuleHeight = 0.3f;
		float capsuleRadius = 0.3f;
		uint32_t capsuleCounter = 0;

		//Sphere
		float sphereRadius = 0.1f;
		uint32_t sphereCounter = 0;

		//Box
		glm::vec3 boxExtents = glm::vec3(0.1f, 0.1f, 0.1f);
		uint32_t boxCounter = 0;

		char ragdollNameBuffer[128] = "";

	};
	static State s_state;



	static void draw(flecs::world& ecs)
	{
		ImGui::Begin("Ragdoll Builder", nullptr, ImGuiWindowFlags_NoCollapse);

		drawBeginCreation(ecs);

		ImGui::SameLine();
		drawReset(ecs);

		ImGui::SameLine();
		drawRagdollNameField();

		ImGui::SameLine();
		drawFinish(ecs);

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
				performDeletion(ecs);
			}

		}

		ImGui::End();
	}


	static void drawBeginCreation(flecs::world& ecs) {

		if (s_state.creating) {
			return;
		}

		float buttonWidth = 140.0f;
	
		if (ImGui::Button("Begin", ImVec2(buttonWidth, 0))) {

			s_state.creating = true;
		}
	}

	static void drawReset(flecs::world& ecs) {

		float buttonWidth = 140.0f;

		if (ImGui::Button("Reset", ImVec2(buttonWidth, 0))) {

			s_state.partToDelete = s_state.root;
		}
	}

	static void drawFinish(flecs::world& ecs) {

		float buttonWidth = 140.0f;

		bool disableCreateButton = false;

		if (strlen(s_state.ragdollNameBuffer) == 0) {
			disableCreateButton = true;
		}

		ImGui::BeginDisabled(disableCreateButton);
		if (ImGui::Button("Finish", ImVec2(buttonWidth, 0))) {
			if (!s_state.root) {
				return;
			}

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			RenderContext& renderContext = ecs.get_mut<RenderContext>();
			Ref<RagdollSettings> ragdollSettings = ragdoll::CreateHumanoidBFS(s_state.root);

			std::stringstream dataOut;
			JPH::StreamOutWrapper stream_out(dataOut);

			ragdollSettings->SaveBinaryState(stream_out, true, false);

			std::string filename = s_state.ragdollNameBuffer;
			filename.append(".bof");

			fs::path repoSavePath = util::getRepoRagdollsFolder();
			repoSavePath /= filename;

			fs::path buildSavePath = util::getBuildRagdollsFolder();
			buildSavePath /= filename;

			bool success = true;

			//Save in repo and in build folder, trigger an event so rescan the ragdolls folder so the file is available for usage.
			if (!util::saveDataToFile(dataOut, repoSavePath.string())) {

				success = false;
			}
			if (!util::saveDataToFile(dataOut, buildSavePath.string())) {

				success = false;
			}

			if (success) {
				ecs.set<RagdollSavedEvent>({ true });
			}

			//reset
			s_state.partToDelete = s_state.root;
		}
		ImGui::EndDisabled();
	}

	static void drawRagdollNameField() {

		ImGui::SetNextItemWidth(300.0f);
		ImGui::InputText("Ragdoll Name", s_state.ragdollNameBuffer,
			IM_ARRAYSIZE(s_state.ragdollNameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue);
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
			ImGui::PopStyleColor();  
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
			ImGui::PopStyleColor();  
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

		ImGui::SetNextItemWidth(250.0f);
		
		drawShapeTypeDropdown();

		switch (s_state.selectedShape) {
		case ShapeType::Capsule:

			drawCapsuleRootOptions(ecs);
			break;
		case ShapeType::Sphere:

			drawSphereRootOptions(ecs);
			break;
		case ShapeType::Box:

			drawBoxRootOptions(ecs);
			break;
		}

	}

	static void drawAddChildPart(flecs::world& ecs, BodyPart* selectedPart) {

		ImGui::SetNextItemWidth(250.0f);
		
		drawShapeTypeDropdown();

		switch (s_state.selectedShape) {
		case ShapeType::Capsule:

			drawCapsuleChildOptions(ecs, s_state.selectedPart);
			break;
		case ShapeType::Sphere:
			
			drawSphereChildOptions(ecs, s_state.selectedPart);
			break;
		case ShapeType::Box:

			drawBoxChildOptions(ecs, s_state.selectedPart);
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
			adjustChildren(ecs, s_state.selectedPart);
		}
		ImGui::SameLine();
		if (ImGui::Button(buttonLabelY)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));
			glm::quat newRot = JPHQuatToGLM(selectedPart->bodyPtr->GetRotation());
			newRot = delta * newRot;
			rotateSelected(ecs, s_state.selectedPart, newRot);
			adjustChildren(ecs, s_state.selectedPart);
		}
		ImGui::SameLine();
		if (ImGui::Button(buttonLabelZ)) {
			glm::quat delta = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1));
			glm::quat newRot = JPHQuatToGLM(selectedPart->bodyPtr->GetRotation());
			newRot = delta * newRot;
			rotateSelected(ecs, s_state.selectedPart, newRot);
			adjustChildren(ecs, s_state.selectedPart);
		}

	}


	static void rotateSelected(flecs::world& ecs, BodyPart* part, const glm::quat newRot) {

		JPH::BodyInterface& bi = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		JPH::Quat joltRotation = GLMQuatToJPH(newRot);
		bi.SetRotation(part->bodyPtr->GetID(), joltRotation, JPH::EActivation::Activate);

		// for child nodes
		if (part->parent) {

			JPH::Vec3  newPlacement = getAttachmentPos(part->parent, part->shape,
				JPHQuatToGLM(part->bodyPtr->GetRotation()), part->attachmentPrimaryAxis, part->attachmentSecondaryAxis, part->attachmentTertiaryAxis);

			bi.SetPosition(part->bodyPtr->GetID(), newPlacement, JPH::EActivation::Activate);

		}
	}

	static void adjustChildren(flecs::world& ecs, BodyPart* part) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		//Positions only need to be adjusted for parts child nodes
		for (int i = 0; i < part->children.size(); i++) {

			adjustPositionRecursive(bodyInterface, part->children[i]);
			
		}
		// constraint may need to part itself if it has a parent,as well as all of it children
		adjustConstraintsRecursive(physicsSystem, part);
	}

	
	static void adjustPositionRecursive(JPH::BodyInterface& bi, BodyPart* part) {
		
		if (!part->parent) {
			cout << "adjustPositionRecursive function called on root node!\n";
			return;
		}


		JPH::Vec3  newPlacement = getAttachmentPos(part->parent, part->shape,
			JPHQuatToGLM(part->bodyPtr->GetRotation()) , part->attachmentPrimaryAxis, part->attachmentSecondaryAxis, part->attachmentTertiaryAxis);

		bi.SetPosition(part->bodyPtr->GetID(), newPlacement, JPH::EActivation::Activate);

		for (int i = 0; i < part->children.size(); i++) {

			adjustPositionRecursive(bi, part->children[i]);
		}

	}

	//Adjust the constraint for part and all of its children
	static void adjustConstraintsRecursive(JPH::PhysicsSystem& physicsSystem, BodyPart* part) {

		//If child node then adjust constraint to parent
		if (part->parent) {
			
			physicsSystem.RemoveConstraint(part->constraint);
			switch (part->constraintType)
			{
			case EConstraintSubType::Fixed:
				addFixedConstraint(part->parent, part, physicsSystem);
				break;

			case EConstraintSubType::Hinge:
			{
				Ref<JPH::HingeConstraintSettings> constraintSettings = JPH::DynamicCast<JPH::HingeConstraintSettings>(part->constraintSettings);
				addHingeConstraint(part->parent, part, physicsSystem,
					constraintSettings->mHingeAxis1, constraintSettings->mNormalAxis1, constraintSettings->mLimitsMin,
					constraintSettings->mLimitsMax);
				break;
			}

			case EConstraintSubType::SwingTwist:
			{
				Ref<JPH::SwingTwistConstraintSettings> constraintSettings2 = JPH::DynamicCast<JPH::SwingTwistConstraintSettings>(part->constraintSettings);
				addSwintTwistConstraint(part->parent, part, physicsSystem,
					constraintSettings2->mTwistAxis1, constraintSettings2->mTwistMinAngle,
					constraintSettings2->mNormalHalfConeAngle, constraintSettings2->mPlaneHalfConeAngle);
				break;
			}
			}
		}
		

		for (int i = 0; i < part->children.size(); i++) {
			adjustConstraintsRecursive(physicsSystem, part->children[i]);
		}
	}


	static void drawCapsuleRootOptions(flecs::world& ecs) {

		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat(
			"CapsuleHeight",
			&s_state.capsuleHeight,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat(
			"CapsuleRadius",
			&s_state.capsuleRadius,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		ImGui::InputFloat3("Position", &(s_state.rootPos.x));

		drawRotationOptions(s_state.rotInput);

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//TODO Add some checks for size 

			JPH::Vec3 joltPosition(s_state.rootPos.x, s_state.rootPos.y, s_state.rootPos.z);
			JPH::Quat joltRotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
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

			s_state.root->name = name.append(std::to_string(s_state.capsuleCounter));
			s_state.root->parent = nullptr;
			s_state.root->shape = capsuleShape;
			s_state.selectedPart = s_state.root;

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		}
	}

	static void drawSphereRootOptions(flecs::world& ecs) {

		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat(
			"SphereRadius",
			&s_state.sphereRadius,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		ImGui::InputFloat3("Position", &(s_state.rootPos.x));

		drawRotationOptions(s_state.rotInput);

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//TODO Add some checks for size 

			JPH::Vec3 joltPosition(s_state.rootPos.x, s_state.rootPos.y, s_state.rootPos.z);
			JPH::Quat joltRotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
			if (!joltRotation.IsNormalized()) {
				joltRotation = joltRotation.Normalized();
			}

			Ref<Shape> SphereShape = new JPH::SphereShape(s_state.sphereRadius);
			JPH::BodyCreationSettings creationSettings(
				SphereShape,
				joltPosition,
				joltRotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Sphere";

			s_state.root = new BodyPart;

			s_state.root->bodyPtr = bodyInterface.CreateBody(creationSettings);
			bodyInterface.AddBody(s_state.root->bodyPtr->GetID(), JPH::EActivation::Activate);

			s_state.root->name = name.append(std::to_string(s_state.sphereCounter));
			s_state.root->parent = nullptr;
			s_state.root->shape = SphereShape;
			s_state.selectedPart = s_state.root;

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	static void drawBoxRootOptions(flecs::world& ecs) {

		ImGui::SetNextItemWidth(340.0f);
		ImGui::DragFloat3(
			"BoxExtents",
			&s_state.boxExtents.x,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		ImGui::InputFloat3("Position", &(s_state.rootPos.x));

		drawRotationOptions(s_state.rotInput);

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//Add some checks for size 
			JPH::Vec3 joltPosition(s_state.rootPos.x, s_state.rootPos.y, s_state.rootPos.z);
			JPH::Quat joltRotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
			if (!joltRotation.IsNormalized()) {
				joltRotation = joltRotation.Normalized();
			}

			Ref<Shape> shape = new JPH::BoxShape(GLMVec3ToJPH(s_state.boxExtents) / 2.0f);

			JPH::BodyCreationSettings creationSettings(
				shape,
				joltPosition,
				joltRotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Box";

			s_state.root = new BodyPart;

			s_state.root->bodyPtr = bodyInterface.CreateBody(creationSettings);
			bodyInterface.AddBody(s_state.root->bodyPtr->GetID(), JPH::EActivation::Activate);

			s_state.root->name = name.append(std::to_string(s_state.boxCounter));
			s_state.root->parent = nullptr;
			s_state.root->shape = shape;

			s_state.selectedPart = s_state.root;

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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

		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat("CapsuleHeight", &s_state.capsuleHeight,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat("CapsuleRadius", &s_state.capsuleRadius,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		drawRotationOptions(s_state.rotInput);

		drawAttachmentDropdown();
		drawConstraintDropdown();
		drawConstraintOptions();

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Child", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//Add some input validation

			Ref<Shape> capsuleShape = new JPH::CapsuleShape(s_state.capsuleHeight / 2.0f, s_state.capsuleRadius);



			JPH::Vec3  position = getAttachmentPos(parent, capsuleShape,
				s_state.rotInput, s_state.attachmentPrimaryAxis, s_state.attachmentSecondaryAxis, s_state.attachmentTertiaryAxis);

			JPH::Quat rotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
			if (!rotation.IsNormalized()) {
				rotation = rotation.Normalized();
			}

			JPH::BodyCreationSettings bodySettings(
				capsuleShape,
				position,
				rotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Capsule";

			BodyPart* part = new BodyPart;

			part->bodyPtr = bodyInterface.CreateBody(bodySettings);
			bodyInterface.AddBody(part->bodyPtr->GetID(), JPH::EActivation::Activate);

			part->attachmentPrimaryAxis = s_state.attachmentPrimaryAxis;
			part->attachmentSecondaryAxis = s_state.attachmentSecondaryAxis;
			part->attachmentTertiaryAxis = s_state.attachmentTertiaryAxis;

			switch (s_state.constraintType)
			{
			case EConstraintSubType::Fixed:
				addFixedConstraint(parent, part, physicsSystem);
				break;
			case EConstraintSubType::Hinge:
				addHingeConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.hingeAxis), getAxisFromRadioButton(s_state.hingeNormalAxis),
					s_state.hingeMinAngleRad, s_state.hingeMaxAngleRad);
				break;
			case EConstraintSubType::SwingTwist:
				addSwintTwistConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.twistAxis), s_state.twistAngleRad, s_state.normalAngleRad, s_state.planeAngleRad);
				break;
			}

			s_state.capsuleCounter += 1;
			part->name = name.append(std::to_string(s_state.capsuleCounter));
			part->shape = capsuleShape;
			part->parent = parent;

			s_state.selectedPart = part;

			parent->children.push_back(part);

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f,0.0f,0.0f,0.0f);
		}
	}

	static void drawSphereChildOptions(flecs::world& ecs, BodyPart* parent) {

		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat(
			"SphereRadius",
			&s_state.sphereRadius,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		drawRotationOptions(s_state.rotInput);

		drawAttachmentDropdown();
		drawConstraintDropdown();
		drawConstraintOptions();

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//TODO Add some checks for size 

			Ref<Shape> sphereShape = new JPH::SphereShape(s_state.sphereRadius);



			JPH::Vec3  position = getAttachmentPos(parent, sphereShape,
				s_state.rotInput, s_state.attachmentPrimaryAxis, s_state.attachmentSecondaryAxis, s_state.attachmentTertiaryAxis);

			JPH::Quat joltRotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
			if (!joltRotation.IsNormalized()) {
				joltRotation = joltRotation.Normalized();
			}

			JPH::BodyCreationSettings bodySettings(
				sphereShape,
				position,
				joltRotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Sphere";

			BodyPart* part = new BodyPart;

			part->bodyPtr = bodyInterface.CreateBody(bodySettings);
			bodyInterface.AddBody(part->bodyPtr->GetID(), JPH::EActivation::Activate);


			switch (s_state.constraintType)
			{
			case EConstraintSubType::Fixed:
				addFixedConstraint(parent, part, physicsSystem);
				break;
			case EConstraintSubType::Hinge:
				addHingeConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.hingeAxis), getAxisFromRadioButton(s_state.hingeNormalAxis),
					s_state.hingeMinAngleRad, s_state.hingeMaxAngleRad);
				break;
			case EConstraintSubType::SwingTwist:
				addSwintTwistConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.twistAxis), s_state.twistAngleRad, s_state.normalAngleRad, s_state.planeAngleRad);
				break;
			}

			s_state.sphereCounter += 1;
			part->name = name.append(std::to_string(s_state.sphereCounter));
			part->shape = sphereShape;
			part->parent = parent;
			part->attachmentPrimaryAxis = s_state.attachmentPrimaryAxis;
			part->attachmentSecondaryAxis = s_state.attachmentSecondaryAxis;
			part->attachmentTertiaryAxis = s_state.attachmentTertiaryAxis;

			s_state.selectedPart = part;

			parent->children.push_back(part);

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	static void drawBoxChildOptions(flecs::world& ecs, BodyPart* parent) {

		ImGui::SetNextItemWidth(340.0f);
		ImGui::DragFloat3(
			"BoxExtents",
			&s_state.boxExtents.x,
			s_state.sliderStep,
			s_state.minBodySize,
			s_state.maxBodySize,
			"%.3f",
			ImGuiSliderFlags_AlwaysClamp
		);

		drawRotationOptions(s_state.rotInput);

		drawAttachmentDropdown();
		drawConstraintDropdown();
		drawConstraintOptions();

		float buttonWidth = 140.0f;
		if (ImGui::Button("Add Shape", ImVec2(buttonWidth, 0))) {

			cout << "Adding : " << ShapeTypeNames.at(s_state.selectedShape) << std::endl;

			JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
			JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

			//Add some checks for size 

			Ref<Shape> boxShape = new JPH::BoxShape(GLMVec3ToJPH(s_state.boxExtents / 2.0f));


			JPH::Vec3  position = getAttachmentPos(parent, boxShape,
				s_state.rotInput, s_state.attachmentPrimaryAxis, s_state.attachmentSecondaryAxis, s_state.attachmentTertiaryAxis);

			JPH::Quat rotation(s_state.rotInput.x, s_state.rotInput.y, s_state.rotInput.z, s_state.rotInput.w);
			if (!rotation.IsNormalized()) {
				rotation = rotation.Normalized();
			}

			JPH::BodyCreationSettings bodySettings(
				boxShape,
				position,
				rotation,
				JPH::EMotionType::Dynamic,
				Layers::MOVING
			);

			string name = "Box";

			BodyPart* part = new BodyPart;

			part->bodyPtr = bodyInterface.CreateBody(bodySettings);
			bodyInterface.AddBody(part->bodyPtr->GetID(), JPH::EActivation::Activate);

			switch (s_state.constraintType)
			{
			case EConstraintSubType::Fixed:
				addFixedConstraint(parent, part, physicsSystem);
				break;
			case EConstraintSubType::Hinge:
				addHingeConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.hingeAxis), getAxisFromRadioButton(s_state.hingeNormalAxis),
					s_state.hingeMinAngleRad, s_state.hingeMaxAngleRad);
				break;
			case EConstraintSubType::SwingTwist:
				addSwintTwistConstraint(parent, part, physicsSystem,
					getAxisFromRadioButton(s_state.twistAxis), s_state.twistAngleRad, s_state.normalAngleRad, s_state.planeAngleRad);
				break;
			}

			s_state.boxCounter += 1;
			part->name = name.append(std::to_string(s_state.boxCounter));
			part->shape = boxShape;
			part->parent = parent;
			part->attachmentPrimaryAxis = s_state.attachmentPrimaryAxis;
			part->attachmentSecondaryAxis = s_state.attachmentSecondaryAxis;
			part->attachmentTertiaryAxis = s_state.attachmentTertiaryAxis;

			s_state.selectedPart = part;

			parent->children.push_back(part);

			//reset rotationInput
			s_state.rotInput = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
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
			flags |= ImGuiTreeNodeFlags_Leaf;  
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

	static void drawShapeTypeDropdown() {
		
		if (ImGui::BeginCombo("Shape", ShapeTypeNames.at(s_state.selectedShape).c_str())) {
			for (const auto& [shapeType, name] : ShapeTypeNames) {
				bool isSelected = (s_state.selectedShape == shapeType);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.selectedShape = shapeType;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	static void filterOptions(const Attachment attachment, std::map<Attachment, std::string> & filteredAttachments) {

		switch (attachment) {
		case Attachment::Right:
			filteredAttachments.erase(Attachment::Right);
			filteredAttachments.erase(Attachment::Left);
			break;
		case Attachment::Left:
			filteredAttachments.erase(Attachment::Left);
			filteredAttachments.erase(Attachment::Right);
			break;
		case Attachment::Top:
			filteredAttachments.erase(Attachment::Top);
			filteredAttachments.erase(Attachment::Bottom);
			break;
		case Attachment::Bottom:
			filteredAttachments.erase(Attachment::Bottom);
			filteredAttachments.erase(Attachment::Top);
			break;
		case Attachment::Front:
			filteredAttachments.erase(Attachment::Front);
			filteredAttachments.erase(Attachment::Back);
			break;
		case Attachment::Back:
			filteredAttachments.erase(Attachment::Back);
			filteredAttachments.erase(Attachment::Front);
			break;
		}

	}

	//TODO if nothing is selected then select none
	static void drawAttachmentDropdown() {

		std::map<Attachment, std::string> filteredAttachments = AttachmentNames;

		float itemWidth = 250.f;


		auto it = filteredAttachments.find(s_state.attachmentPrimaryAxis);
		const char* currentName1 = (it != filteredAttachments.end()) ? it->second.c_str() : filteredAttachments.at(Attachment::None).c_str();

		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::BeginCombo("Attachment Primary Axis", currentName1)) {
			for (const auto& [attachment, name] : filteredAttachments) {
				bool isSelected = (s_state.attachmentPrimaryAxis == attachment);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.attachmentPrimaryAxis = attachment;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		filterOptions(s_state.attachmentPrimaryAxis, filteredAttachments);

		it = filteredAttachments.find(s_state.attachmentSecondaryAxis);
		const char* currentName2 = (it != filteredAttachments.end()) ? it->second.c_str() : filteredAttachments.at(Attachment::None).c_str();

		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::BeginCombo("Attachment Secondary Axis", currentName2)) {
			for (const auto& [attachment, name] : filteredAttachments) {
				bool isSelected = (s_state.attachmentSecondaryAxis == attachment);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.attachmentSecondaryAxis = attachment;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		filterOptions(s_state.attachmentSecondaryAxis, filteredAttachments);

		it = filteredAttachments.find(s_state.attachmentTertiaryAxis);
		const char* currentName3 = (it != filteredAttachments.end()) ? it->second.c_str() : filteredAttachments.at(Attachment::None).c_str();

		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::BeginCombo("Attachment Tertiary Axis", currentName3)) {
			for (const auto& [attachment, name] : filteredAttachments) {
				bool isSelected = (s_state.attachmentTertiaryAxis == attachment);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.attachmentTertiaryAxis = attachment;
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

	static void addFixedConstraint(BodyPart* parent, BodyPart* part,
		JPH::PhysicsSystem& physicsSystem) {

			FixedConstraintSettings* constraintSettings = new FixedConstraintSettings;

			constraintSettings->mPoint1 = constraintSettings->mPoint2 = getConstraintPos(
				parent, part->attachmentPrimaryAxis,
				part->attachmentSecondaryAxis,
				part->attachmentTertiaryAxis);

			part->constraintSettings = constraintSettings;
			part->constraintType = EConstraintSubType::Fixed;

			part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
			physicsSystem.AddConstraint(part->constraint);

	}

	static void addHingeConstraint(BodyPart* parent, BodyPart* part,
		JPH::PhysicsSystem& physicsSystem, JPH::Vec3Arg hingeAxis,
		JPH::Vec3Arg hingeNormalAxis, float hingeMinAngleRad, float hingeMaxAngleRad) {

		HingeConstraintSettings* constraintSettings = new HingeConstraintSettings;

		constraintSettings->mPoint1 = constraintSettings->mPoint2 = getConstraintPos(
			parent, part->attachmentPrimaryAxis,
			part->attachmentSecondaryAxis,
			part->attachmentTertiaryAxis);

		constraintSettings->mHingeAxis1 = constraintSettings->mHingeAxis2 = hingeAxis;
		constraintSettings->mNormalAxis1 = constraintSettings->mNormalAxis2 = hingeNormalAxis;
		constraintSettings->mLimitsMin = DegreesToRadians(hingeMinAngleRad);
		constraintSettings->mLimitsMax = DegreesToRadians(hingeMaxAngleRad);

		part->constraintSettings = constraintSettings;
		part->constraintType = EConstraintSubType::Hinge;

		part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
		physicsSystem.AddConstraint(part->constraint);

	}

	//TwistAngle is symmetrical
	static void addSwintTwistConstraint(BodyPart* parent, BodyPart* part,
		JPH::PhysicsSystem& physicsSystem, JPH::Vec3Arg twistAxis,
		float twistAngleRad, float normalAngleRad, float planeAngleRad) {

		SwingTwistConstraintSettings* constraintSettings = new SwingTwistConstraintSettings;

		constraintSettings->mPosition1 = constraintSettings->mPosition2 = getConstraintPos(
			parent, part->attachmentPrimaryAxis,
			part->attachmentSecondaryAxis,
			part->attachmentTertiaryAxis);

		constraintSettings->mTwistAxis1 = constraintSettings->mTwistAxis2 = twistAxis;
		constraintSettings->mPlaneAxis1 = constraintSettings->mPlaneAxis2 = Vec3::sAxisZ();
		constraintSettings->mTwistMinAngle = min(-twistAngleRad,twistAngleRad);
		constraintSettings->mTwistMaxAngle = max(-twistAngleRad,twistAngleRad);
		constraintSettings->mNormalHalfConeAngle = normalAngleRad;
		constraintSettings->mPlaneHalfConeAngle = planeAngleRad;

		part->constraintSettings = constraintSettings;
		part->constraintType = EConstraintSubType::SwingTwist;

		part->constraint = constraintSettings->Create(*parent->bodyPtr, *part->bodyPtr);
		physicsSystem.AddConstraint(part->constraint);

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

	static JPH::Vec3 getAttachmentPos(const BodyPart* parent, const JPH::Shape* childShape,
		const glm::quat rotation, const Attachment PrimaryAxis, const Attachment SecondaryAxis, const Attachment TertiaryAxis) {

		if (!parent) {
			cout << " Error GetAttachmentPos called on root node!\n";
			return Vec3::sZero();
		}

		JPH::AABox parentAABB = parent->bodyPtr->GetWorldSpaceBounds();

		JPH::Vec3 pos = parent->bodyPtr->GetPosition();

		// Get child half-extents for the relevant axis
		JPH::Vec3 childExtents = GetShapeHalfExtents(childShape, rotation);


		GetAttachmentPrimaryAxis(parentAABB, pos, childExtents, PrimaryAxis);
		GetAttachmentSecondaryAxis(parentAABB, pos, childExtents, PrimaryAxis, SecondaryAxis);
		GetAttachmentSecondaryAxis(parentAABB, pos, childExtents, SecondaryAxis,  TertiaryAxis);

		return pos;

	}

	static void GetAttachmentPrimaryAxis(
		const JPH::AABox parentAABB, JPH::Vec3 & pos, JPH::Vec3 childExtents, const Attachment primaryAxis) {

		//Note for 1 axis attachments
		//When using parent's Max: add child extent
		//When using parent's Min: subtract child extent


		switch (primaryAxis) {
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
	}

	static void GetAttachmentSecondaryAxis(const JPH::AABox parentAABB, JPH::Vec3 & pos, JPH::Vec3 childExtents, 
		const Attachment primaryAxis, const Attachment secondaryAxis) {

		if (primaryAxis == secondaryAxis) {
			return;
		}

		//for secondary axis attachments
		//When using parent's min: add child extent
		//When using parent's max: subtract child extent
		switch (secondaryAxis) {
		case Attachment::Right:
			pos.SetX(parentAABB.mMax.GetX() - childExtents.GetX());
			break;
		case Attachment::Left:
			pos.SetX(parentAABB.mMin.GetX() + childExtents.GetX());
			break;
		case Attachment::Top:
			pos.SetY(parentAABB.mMax.GetY() - childExtents.GetY());
			break;
		case Attachment::Bottom:
			pos.SetY(parentAABB.mMin.GetY() + childExtents.GetY());
			break;
		case Attachment::Front:
			pos.SetZ(parentAABB.mMax.GetZ() - childExtents.GetZ());
			break;
		case Attachment::Back:
			pos.SetZ(parentAABB.mMin.GetZ() + childExtents.GetZ());
			break;

		}
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

	static JPH::Vec3 getConstraintPos(const BodyPart* parent, const Attachment primary, Attachment secondary, Attachment tertiary) {

		JPH::AABox parentAABB = parent->bodyPtr->GetWorldSpaceBounds();

		JPH::Vec3 pos = parent->bodyPtr->GetPosition();

		getConstraintPosAxis(parentAABB, pos, primary);
		getConstraintPosAxis(parentAABB, pos, secondary);
		getConstraintPosAxis(parentAABB, pos, tertiary);

		return pos;
	}

	static void getConstraintPosAxis(JPH::AABox parentAABB, JPH::Vec3 & pos, const Attachment side) {

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
		case Attachment::None:
			//Do nothing if constraint is NONE
			break;
		}
	}

	static void performDeletion(flecs::world& ecs) {

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

			//set all counters to zero
			s_state.capsuleCounter = 0;
			s_state.sphereCounter = 0;
			s_state.boxCounter = 0;
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

		//IF base shape then they will be set to zero later anyways!
		if (part->shape->GetSubType() == EShapeSubType::Capsule) {
			s_state.capsuleCounter --;
		}
		if (part->shape->GetSubType() == EShapeSubType::Sphere) {
			s_state.sphereCounter--;
		}
		if (part->shape->GetSubType() == EShapeSubType::Box) {
			s_state.boxCounter--;
		}


		// Smart pointers (Ref<>) clean themselves up automatically
		delete part;
	}
};

// Initialize static state
RagdollBuilder::State RagdollBuilder::s_state;