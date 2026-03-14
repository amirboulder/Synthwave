#pragma once


//Used for Scene dropdown list
//NOTE: EntityType must have a corresponding name here to show up in the dropdown
static const std::map<EntityType, std::string> sceneEntityNames = {
	{EntityType::Player, "Player"},
	{EntityType::Actor, "Actor"},
	{EntityType::Humanoid, "Humanoid"},
	{EntityType::Ragdoll, "Ragdoll"},
	{EntityType::RobotArm, "RobotArm"},
	{EntityType::Snake, "Snake"},
	{EntityType::Capsule, "Capsule"},
	{EntityType::Grid, "Grid"},
	{EntityType::StaticMesh, "StaticMesh"},
	{EntityType::Sphere, "Sphere"},
	{EntityType::Cube, "Cube"},
	{EntityType::Car, "Car"},
	//{EntityType::Light, "Light"},
	//{EntityType::Camera, "Camera"},
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
			ImGui::Text("Selected: %s", s_state.selectedEntity.name().c_str());
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

	static void drawAddChildPopup(flecs::world& ecs) {
		// Open popup if flagged
		if (s_state.showAddChildPopup) {
			ImGui::OpenPopup("Add Child Entity");
			s_state.showAddChildPopup = false;
			s_state.selectedType = EntityType::Empty;  // Reset to default
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

			ImGui::SetNextItemWidth(500.0f);

			// Get the Ent type name safely
			auto it = sceneEntityNames.find(s_state.selectedType);
			const char* currentName = (it != sceneEntityNames.end()) ? it->second.c_str() : " ";

			// Dropdown for entity Type
			if (ImGui::BeginCombo("##entitytype", currentName)) {
				for (const auto& [entType, name] : sceneEntityNames) {
					bool isSelected = (s_state.selectedType == entType);
					if (ImGui::Selectable(name.c_str(), isSelected)) {
						s_state.selectedType = entType;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			// if no entity type is selected disable the create button
			if (!sceneEntityNames.contains(s_state.selectedType)) {
				disableCreateButton = true;
			}


			//Put common options like pos, rot, scale here
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

					std::cout << "Creating " << sceneEntityNames.at(s_state.selectedType)
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
					case EntityType::Car:
						createCarChild(ecs);
						break;
					case EntityType::Humanoid:

						createHumanoidChild(ecs);
						break;
					case EntityType::Ragdoll:

						createRagdollChild(ecs);
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
					case EntityType::StaticMesh:

						createStaticMeshChild(ecs);
						break;
					case EntityType::Sphere:
						SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Adding Sphere not yet implemented");
						break;
					case EntityType::Cube:
						createCubeChild(ecs);
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
			ImGui::EndDisabled();

			ImGui::SameLine();


			//Exiting the editor does not clear the buffer 
			// Cancel button (or ESC key)
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				s_state.childNameBuffer[0] = '\0';
				s_state.contextEntity = flecs::entity();
				s_state.selectedType = EntityType::Empty;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	static void createSceneChild(flecs::world& ecs) {




	}

	static void createPlayerChild(flecs::world& ecs) {

		//Transforms does nothing yet
		Transform playerTransform;
		playerTransform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		playerTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		playerTransform.scale = glm::vec3(1.0f);

		EntityFactory::createPlayerEntity(ecs, s_state.contextEntity, playerTransform, "pipelineUnlit");

	}


	static void createCapsuleChild(flecs::world& ecs) {

		Transform capsule1Transform;
		capsule1Transform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		capsule1Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule1Transform.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "capsule4", capsule1Transform, "pipelineUnlit");

	}

	static void createCubeChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		transform.scale = glm::vec3(1.0f);
		EntityFactory::createCubeEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "Cube", transform, "pipelineUnlit");

	}

	static void createCarChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createCarEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "Car", transform, "pipelineUnlit");

	}

	static void createActorChild(flecs::world& ecs) {

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
		EntityFactory::createActorEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "enemy1", actorTransform, settings, actor1Update, "pipelineUnlit");

	}

	static void createGridChild(flecs::world& ecs) {

		Transform gridTransform;
		gridTransform.position = glm::vec3(12.0f, 0.0f, 0.0f);
		gridTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		EntityFactory::createGridEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, gridTransform, "pipelineGrid", 256);

	}

	static void createStaticMeshChild(flecs::world& ecs) {

		Transform mtnTransform;
		mtnTransform.position = glm::vec3(0.0f, -40.0f, 0.0f);
		mtnTransform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createStaticMeshEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "Mountains", mtnTransform, "pipelineWireframe");

	}

	static void createHumanoidChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(1.0f, 12.0f, 1.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createHumanRagdollEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "capsule4", transform, scripts::empty, "pipelineUnlit");

	}

	static void createRagdollChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createRagdollEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "capsule4", transform, s_state.selectedRagdoll, ragdollUpdate, "pipelineUnlit");

	}

	static void createRobotArmChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createRobotArmEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "capsule4", transform, armUpdate, "pipelineUnlit");

	}

	static void createSnakeChild(flecs::world& ecs) {

		Transform transform;
		transform.position = glm::vec3(1.0f, 7.0f, 1.0f);
		transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createSnakeEntity(ecs, s_state.contextEntity, s_state.childNameBuffer, "Capsule4", transform, SnakeUpdate, "pipelineUnlit");

	}

	static void drawCommonOptions() {

	}

	static bool drawEntSpecificOptions(flecs::world& ecs) {

		//If the
		bool isValid = true;

		switch (s_state.selectedType) {

		case EntityType::Ragdoll:

			//TODO move creation to under Game
			isValid = drawRagdollEntOptions(ecs);
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

	static void LoadScene(flecs::entity sceneEntity) {
		// TODO Implement scene loading
	}

	static void OpenPropertiesPanel(flecs::entity objectEntity) {
		// TODO Implement properties panel
	}


};

// Initialize static state
SceneTree::State SceneTree::s_state;

