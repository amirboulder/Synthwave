#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

class Actor {

public:

	JPH::Vec3 position = JPH::Vec3(1.0f, -15.0f, 0.0f);
	JPH::Quat rotation = JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f);

	JPH::Character* joltCharacter;

	float moveSpeed = 10.1f;
	//jump is negative because vulkan
	float jumpStrength = -7.5f;


	void createPhysicsBody(PhysicsSystem& physics_system) {

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.0f, 1.0f);
		settings.mMass = 2000.0f;
		settings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		joltCharacter = new JPH::Character(&settings, position, rotation, 69, &physics_system);

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);

	}

	void update()
	{
		// Perform character post-simulation update this has to happen before anything joltCharacter related is updated
		joltCharacter->PostSimulation(0.1f);

		//position = joltCharacter->GetPosition();
		//JPH::Vec3 currentVelocity = joltCharacter->GetLinearVelocity();
		//bool isGrounded = joltCharacter->IsSupported();

	}

};