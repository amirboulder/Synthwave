module;

#include <vector>

#include <flecs.h>

export module AiSystems;

import Logger;
import Phases;
import Jolt;
import Components;
import PhysicsComponents;


export class AiSystems {

	flecs::world& ecs;

	flecs::system updateActorsSys;
	flecs::system callScriptsSys;

public:

	AiSystems(flecs::world& ecs)
		:ecs(ecs)
	{
		registerSystems();
	}


	void registerSystems() {

		updateActorsSystem();
		callContactScripts();

		LogSuccess(LOG_AI, "AI Systems Initialized");
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
			.each([&](flecs::iter& it, size_t i, ContactDataList& contactDataList) {

			ContactFunction& contactFunction = it.field_at<ContactFunction>(1, i);

			std::vector<ContactData>& contacts = contactDataList.contacts;
			for (size_t j = 0; j < contacts.size(); j++) {
				contactFunction(contacts[j]);
			}
				});
	}

};