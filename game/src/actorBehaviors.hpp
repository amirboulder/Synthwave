#pragma once

#include "../../core/src/ecs/components.hpp"

void actor1Update(flecs::world& ecs, flecs::entity self) {

	JPH::Character* joltCharacter = self.get_mut<JoltCharacter>().characterPtr;

	JPH::Vec3 position = joltCharacter->GetPosition();

	// Perform character post-simulation update this has to happen before anything joltCharacter related is updated
	joltCharacter->PostSimulation(0.1f);

	JPH::Vec3 currentVelocity = joltCharacter->GetLinearVelocity();
	JPH::Vec3 desiredVelocity = JPH::Vec3(0.0f, 5.0f,0.0f);

	if (joltCharacter->IsSupported()) {
		joltCharacter->SetLinearVelocity(desiredVelocity);
	}

	//SDL_Log("Actor1 position  x: %.3f  y: %.3f z: %.3f ", position.GetX(), position.GetY(), position.GetZ());

}