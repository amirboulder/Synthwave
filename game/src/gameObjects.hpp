#pragma once

#include "../../core/src/EntityFactory.hpp"

#include "../../core/src/physics/physicsUtil.hpp"

#include "../../core/src/state/stateManager.hpp"

#include "sensorBehaviors.hpp"
#include "actorBehaviors.hpp"

using std::vector;

class Scene {

public:

	flecs::world& ecs;

	flecs::entity playerEntity;
	flecs::ref<Player> player;

	StateManager& stateManager;

	Fisiks& fisiks;

	Renderer& renderer;


	flecs::query<ActorBehavior>q1;
	
	Scene(flecs::world & ecs,Fisiks& fisiks, Renderer& renderer, StateManager& stateManager)
		: ecs(ecs), fisiks(fisiks), renderer(renderer), stateManager(stateManager)
	{
		///////////////creating shaders
		
		auto entUnlitPipeline = ecs.entity("pipelineUnlit").set<Pipeline>({});
		Pipeline & unlit = entUnlitPipeline.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/shaders.slang", "shaders/compiled/VertexShader.spv", "shaders/compiled/FragmentShader.spv");
		RenderUtil::loadShaderSPRIV(renderer.context.device, unlit.vertexShader, "shaders/compiled/VertexShader.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderer.context.device, unlit.fragmentShader, "shaders/compiled/FragmentShader.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		unlit.createPipeline(renderer.context, "Blinn-Phong",false);

		auto entGridPipeline = ecs.entity("pipelineGrid").set<Pipeline>({});
		Pipeline & gridPipeline = entGridPipeline.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/gridshader.slang", "shaders/compiled/grid.vert.spv", "shaders/compiled/grid.frag.spv");
		RenderUtil::loadShaderSPRIV(renderer.context.device, gridPipeline.vertexShader, "shaders/compiled/grid.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderer.context.device, gridPipeline.fragmentShader, "shaders/compiled/grid.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);
		gridPipeline.createPipeline(renderer.context, "Grid", false);

		auto e_Mtn = ecs.entity("pipelineMtn").set<Pipeline>({});
		Pipeline& mtnPipeline = e_Mtn.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/wireframe.slang", "shaders/compiled/wireframe.vert.spv", "shaders/compiled/wireframe.frag.spv");
		RenderUtil::loadShaderSPRIV(renderer.context.device, mtnPipeline.vertexShader, "shaders/compiled/wireframe.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX,0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderer.context.device, mtnPipeline.fragmentShader, "shaders/compiled/wireframe.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT,0, 0, 0, 0);
		mtnPipeline.createPipeline(renderer.context, "Mtn", false);


		//SETTING DEFAULT PIPELINE
		//TODO MOVE THIS 
		ecs.entity("RenderState")
			.set<RenderState>({ entUnlitPipeline });

		constructLevel();

	}

	bool constructLevel() {

		//create Model Sources
		//robot
		ModelSource robotSource("assets/robot4Wheels.glb", renderer.context.device);
		//capsule
		ModelSource capsuleSource("assets/capsule4.glb", renderer.context.device);
		//Grid
		ModelSource gridSource(256, 256, renderer.context.device);
		//Mountain
		ModelSource mtnSource("assets/mtn2.obj", renderer.context.device, true);
		//Mountain
		ModelSource ActorSource("assets/enemy1.glb", renderer.context.device);

		//sponza
		//ModelSource sponzaSource("assets/Sponza/sponza.obj", renderer.context.device);

		//////////////////////////////
		//Player
		playerEntity = ecs.entity("player").emplace<Player>(ecs);
		//playerEntity.add<PlayerInput>();

		player = playerEntity.get_ref<Player>();

		stateManager.load();
		player->createPhysicsBody(fisiks.physics_system, playerEntity.id());

		///////////////////////////
	

		//Entities

		//Actor 1
		Transform actorTransform;
		actorTransform.position = glm::vec3(1.0f, 17.0f, 0.0f);
		actorTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		actorTransform.scale = glm::vec3(1.0f);
		// TODO CHANGE TO ACTOR ENTITY
		//EntityFactory::createCharacterEntity(ecs, fisiks, "Amir1", ActorSource, actorTransform,actor1);

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		EntityFactory::createActorEntity(ecs, fisiks, "Actor1", ActorSource, actorTransform, settings,actor1Update);


		//Grid
		Transform gridTransfrom;
		gridTransfrom.rotation = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);
		EntityFactory::createGridEntity(ecs, fisiks, "gridChunk1", gridSource, gridTransfrom, "pipelineGrid", 256, 256);

		//MTN
		Transform mtnTransform;
		mtnTransform.position = glm::vec3(0.0f, -50.0f, 0.0f);
		EntityFactory::createStaticMeshEntity(ecs, fisiks,"Mountain", mtnSource, mtnTransform, "pipelineMtn");

		////Capsule1
		Transform capsule1Transfrom;
		capsule1Transfrom.position = glm::vec3(1.0f, 5.0f, 0.0f);
		capsule1Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule1Transfrom.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, fisiks, "Capsule1", capsuleSource, capsule1Transfrom);


		//Capsule2
		Transform capsule2Transfrom;
		capsule2Transfrom.position = glm::vec3(1.0f, 19.0f, 0.0f);
		capsule2Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule2Transfrom.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, fisiks, "Capsule2", capsuleSource, capsule2Transfrom);

		//Capsule3
		Transform capsule3Transfrom;
		capsule3Transfrom.position = glm::vec3(1.0f, 29.0f, 0.0f);
		capsule3Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule3Transfrom.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, fisiks, "Capsule3", capsuleSource, capsule3Transfrom);
		

		///////////////////////////
		// Sensors

		//Sensor1
		Transform boxSensorTransform;
		boxSensorTransform.position = glm::vec3(1.0f, 3.0f, 0.0f);
		JPH::Vec3 boxSensorSize = JPH::Vec3(15.0f, 15.0f, 15.0f);
		EntityFactory::createBoxSensorEntity(ecs, fisiks, "Sensor1", boxSensorTransform, boxSensorSize,sensor1Behavoir);

	
		
		/*Transform sponzaTransform;
		sponzaTransform.scale.x = 0.1;
		sponzaTransform.scale.y = 0.1;
		sponzaTransform.scale.z = 0.1;
		EntityFactory::createRenderableEntity(ecs,"Sponza", sponzaSource, sponzaTransform);*/


		createQuries();

		return true;


		
	}


	void createQuries() {

		q1 = ecs.query_builder<ActorBehavior>()
			.cached()
			.build();


	}

	void LVL1Script(PhysicsSystem& physicsSystem, JPH::Vec3Arg playerPos) {

		/*
		float moveSpeed = 3;
		BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

		for (int i = 0; i < dynamicEnts.physicsComponents.size(); i++) {

			JPH::Vec3 entityPos = bodyInterface.GetPosition(dynamicEnts.physicsComponents[i].bodyID);
			JPH::Quat entityRot = bodyInterface.GetRotation(dynamicEnts.physicsComponents[i].bodyID);
			JPH::Quat UprightRot = JPH::Quat(0, 0, 0, 1);

			JPH::TransformedShape entityShape = bodyInterface.GetTransformedShape(dynamicEnts.physicsComponents[i].bodyID);
			JPH::AABox entityAABOX = bodyInterface.GetTransformedShape(dynamicEnts.physicsComponents[i].bodyID).GetWorldSpaceBounds();
			JPH::Vec3 entityExtent = entityAABOX.GetExtent();
			FUtil::GroundInfo groundInfo = FUtil::CheckGround(physicsSystem, entityPos, entityAABOX, entityExtent, dynamicEnts.physicsComponents[i].bodyID);
			//	if (groundInfo.isGrounded ) {
			JPH::Vec3 direction(playerPos.GetX() - entityPos.GetX(), 0, playerPos.GetZ() - entityPos.GetZ());
			if (direction.LengthSq() > 0.0f) {
				direction = direction.Normalized();
			}
			JPH::Vec3 desiredVelocity = direction * moveSpeed;
			bodyInterface.SetLinearVelocity(dynamicEnts.physicsComponents[i].bodyID, desiredVelocity);
			bodyInterface.SetRotation(dynamicEnts.physicsComponents[i].bodyID, entityRot, JPH::EActivation::Activate);
			//	}
		}
		*/

	}


	void update() {

		player->update();


		q1.each([&](flecs::entity ent, ActorBehavior & update) {

			update.actorUpdate(ecs, ent);

		});

	}

};