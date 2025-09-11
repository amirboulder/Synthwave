#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

#include "renderer/Model.hpp"
#include "renderer/Camera.hpp"

#include "optick.h"

using namespace JPH;


class Player
{
public:

	
	JPH::Vec3 position;
	JPH::Quat rotation;

	JPH::Character* JoltCharacter;

	bool jump = false;
	
	void CreatePlayer(PhysicsSystem& physics_system, JPH::Vec3 position, JPH::Quat rotation, float Radius, float halfHeight) {

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(halfHeight, Radius); 
		settings.mMass = 7000.0f; // Player mass
		settings.mMaxSlopeAngle = DegreesToRadians(45.0f); // Max walkable slope
		settings.mLayer = Layers::MOVING;
		settings.mGravityFactor = 1;

		JoltCharacter = new JPH::Character(&settings, position, rotation, 69, &physics_system);

		JoltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);

	}

	//bool jumpOnCooldown = false;
	//Uint32 jumpCooldownStart = 0;

	void test1() {


		JoltCharacter->PostSimulation(0.1f);
		JPH::Vec3 characterPosition = JoltCharacter->GetPosition();
		JPH::Vec3 currentVelocity = JoltCharacter->GetLinearVelocity();

		cout << characterPosition.GetX() << " " << characterPosition.GetY() << " " << characterPosition.GetZ() << '\n';

	}

	void update(PlayerInput input, Camera& camera) {

		float moveSpeed = 10.1f;
		//jump is negative because vulkan
		float jumpStrength = -7.5f;

		// Perform character post-simulation update this has to happen before anything JoltCharacter related is updated
		JoltCharacter->PostSimulation(0.1f);
		JPH::Vec3 characterPosition = JoltCharacter->GetPosition();
		JPH::Vec3 currentVelocity = JoltCharacter->GetLinearVelocity();
		bool isGrounded = JoltCharacter->IsSupported();

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
			JoltCharacter->SetLinearVelocity(desiredVelocity);
		}
		//if player is not grounded let them fall 
		else
		{
			JPH::Vec3 dir(desiredVelocity.GetX(), currentVelocity.GetY(), desiredVelocity.GetZ());
			JoltCharacter->SetLinearVelocity(dir);
		}

		camera.rotateCamera(input.offsetX, input.offsetY);

		// Update camera position to follow the character (third-person)
		glm::vec3 offset = glm::vec3(0.0f, 1.5f, 0.0f); // Above and behind
		glm::vec3 characterPosGLM = glm::vec3(characterPosition.GetX(), characterPosition.GetY(), characterPosition.GetZ());

		// Rotate offset based on character's yaw
		JPH::Quat rotation = JoltCharacter->GetRotation();
		JPH::Mat44 rotationMatrix = JPH::Mat44::sRotation(rotation);
		JPH::Vec3 offsetJolt(offset.x, offset.y, offset.z);
		JPH::Vec3 rotatedOffset = rotationMatrix.Multiply3x3(offsetJolt);
		camera.position = characterPosGLM + glm::vec3(rotatedOffset.GetX(), rotatedOffset.GetY(), rotatedOffset.GetZ());

		camera.updateVectors();
	}
	

};

