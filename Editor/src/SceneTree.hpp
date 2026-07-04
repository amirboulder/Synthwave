#pragma once


std::unordered_map<std::string, entUpdateFn> updateFunctions{
	{"ragdollUpdate", Scripts::ragdollUpdate},
	{"updateRagdollNoAnim", Scripts::updateRagdollNoAnim},
	{"updateRagdollKinematic", Scripts::updateRagdollKinematic},
	{"updateRagdollNoAnim", Scripts::empty},

};


class SceneTree {

public:

	// State management
	//TODO make part of the editor or ecs
	struct State {
		flecs::entity selectedEntity;
		flecs::entity contextEntity;  // Entity that opened the context menu
		char childNameBuffer[128] = "";
		bool showAddChildPopup = false;
		EntityType selectedType = EntityType::Empty;
		bool isNameValid = true;
		char errorMessage[256] = "";

		glm::vec3 childPosition = { -3.f, 5.f, -3.f };
		glm::vec3 childRotation = { 0.f, 0.f, 0.f };  // Euler degrees
		glm::vec3 childScale = { 1.f, 1.f, 1.f };

		glm::vec3 childPositionDefault = { -3.f, 5.f, -3.f };

		string selectedUpdatefuncName;
		entUpdateFn selectedUpdatefunc;

		string selectedRagdoll;
	};

	static State s_state;

	// Adds emojis to each entity
	static const char* GetEntityIcon(flecs::entity entity) {

		if (!entity.has<EntityTypeComponent>()) return " ";

		EntityType type = entity.get<EntityTypeComponent>().type;

		if (type == EntityType::Game) return "🌎";
		if (type == EntityType::Scene) return "🎬";
		if (type == EntityType::Cube) return "📦";
		if (type == EntityType::Capsule) return "💊";
		if (type == EntityType::Humanoid) return "🧍";
		if (type == EntityType::Player) return "👤";
		if (type == EntityType::Camera) return "🎥";
		if (type == EntityType::Grid) return "🟪";
		if (type == EntityType::StaticMesh) return "⛰️";
		if (type == EntityType::Actor) return "🎭";
		if (type == EntityType::Sensor) return "📡";
		if (type == EntityType::Light) return "💡";

		return "  ";
	}

	static bool HasChildren(flecs::world& ecs, flecs::entity entity) {
		ecs_iter_t it = ecs_each_pair(ecs, EcsChildOf, entity);
		return ecs_iter_is_true(&it);
	}

	static void SceneTreeDraw(flecs::world& ecs) {
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

			std::string entityTypeName;
			auto entTypeComp = s_state.selectedEntity.try_get<EntityTypeComponent>();
			if (entTypeComp) {
				entityTypeName = magic_enum::enum_name(entTypeComp->type);
			}

			ImGui::Text("Selected: %s %s", entityTypeName.c_str(),  s_state.selectedEntity.name().c_str());

			if (Transform* entTransform = s_state.selectedEntity.try_get_mut<Transform>()) {
				DragFloat3XYZ("Position", entTransform->position);

				glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(entTransform->rotation));
				if (DragFloat3XYZ("Rotation", eulerDeg)) {
					entTransform->rotation = rotationFromEulerDegrees(eulerDeg);
				}
			}

		}


		// must be called every frame
		drawAddChildPopup(ecs);

		ImGui::End();
	}

	static void DrawEntityNode(flecs::world& ecs, flecs::entity entity) {
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

	static void HandleEntitySelection(flecs::entity entity) {
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			s_state.selectedEntity = entity;
		}
	}

	static void HandleEntityDoubleClick(flecs::entity entity) {
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

	static void DrawContextMenu(flecs::world& ecs, flecs::entity entity) {
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
				std::cout << "Deleting not yet Implemented " << std::endl;
				// TODO: Implement deletion
			}

			if (ImGui::MenuItem("Duplicate")) {
				std::cout << "Duplicating not yet Implemented " << std::endl;
				// TODO: Implement duplication
			}

			ImGui::EndPopup();
		}
	}

	static void resetAddChildForm() {
		s_state.childNameBuffer[0] = '\0';
		s_state.selectedType = EntityType::Empty;
		s_state.isNameValid = true;
		s_state.errorMessage[0] = '\0';
		s_state.selectedRagdoll.clear();
		s_state.childPosition = s_state.childPositionDefault;
		s_state.childRotation = glm::vec3(0.f);
		s_state.childScale = glm::vec3(1.f);

		s_state.selectedUpdatefunc = Scripts::empty;
		s_state.selectedUpdatefuncName.clear();

	}

	static Transform buildChildTransform() {
		Transform transform;
		transform.position = s_state.childPosition;
		transform.rotation = rotationFromEulerDegrees(s_state.childRotation);
		transform.scale = s_state.childScale;
		return transform;
	}

	static void drawAddChildPopup(flecs::world& ecs) {
		// Open popup if flagged
		if (s_state.showAddChildPopup) {
			ImGui::OpenPopup("Add Child Entity");
			s_state.showAddChildPopup = false;
			resetAddChildForm();
		}


		// Render popup modal
		if (ImGui::BeginPopupModal("Add Child Entity", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {

			//If any elements that the create button relies on return false it will be disabled.
			bool disableCreateButton = false;

			float windowWidth = ImGui::GetWindowSize().x;
			const char* entTypeTxt = "Entity Type:";
			float textWidth = ImGui::CalcTextSize(entTypeTxt).x;
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

			ImGui::Text("%s", entTypeTxt);

			const float comboWidth = 500.0f;
			ImGui::SetCursorPosX((windowWidth - comboWidth) * 0.5f);
			ImGui::SetNextItemWidth(comboWidth);

			// Get the Ent type name safely
			//std::string_view currentName = magic_enum::enum_name(s_state.selectedType);

			const char* preview = magic_enum::enum_name(s_state.selectedType).data();


			// Dropdown for entity Type
			if (ImGui::BeginCombo("##Entity Type", preview)) {
				for (auto entType : magic_enum::enum_values<EntityType>()) {

					bool isSelected = (s_state.selectedType == entType);
					const char* label = magic_enum::enum_name(entType).data();

					if (ImGui::Selectable(label, isSelected)) {
						s_state.selectedType = entType;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			// if no entity type is selected disable the create button
			if (s_state.selectedType == EntityType::Empty) {
				disableCreateButton = true;
			}


			ImGui::Spacing();

			const char* transformTxt = "Transform:";
			textWidth = ImGui::CalcTextSize(transformTxt).x;
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::Text("%s", transformTxt);

			DragFloat3XYZ("Position", s_state.childPosition);
			DragFloat3XYZ("Rotation", s_state.childRotation);
			DragFloat3XYZ("Scale", s_state.childScale, 0.01f, "%.3f");

			if (s_state.childScale.x <= 0.f || s_state.childScale.y <= 0.f || s_state.childScale.z <= 0.f) {
				disableCreateButton = true;
			}

			ImGui::Spacing();

			// If something is not valid then disable button
			if (!drawEntSpecificOptions(ecs)) {
				disableCreateButton = true;
			}
			//Put Ent type specific options here.

			ImGui::Spacing();
			const char* entNameTxt = "Enter entity name:";
			textWidth = ImGui::CalcTextSize(entNameTxt).x;
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::Text("%s", entNameTxt);

			if (strlen(s_state.childNameBuffer) == 0) {
				disableCreateButton = true;
			}

			if (!s_state.isNameValid) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red text
				ImGui::TextWrapped("%s", s_state.errorMessage);
				ImGui::PopStyleColor();
			}

			ImGui::SetCursorPosX((windowWidth - 300.0f) * 0.5f);
			ImGui::SetNextItemWidth(300.0f);
			bool enterPressed = ImGui::InputText("##childname", s_state.childNameBuffer,
				IM_ARRAYSIZE(s_state.childNameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue);

	

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			float buttonWidth = 140.0f;
			float spacing = ImGui::GetStyle().ItemSpacing.x;
			float totalWidth = buttonWidth * 2 + spacing;
			float availWidth = ImGui::GetContentRegionAvail().x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - totalWidth) * 0.5f);

			ImGui::BeginDisabled(disableCreateButton);
			if (ImGui::Button("Create", ImVec2(buttonWidth, 0)) || enterPressed) {
			
				if (!EntityFactory::validateName(ecs, s_state.contextEntity, s_state.childNameBuffer)) {
					s_state.isNameValid = false;
					snprintf(s_state.errorMessage, sizeof(s_state.errorMessage), "Entity name '%s' is already taken", s_state.childNameBuffer);
				}
				else {
					s_state.isNameValid = true;

					std::cout << "Creating " << magic_enum::enum_name(s_state.selectedType)
						<< " entity '" << s_state.childNameBuffer
						<< "' under " << s_state.contextEntity.name().c_str() << std::endl;

					switch (s_state.selectedType) {
					case EntityType::Empty:

						SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Empty not yet implemented");
						break;
					case EntityType::Player:

						//TODO move creation to under Game
						createPlayerChild(ecs);
						break;
					case EntityType::Actor:

						createActorChild(ecs);
						break;
					case EntityType::BoxCar:
						createBoxCarChild(ecs);
						break;
					case EntityType::Humanoid:

						createHumanoidChild(ecs);
						break;
					case EntityType::Ragdoll:

						createRagdollChild(ecs);
						break;

					case EntityType::JoltRagdollExample:

						createTOFRagdollChild(ecs);
						break;

					case EntityType::RobotArm:
						createRobotArmChild(ecs);
						break;
					case EntityType::Snake:
						createSnakeChild(ecs);
						break;
					case EntityType::Capsule:

						createCapsuleChild(ecs);
						break;
					case EntityType::Grid:

						createGridChild(ecs);
						break;
					case EntityType::Mountain:

						createMountainChild(ecs);
						break;
					case EntityType::Sphere:
						createSphereChild(ecs);
						break;
					case EntityType::Cylinder:
						createCylinderChild(ecs);
						break;
					case EntityType::Cube:
						createCubeChild(ecs);
						break;
					case EntityType::Light:
						createDirLightChild(ecs);
						break;
					case EntityType::Camera:
						SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, " Adding Camera not yet implemented");
						break;
					}

					resetAddChildForm();
					s_state.contextEntity = flecs::entity();
					ImGui::CloseCurrentPopup();

				}
			
			}
			ImGui::EndDisabled();

			ImGui::SameLine();


			//Exiting the editor does not clear the buffer 
			// Cancel button (or ESC key)
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				resetAddChildForm();
				s_state.contextEntity = flecs::entity();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	static void createSceneChild(flecs::world& ecs) {




	}

	static void createPlayerChild(flecs::world& ecs) {
		EntityFactory::createPlayerEntity(ecs, s_state.contextEntity, buildChildTransform(), "pipelineUnlit");
	}

	static void createCapsuleChild(flecs::world& ecs) {
		EntityFactory::createCapsuleEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createCubeChild(flecs::world& ecs) {
		EntityFactory::createCubeEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createSphereChild(flecs::world& ecs) {
		EntityFactory::createSphereEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createCylinderChild(flecs::world& ecs) {
		EntityFactory::createCylinderEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createBoxCarChild(flecs::world& ecs) {
		EntityFactory::createBoxCarEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createActorChild(flecs::world& ecs) {
		Transform actorTransform = buildChildTransform();

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;
		EntityFactory::createActorEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, actorTransform, settings, Scripts::enemyUpdate);

	}

	static void createGridChild(flecs::world& ecs) {
		EntityFactory::createGridEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform(), 256);
	}

	//static void createStaticMeshChild(flecs::world& ecs) {
	//	Transform mtnTransform;
	//	mtnTransform.position = glm::vec3(0.0f, -40.0f, 0.0f);
	//	mtnTransform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	//	EntityFactory::createStaticMeshEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, mtnTransform, 12180758562205882676);
	//}

	static void createMountainChild(flecs::world& ecs) {
		EntityFactory::createMTNEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform());
	}

	static void createHumanoidChild(flecs::world& ecs) {
		EntityFactory::createHumanRagdollEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform(), Scripts::empty);
	}

	static void createRagdollChild(flecs::world& ecs) {
		EntityFactory::createRagdollEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform(), s_state.selectedRagdoll, Scripts::ragdollUpdate);
	}

	static void createTOFRagdollChild(flecs::world& ecs) {
		EntityFactory::createHumanTOFRagdollEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, buildChildTransform(), s_state.selectedUpdatefunc);
	}

	static void createRobotArmChild(flecs::world& ecs) {
		EntityFactory::createRobotArmEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "capsule4", buildChildTransform(), Scripts::armUpdate, "pipelineUnlit");
	}

	static void createSnakeChild(flecs::world& ecs) {
		EntityFactory::createSnakeEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "Capsule4", buildChildTransform(), Scripts::SnakeUpdate, "pipelineUnlit");
	}

	static void createDirLightChild(flecs::world& ecs) {
		DirectionalLight directionalLight;
		directionalLight.direction = quatToDirection(rotationFromEulerDegrees(s_state.childRotation));
		EntityFactory::createDirectionalLightEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, directionalLight);
	}

	static bool drawEntSpecificOptions(flecs::world& ecs) {

		bool isValid = true;

		switch (s_state.selectedType) {

		case EntityType::Ragdoll:

			//TODO move creation to under Game
			isValid = drawRagdollEntOptions(ecs);
			break;

		case EntityType::JoltRagdollExample:

			//TODO move creation to under Game
			isValid = drawRagdollUpdateOptions(ecs);
			break;

		case EntityType::Light:

			break;
		}


		return isValid;
	}

	//If a ragdoll is selected from the dropdown return true,
	static bool drawRagdollEntOptions(flecs::world& ecs) {

		//TODO cache the ragdoll list maybe, if we cache it then we have to have a mechanism that checks for updates.
		AssetLibRef ref = ecs.get<AssetLibRef>();
		std::map<std::string, std::string>& ragdollList = ref.assetLib->ragdolls;

		auto it = ragdollList.find(s_state.selectedRagdoll);
		const char* selectedName = (it != ragdollList.end()) ? it->first.c_str() : " ";
		

		if (ImGui::BeginCombo("Ragdoll", selectedName)) {
			for (const auto& [name, filePath] : ragdollList) {
				bool isSelected = (s_state.selectedRagdoll == name);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					s_state.selectedRagdoll = name;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		//If something is selected then return true
		if (!s_state.selectedRagdoll.empty()) {

			return true;
		}

		return false;
	}

	//If a ragdoll is selected from the dropdown return true,
	static bool drawRagdollUpdateOptions(flecs::world& ecs) {

		
		auto it = updateFunctions.find(s_state.selectedUpdatefuncName);
		const char* selectedName = (it != updateFunctions.end()) ? it->first.c_str() : " ";


		if (ImGui::BeginCombo("RagdollUpdateFunctions", selectedName)) {
			for (const auto& [name, function] : updateFunctions) {
				bool isSelected = (s_state.selectedUpdatefuncName == name);
				if (ImGui::Selectable(name.c_str(), isSelected)) {

					s_state.selectedUpdatefuncName = name;
					s_state.selectedUpdatefunc = function;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		//If something is selected then return true
		if (!s_state.selectedUpdatefuncName.empty()) {

			return true;
		}

		return false;
	}

	static void LoadScene(flecs::entity sceneEntity) {
		// TODO Implement scene loading
	}

	static void OpenPropertiesPanel(flecs::entity objectEntity) {
		// TODO Implement properties panel
	}


	static bool DragFloatColored(const char* id, float* v, float speed,
		const char* fmt, ImVec4 color)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(id);
		ImGui::PopStyleColor();

		ImGui::SameLine(0, 2);

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.16f, 0.16f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.18f, 0.28f, 1.f));

		// No SetNextItemWidth here — caller sets it before invoking us
		bool changed = ImGui::DragFloat(id, v, speed, 0.f, 0.f, fmt);

		ImGui::PopStyleColor(3);
		return changed;
	}

	// XYZ drag widget — call this wherever you want the row
	// Returns true if any component changed.
		static bool DragFloat3XYZ(const char* label, glm::vec3& v, float speed = 0.01f,
			const char* fmt = "%.3f")
		{
			bool changed = false;
			ImGui::PushID(label);

			const float labelColumnWidth = ImGui::CalcTextSize("Rotation").x + ImGui::GetStyle().ItemInnerSpacing.x * 2.f;
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label);
			ImGui::SameLine(labelColumnWidth);

			const float axisLabelWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().ItemInnerSpacing.x;
			float total_width = ImGui::GetContentRegionAvail().x;
			float spacing = ImGui::GetStyle().ItemSpacing.x;
			float cell_width = (total_width - spacing * 2 - axisLabelWidth * 3) / 3.f;

			// X — red
			ImGui::PushID(0);
			ImGui::SetNextItemWidth(cell_width);
			changed |= DragFloatColored("X", &v[0], speed, fmt, ImVec4(0.80f, 0.26f, 0.26f, 1.f));
			ImGui::PopID();

			ImGui::SameLine(0, spacing);

			// Y — green
			ImGui::PushID(1);
			ImGui::SetNextItemWidth(cell_width);
			changed |= DragFloatColored("Y", &v[1], speed, fmt, ImVec4(0.27f, 0.67f, 0.27f, 1.f));
			ImGui::PopID();

			ImGui::SameLine(0, spacing);

			// Z — blue
			ImGui::PushID(2);
			ImGui::SetNextItemWidth(cell_width);
			changed |= DragFloatColored("Z", &v[2], speed, fmt, ImVec4(0.27f, 0.47f, 0.80f, 1.f));
			ImGui::PopID();

			ImGui::PopID();
			return changed;
		}
};

// Initialize static state
SceneTree::State SceneTree::s_state;

