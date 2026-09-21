#pragma once

#include "sensorBehaviors.hpp"
#include "actorBehaviors.hpp"
#include "hud.hpp"

//This should be renamed to Something else as its basically connects Game Code to Engine Code.
class Scene {

public:

	flecs::world& ecs;

	flecs::system updateActorsSys;
	flecs::system updatePlayerSys;
	flecs::system callScriptsSys;

	flecs::system drawVirtualCharacterPhysicsBodiesSys;

	Scene(flecs::world & ecs)
		: ecs(ecs)
	{
		registerSystems();

		LogSuccess(LOG_APP,"Scene Initialized");
	}

	void registerSystems() {

		updateActorsSystem();
		callContactScripts();
		drawVirtualCharacterPhysicsBodies();
	}

	void updateActorsSystem() {

		updateActorsSys = ecs.system<ActorBehavior>("ActorsUpdateSys")
			.kind<AIPhase>()
			.each([&](flecs::entity e, ActorBehavior& update) {

			update.actorUpdate(ecs, e);

		});

	}

	void callContactScripts() {

		callScriptsSys = ecs.system<ContactDataList>("callScriptsSys")
			.kind<AIPhase>()
			.with<HasContactScript>(flecs::Wildcard)
			.each([&](flecs::iter& it, size_t i, ContactDataList & contactDataList) {

			ContactFunction& contactFunction = it.field_at<ContactFunction>(1, i);

			std::vector<ContactData>& contacts = contactDataList.contacts;
			for (size_t j = 0; j < contacts.size(); j++) {
				contactFunction(contacts[j]);
			}
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