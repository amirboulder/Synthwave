#pragma once 

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

#include "renderer/Model.hpp"
#include "renderer/Camera.hpp"

#include "state/PlayerState.hpp"

#include "optick.h"

using namespace JPH;


class Player
{
public:

	
	JPH::Vec3 position = JPH::Vec3(1.0f, -15.0f, 0.0f);
	JPH::Quat rotation = JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f);

	JPH::Character* joltCharacter;

	float moveSpeed = 10.1f;
	//jump is negative because vulkan
	float jumpStrength = -7.5f;

	// Update camera position to follow the character (third-person)
	glm::vec3 offset = glm::vec3(0.0f, -1.5f, 0.0f); // Above and behind

	Camera& camera;

	float spinSpeed = 1.0f;

	Player(Camera& camera)
		: camera(camera)
	{

	}
	
	void CreatePlayer(PhysicsSystem& physics_system) {

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(2.5f, 1.0f);
		settings.mMass = 7000.0f; 
		settings.mMaxSlopeAngle = DegreesToRadians(45.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		joltCharacter = new JPH::Character(&settings, position, rotation, 69, &physics_system);

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);


		// updating the camera in players contructor because it was created with default values before 
		// player state was loaded, updating it here avoid a camera jump in the first frame as it is rendered before
		// enough time delta time is accumulated for player.update( and other update functions) to be called.

		glm::vec3 characterPosGLM = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		camera.position = characterPosGLM + offset;

		camera.updateVectors();

	}


	void update(PlayerInput input) {

		// Perform character post-simulation update this has to happen before anything joltCharacter related is updated
		joltCharacter->PostSimulation(0.1f);
		position = joltCharacter->GetPosition();
		JPH::Vec3 currentVelocity = joltCharacter->GetLinearVelocity();
		bool isGrounded = joltCharacter->IsSupported();

		// Convert input.direction (glm::vec3) to Jolt's Vec3
		JPH::Vec3 worldDirection(input.direction.x, 0.0f, input.direction.z);
		// Normalize direction (already normalized in input handler, but ensure consistency)
		if (worldDirection.LengthSq() > 0.0f) {
			worldDirection = worldDirection.Normalized();
		}
		
		JPH::Vec3 desiredVelocity = worldDirection * moveSpeed;
		
		if (isGrounded) {
			if (input.jump) {
 				desiredVelocity.SetY(jumpStrength);
			}
			joltCharacter->SetLinearVelocity(desiredVelocity);
		}

		//if player is not grounded let them fall 
		else
		{
			JPH::Vec3 dir(desiredVelocity.GetX(), currentVelocity.GetY(), desiredVelocity.GetZ());
			joltCharacter->SetLinearVelocity(dir);
		}

		camera.rotateCamera(input.offsetX, input.offsetY);

		glm::vec3 characterPosGLM = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		camera.position = characterPosGLM + glm::vec3(offset.x, offset.y, offset.z);

		
		float characterYaw = glm::radians(camera.yaw);

		rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), characterYaw);

		joltCharacter->SetRotation(rotation);


		camera.updateVectors();
	}

};

