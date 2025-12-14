#pragma once

#include "../../core/src/player.hpp"

#include "sensorBehaviors.hpp"
#include "actorBehaviors.hpp"
#include "hud.hpp"

using std::vector;

class Scene {

public:

	flecs::world& ecs;

	flecs::entity playerPhase;

	flecs::entity aiUpdatePhase;

	flecs::system updateActorsSys;
	flecs::system updatePlayerSys;

	flecs::system drawVirtualCharacterPhysicsBodiesSys;

	Scene(flecs::world & ecs)
		: ecs(ecs)
	{

		registerPhases();
		registerSystems();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Scene Initialized");
	}

	void registerSystems() {

		updateActorsSystem();
		updatePlayerSystem();
		drawVirtualCharacterPhysicsBodies();
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
	// by disabling fisiksDebugRenderer we are effectively disabling this system as it won't be found by the query
	void drawVirtualCharacterPhysicsBodies() {

		drawVirtualCharacterPhysicsBodiesSys = ecs.system<fisiksDebugRenderer>("DrawVirtualCharacterPhysicsBodiesSys")
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(flecs::PostFrame)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {


			if (!ecs.try_get<PlayerRef>()) return;

			flecs::entity playerEntity = ecs.get<PlayerRef>().value;

			if (playerEntity.is_valid()) {
				Ref<CharacterVirtual> mChar = playerEntity.get_mut<Player>().mCharacter;
				RMat44 com = mChar->GetCenterOfMassTransform();
				mChar->GetShape()->Draw(&fisiksRenderer, com, Vec3::sOne(), Color::sWhite, false, true);
			}

		});
	}

};