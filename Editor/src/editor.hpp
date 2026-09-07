#pragma once


#include "../../core/src/Serialization/serialization.hpp"
#include "../../core/src/ecs/RegisterReflectionData.hpp"

#include "SceneTree.hpp"
#include "RagdollBuilder.hpp"
#include "editorMode.hpp"

class Editor {

	flecs::entity editorToggle;

	flecs::query<> activeGameQuery;

	flecs::entity freeCam;

	flecs::entity editorPhase;

	flecs::system updateUIComponentsSys;

public:

	flecs::world& ecs;

	glm::vec2 direction = glm::vec2(0);
	float offsetX = 0.0f;
	float offsetY = 0.0f;

	flecs::entity forwardMVMTEnt;
	flecs::entity backwardMVMTEnt;
	flecs::entity leftMVMTEnt;
	flecs::entity rightMVMTEnt;

	Editor(flecs::world& ecs)
		: ecs(ecs)
	{
		registerReflectionData(ecs);

		registerFreeCam();

		ecs.component<HighlightedEntRef>().add(flecs::Singleton);
		ecs.set<HighlightedEntRef>({ flecs::entity::null() });

		ecs.component<EditorMode>().add(flecs::Singleton);
		ecs.set<EditorMode>({ EditorMode::None });
	}

	//This is called by StateManager
	void init() {

		registerPrefab();

		registerEditorItems();

		registerQuery();

		registerVisualizations();

		lookupEventEnts();

		editorToggle.disable();

		registerPhase();
		registerSystems();

		LogSuccess(LOG_APP, "Editor Initialized");
	}

	bool lookupEventEnts() {

		//Get the required Event entities.
		forwardMVMTEnt = ecs.lookup("forwardMVMTEnt");
		if (!forwardMVMTEnt) {
			LogError(LOG_APP, "forwardMVMTEnt is null");
			return false;
		}

		backwardMVMTEnt = ecs.lookup("backwardMVMTEnt");
		if (!backwardMVMTEnt) {
			LogError(LOG_APP, "backwardMVMTEnt is null");
			return false;
		}

		leftMVMTEnt = ecs.lookup("leftMVMTEnt");
		if (!leftMVMTEnt) {
			LogError(LOG_APP, "leftMVMTEnt is null");
			return false;
		}

		rightMVMTEnt = ecs.lookup("rightMVMTEnt");
		if (!rightMVMTEnt) {
			LogError(LOG_APP, "rightMVMTEnt is null");
			return false;
		}

		return true;
	}

	// All editor Items are created using the prefab editorComponent this allows us to disable all of them by disabling editorComponent
	void registerPrefab() {

		editorToggle = ecs.prefab("editorComponent");
	}

	void registerQuery() {

		activeGameQuery = ecs.query_builder()
			.with<Game>()
			.term_at(0).self()
			.cascade(flecs::ChildOf)
			.build();
	}

	void registerEditorItems() {

		EntityFactory::createEditorItemEntity(ecs, "SceneTree", editorToggle, SceneTree::SceneTreeDraw);
		EntityFactory::createEditorItemEntity(ecs, "RagdollCreator", editorToggle, RagdollBuilder::draw);
		EntityFactory::createEditorItemEntity(ecs, "EditorModeSelector", editorToggle, EditorModeSelector::draw);
	}

	void disable() {

		editorToggle.disable();

		freeCam.get_mut<CameraMVMTState>().locked = false;

		editorPhase.disable();
	}

	void enable() {

		editorToggle.enable();

		freeCam.get_mut<CameraMVMTState>().locked = true;

		editorPhase.enable();
	}

	// TODO Remove
	void RegisterSampleGame() {

		flecs::entity sampleGame = ecs.entity("Sample Game")
			.add<Game>()
			.add<IsActive>().add(flecs::CanToggle);

		flecs::entity sampleScene1 = ecs.entity("Sample Scene1")
			.add<_Scene>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleGame);

		flecs::entity sampleObject1 = ecs.entity("Sample Object1")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleObject2 = ecs.entity("Sample Object2")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleObject3 = ecs.entity("Sample Object3")
			.add<DynamicEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene1);

		flecs::entity sampleScene2 = ecs.entity("Sample Scene2")
			.add<_Scene>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleGame);

		flecs::entity sampleObject4 = ecs.entity("Sample Object4")
			.add<StaticEnt>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(sampleScene2);
	}

	//Static function so we lookup FreeCam
	static void setEditorCamPos(flecs::world& ecs,glm::vec3 newPos) {

		flecs::entity freeCamEnt =  ecs.lookup("FreeCam");
		freeCamEnt.get_mut<Camera>().position = newPos;

	}

	// All the Gizmos, highlights, Helper geometry , etc 
	void registerVisualizations() const {

		Transform xyzAxisTransform;
		xyzAxisTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
		xyzAxisTransform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		vector<LineVertex> xyzLines;

		xyzLines.reserve(6);

		//X is red
		xyzLines.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		xyzLines.emplace_back(glm::vec3(100.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

		//Y is green
		xyzLines.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		xyzLines.emplace_back(glm::vec3(0.0f, 100.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

		//Z is blue
		xyzLines.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		xyzLines.emplace_back(glm::vec3(0.0f, 0.0f, 100.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

		flecs::entity xyzAxis = ecs.entity("XYZAxis")
			.add<EditorMesh>()
			.set<LineVertices>({xyzLines})
			.add<RenderPipeline>(ecs.lookup("pipelineLine"))
			.set<MeshStandalone>({})
			.set<Transform>(xyzAxisTransform);

		const RenderContext& renderContext = ecs.get<RenderContext>();

		RenderUtil::uploadBufferData(renderContext.device, xyzAxis.get_mut<MeshStandalone>().vertexBuffer, xyzLines.data(),
			xyzLines.size() * sizeof(LineVertex), SDL_GPU_BUFFERUSAGE_VERTEX);

	}

	void registerFreeCam() {

		const RenderConfig& config = ecs.get<RenderConfig>();
		const RenderContext& renderContext = ecs.get<RenderContext>();


		Transform transform;
		transform.position = config.FreeCamPos;

		//TODO Fix the camera editor Mesh generation
		//But Also does editor free cam really need a mesh ?
		//We will still want the mesh for knowing where the players camera is located.
	
		/*MeshStandalone mesh = Camera::createMesh(config.freeCamFov);
		RenderUtil::uploadBufferData(renderContext.device, mesh.vertexBuffer, mesh.vertices.data(),
			mesh.vertices.size() * sizeof(Vertex), SDL_GPU_BUFFERUSAGE_VERTEX);
		RenderUtil::uploadBufferData(renderContext.device, mesh.indexBuffer, mesh.indices.data(),
			mesh.indices.size() * sizeof(unsigned int), SDL_GPU_BUFFERUSAGE_INDEX);*/

		//mesh.indexCount = mesh.indices.size();

		freeCam = ecs.entity("FreeCam")
			.emplace<Camera>(config)
			.set<Transform>(transform)
			//.set<ModelSourceName>({ "camera" })
			//.set<MeshStandalone>({ std::move(mesh) })
			.add<RenderPipeline>(ecs.lookup("pipelineWireframe-non-instanced"))
			.set<CameraMVMTState>({ false });
			//.add<EditorMesh>();
	}

	//Set selected entity based on mouse click position if editor is enabled
	void entitySelected(flecs::entity ent) {

		const EditorState* editorState = ecs.try_get<EditorState>();
		if (!editorState) {
			return;
		}
		if (*editorState != EditorState::Enabled) {
			return;
		}

		ecs.set<HighlightedEntRef>({ ent });

	}

	void registerSystems() {

		//Creation order determines the order in which these systems run within a phase
		updateEditorComponentsSystem();


	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity editorPhaseDependency = ecs.entity("EditorPhaseDependency");

		editorPhase = ecs.entity("EditorPhase")
			.add(flecs::Phase)
			.depends_on(editorPhaseDependency);
	}

	void updateEditorComponentsSystem() {

		
		updateUIComponentsSys = ecs.system("UpdateUIComponentsSys")
			.kind(editorPhase)
			.run([&](flecs::iter& it) {

			updateCamera();

			//TODO update gizmos

			//TODO update xyz lines

		});

	}

	void updateCamera() {

		CameraState camState = ecs.get<CameraState>();

		if (camState != CameraState::FREECAM) {
			return;
		}

		//Camera& camera = freeCam.get_mut<Camera>();
		bool camLocked = freeCam.get<CameraMVMTState>().locked;
		if (camLocked) {
			return;
		}

		const ActionState& forwardState = forwardMVMTEnt.get<ActionState>();
		const ActionState& backwardState = backwardMVMTEnt.get<ActionState>();
		const ActionState& leftState = leftMVMTEnt.get<ActionState>();
		const ActionState& rightState = rightMVMTEnt.get<ActionState>();

		const MouseMovementState& mouseMovement = ecs.get<MouseMovementState>();

		// Reset each frame before accumulating
		direction = glm::vec2(0);

		if (forwardState.occurred) {

			direction.y += 1;
		}
		if (backwardState.occurred) {

			direction.y -= 1;
		}
		if (leftState.occurred) {

			direction.x -= 1;
		}
		if (rightState.occurred) {

			direction.x += 1;
		}

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(direction) > 0.0f) {
			direction = glm::normalize(direction);
		}

		//TODO parameterize
		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + mouseMovement.deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + mouseMovement.deltaY * smoothingFactor;

		offsetX = smoothedXOffset;
		offsetY = smoothedYOffset;

		
		
		Camera& camera = freeCam.get_mut<Camera>();

		camera.rotateCamera(offsetX, offsetY);


		camera.position += camera.front * direction.y * camera.movementSpeed;
		camera.position += camera.right * direction.x * camera.movementSpeed;

		camera.updateVectors();

		Transform& transform = freeCam.get_mut<Transform>();
		
		transform.position = camera.position;
		transform.rotation = camera.getRotationQuat();

	}

};


