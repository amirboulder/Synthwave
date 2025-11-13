#pragma once

#include "../../core/src/AssetLibrary/AssetLibrary.hpp"

#include "../../core/src/ecs/components.hpp"

#include "../../core/src/EntityFactory.hpp"

#include "../../core/src/physics/physicsUtil.hpp"

#include "../../core/src/player.hpp"


#include "sensorBehaviors.hpp"
#include "actorBehaviors.hpp"
#include "hud.hpp"

using std::vector;

class Scene {

public:

	flecs::world& ecs;

	flecs::entity playerEntity;

	flecs::entity playerPhase;

	Fisiks& fisiks;

	Renderer& renderer;

	flecs::entity aiUpdatePhase;

	flecs::system updateActorsSys;
	flecs::system updatePlayerSys;

	flecs::system drawVirtualCharacterPhysicsBodiesSys;

	Scene(flecs::world & ecs,Fisiks& fisiks, Renderer& renderer)
		: ecs(ecs), fisiks(fisiks), renderer(renderer)
	{
		
		//////////////////////////////
		// FreeCam
		const RendererConfig& config = ecs.get<RendererConfig>();

		ecs.entity("FreeCam")
			.emplace<Camera>(config);

		ecs.entity("PlayerCam")
			.emplace<Camera>(config);

		//////////////////////////////
		//Player

		playerEntity = ecs.entity("player").emplace<Player>(ecs, fisiks);

		playerEntity.get_mut<Player>().init(JPH::Vec3(1.0f, 15.0f, 0.0f), JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, 1.0f, playerEntity.id());

		registerPhases();
		registerSystems();

	}

	void registerSystems() {

		updateActorsSystem();
		updatePlayerSystem();
		drawVirtualCharacterPhysicsBodies();
	}

	bool constructLevel() {


		flecs::entity Game1 = ecs.entity("Game1")
			.add<Game>()
			.add<IsActive>().add(flecs::CanToggle);

		flecs::entity Scene1 = ecs.entity("Scene1")
			.add<_Scene>()
			.add<IsActive>().add(flecs::CanToggle)
			.child_of(Game1);

		///////////////////////////
	

		//Entities


		//Actor 1
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
		EntityFactory::createActorEntity(ecs, Scene1, "Actor1", "ActorModel", actorTransform, settings, actor1Update);


		//Grid
		Transform gridTransform;
		gridTransform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		EntityFactory::createGridEntity(ecs,Scene1,"GridChunk1","Grid256", gridTransform, "pipelineGrid", 256, 256);

		//MTN
		Transform mtnTransform;
		mtnTransform.position = glm::vec3(0.0f, -50.0f, 0.0f);
		mtnTransform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		EntityFactory::createStaticMeshEntity(ecs,Scene1,"Mountains","Mountains", mtnTransform, "pipelineMtn");

		////Capsule1
	
		Transform capsule1Transform;
		capsule1Transform.position = glm::vec3(1.0f, 5.0f, 0.0f);
		capsule1Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule1Transform.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, Scene1, "Capsule1", "CapsuleModel", capsule1Transform);


		//Capsule2
		Transform capsule2Transform;
		capsule2Transform.position = glm::vec3(1.0f, 19.0f, 0.0f);
		capsule2Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule2Transform.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, Scene1, "Capsule2", "CapsuleModel", capsule2Transform);


		//Capsule3
		Transform capsule3Transform;
		capsule3Transform.position = glm::vec3(1.0f, 29.0f, 0.0f);
		capsule3Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule3Transform.scale = glm::vec3(1.0f);
		EntityFactory::createCapsuleEntity(ecs, Scene1, "Capsule3", "CapsuleModel", capsule3Transform);

		
		///////////////////////////
		// Sensors

		//Sensor1
		//TODO update factory function
		/*Transform boxSensorTransform;
		boxSensorTransform.position = glm::vec3(1.0f, 3.0f, 0.0f);
		JPH::Vec3 boxSensorSize = JPH::Vec3(15.0f, 15.0f, 15.0f);
		EntityFactory::createBoxSensorEntity(ecs, fisiks, "Sensor1", boxSensorTransform, boxSensorSize,sensor1Behavoir);*/


		/////////////////////////
		
		// Create game HUD
		//FPS counter should not be part of game hud
		//TODO make a part of the renderer
		//EntityFactory::createHUDElementEntity(ecs,"fps", FPSDraw);

		

		/*Transform sponzaTransform;
		sponzaTransform.scale.x = 0.1;
		sponzaTransform.scale.y = 0.1;
		sponzaTransform.scale.z = 0.1;
		EntityFactory::createRenderableEntity(ecs,"Sponza", sponzaSource, sponzaTransform);*/


		return true;


		
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


	void updateActorsSystem() {

		updateActorsSys = ecs.system<ActorBehavior>("ActorsUpdateSys")
			.kind(aiUpdatePhase)
			.each([&](flecs::entity e, ActorBehavior& update) {

			update.actorUpdate(ecs, e);

		});

	}

	void registerPhases() {

		registerAIPhase();
		registerPlayerPhase();
	}

	void registerAIPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity aiPhaseDependency = ecs.entity("AIPhaseDependency");
		aiUpdatePhase = ecs.entity("AIUpdatePhase")
			.add(flecs::Phase)
			.depends_on(aiPhaseDependency);

		// disabled by default so that we don't start simulating until a level is loaded
		aiUpdatePhase.disable();
	}

	void registerPlayerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity playerPhaseDependency = ecs.entity("PlayerPhaseDependency");

		playerPhase = ecs.entity("PlayerPhase")
			.add(flecs::Phase)
			.depends_on(playerPhaseDependency);

		// disabled by default so that we don't start simulating until a level is loaded
		playerPhase.disable();
	}

	// Player Phase is made independent of the scene which allows the player to move around while the world is frozen which is can be interesting for gameplay.
	void updatePlayerSystem() {

		flecs::system playerUpdateSys = ecs.system<Player>("PlayerUpdateSys")
			.kind(playerPhase)
			.each([&](flecs::entity e, Player & p) {

			p.update();

		});

	}


	// Eventually there will be a loop/query in this system which will draw all VirtualCharacterPhysicsBodies
	void drawVirtualCharacterPhysicsBodies() {


		drawVirtualCharacterPhysicsBodiesSys = ecs.system<fisiksDebugRenderer>("DrawVirtualCharacterPhysicsBodiesSys")
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(flecs::PostFrame)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {

			Ref<CharacterVirtual> mChar =  playerEntity.get_mut<Player>().mCharacter;

			RMat44 com = mChar->GetCenterOfMassTransform();

			mChar->GetShape()->Draw(&fisiksRenderer, com, Vec3::sOne(), Color::sWhite, false, true);

		});
	}

};