#pragma once

#include "../../core/src/player.hpp"

#include "sensorBehaviors.hpp"
#include "actorBehaviors.hpp"
#include "hud.hpp"

//This should be renamed to Something else as its basically connects Game Code to Engine Code.
class Scene {

public:

	flecs::world& ecs;

	flecs::entity playerPhase;

	flecs::entity aiUpdatePhase;

	flecs::system updateActorsSys;
	flecs::system updatePlayerSys;
	flecs::system callScriptsSys;

	flecs::system drawVirtualCharacterPhysicsBodiesSys;

	Scene(flecs::world & ecs)
		: ecs(ecs)
	{

		registerPhases();
		registerSystems();

		LogSuccess(LOG_APP,"Scene Initialized");
	}

	void registerSystems() {

		updateActorsSystem();
		updatePlayerSystem();
		callContactScripts();
		drawVirtualCharacterPhysicsBodies();
	}

	void updateActorsSystem() {

		updateActorsSys = ecs.system<ActorBehavior>("ActorsUpdateSys")
			.kind(aiUpdatePhase)
			.each([&](flecs::entity e, ActorBehavior& update) {

			update.actorUpdate(ecs, e);

		});

	}

	void callContactScripts() {

		callScriptsSys = ecs.system<ContactDataList>("callScriptsSys")
			.kind(aiUpdatePhase)
			.with<HasContactScript>(flecs::Wildcard)
			.each([&](flecs::iter& it, size_t i, ContactDataList & contactDataList) {

			ContactFunction& contactFunction = it.field_at<ContactFunction>(1, i);

			std::vector<ContactData>& contacts = contactDataList.contacts;
			for (size_t j = 0; j < contacts.size(); j++) {
				contactFunction(contacts[j]);
			}
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

		if (!playerPhaseDependency || !playerPhase)
			LogError(LOG_APP, "playerPhaseDependency and/or playerPhase do not exist");

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

		//TODO remove this and move the draw code to player
		drawVirtualCharacterPhysicsBodiesSys = ecs.system<fisiksDebugRenderer>("DrawVirtualCharacterPhysicsBodiesSys")
			.term_at(0).src<fisiksDebugRenderer>()
			.kind(flecs::PostFrame)
			.each([&](fisiksDebugRenderer& fisiksRenderer) {


			if (!ecs.try_get<PlayerRef>()) return;

			flecs::entity playerEntity = ecs.get<PlayerRef>().value;

			/*if (playerEntity.is_valid()) {
				Ref<CharacterVirtual> mChar = playerEntity.get_mut<Player>().mCharacter;
				RMat44 com = mChar->GetCenterOfMassTransform();
				mChar->GetShape()->Draw(&fisiksRenderer, com, Vec3::sOne(), Color::sWhite, false, true);
			}*/

		});
	}

};