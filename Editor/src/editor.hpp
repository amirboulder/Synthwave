#pragma once


#include "../../core/src/EntityFactory.hpp"
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

		editorToggle.disable();

		registerPhase();
		registerSystems();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, GOOD "Editor Initialized" RESET);
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
			.set<MeshInstance>({})
			.set<Transform>(xyzAxisTransform);

		const RenderContext& renderContext = ecs.get<RenderContext>();

		RenderUtil::uploadBufferData(renderContext.device, xyzAxis.get_mut<MeshInstance>().vertexBuffer, xyzLines.data(),
			xyzLines.size() * sizeof(LineVertex), SDL_GPU_BUFFERUSAGE_VERTEX);

	}

	void registerFreeCam() {

		const RenderConfig& config = ecs.get<RenderConfig>();

		Transform transform;

		transform.position = config.FreeCamPos;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;

		MeshComponent meshComp = assetLib->requestMeshComponent("camera");

		MeshAsset& asset = assetLib->meshRegistry[meshComp.MeshAssetIndices[0]];

		MeshInstance instance;

		instance.transform = asset.transform;
		instance.vertexBuffer = asset.vertexBuffer;
		instance.indexBuffer = asset.indexBuffer;
		instance.numIndices = asset.numIndices;


		freeCam = ecs.entity("FreeCam")
			.emplace<Camera>(config)
			.set<Transform>(transform)
			.set<ModelSourceName>({ "camera" })
			.set<MeshInstance>({ std::move(instance) })
			.add<RenderPipeline>(ecs.lookup("pipelineWireframe2"))
			.set<CameraMVMTState>({ false })
			.add<EditorMesh>();

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

		//check which camera is active

		const UserInput& input = ecs.get<UserInput>();
		Camera& camera = freeCam.get_mut<Camera>();

		camera.rotateCamera(input.offsetX, input.offsetY);


		camera.position += camera.front * input.direction.y * camera.movementSpeed;
		camera.position += camera.right * input.direction.x * camera.movementSpeed;


		camera.updateVectors();




		Transform& transform = freeCam.get_mut<Transform>();
		
		transform.position = camera.position;
		transform.rotation = camera.getRotationQuat();

	}

};


