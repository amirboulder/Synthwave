#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/Character.h>

#include "renderer/Model.hpp"
#include "renderer/Camera.hpp"


using namespace JPH;


class Player
{
public:

	JPH::Character* JoltCharacter;

	Player() {};


	void CreatePlayer(PhysicsSystem& physics_system, JPH::Vec3 position, JPH::Quat rotation, float Radius, float halfHeight) {

		// Character settings
		JPH::CharacterSettings settings;
		settings.mShape = new CapsuleShape(halfHeight, Radius); // Capsule: 1m tall, 0.5m radius
		settings.mMass = 70.0f; // Player mass
		settings.mMaxSlopeAngle = DegreesToRadians(45.0f); // Max walkable slope
		//settings.mGravityFactor = 1;
		settings.mLayer = Layers::MOVING;

		JoltCharacter = new JPH::Character(&settings, position, rotation, 69, &physics_system);

		JoltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);


	}

	bool jumpOnCooldown = false;
	Uint32 jumpCooldownStart = 0;

	void update(PlayerInput input, Camera& camera) {
		float moveSpeed = 10.1f;
		float jumpStrength = 7.5f;

		
		
		const Uint32 jumpCooldownDuration = 1000;
		Uint32 now = SDL_GetTicks();

		// Perform character post-simulation update this has to happen before anything JoltCharacter related is updated
		JoltCharacter->PostSimulation(0.1f);


		// Get the character's current position
		JPH::RVec3 characterPosition = JoltCharacter->GetPosition();

		bool isGrounded = JoltCharacter->IsSupported();

		if (isGrounded) {


			// Convert input.direction (glm::vec3) to Jolt's Vec3
			JPH::Vec3 worldDirection(input.direction.x, 0.0f, input.direction.z);

			// Normalize direction (already normalized in input handler, but ensure consistency)
			if (worldDirection.LengthSq() > 0.0f) {
				worldDirection = worldDirection.Normalized();
			}

			JPH::Vec3 desiredVelocity = worldDirection * moveSpeed;

			// Get current velocity
			JPH::Vec3 currentVelocity = JoltCharacter->GetLinearVelocity();

		
			// Reset vertical velocity when grounded (unless jumping)
			if (!input.jump) {
				desiredVelocity.SetY(0.0f); // Clear any climbing-induced velocity
			}
			else {
				// Preserve vertical velocity during jump
				desiredVelocity.SetY(currentVelocity.GetY());
			}


			// Apply the velocity
			JoltCharacter->SetLinearVelocity(desiredVelocity);

			// Handle jumping
			if (input.jump && !jumpOnCooldown) {
				JPH::Vec3 jumpVelocity = currentVelocity + JPH::Vec3(0.0f, jumpStrength, 0.0f);

					
				JoltCharacter->SetLinearVelocity(jumpVelocity);

				// Start cooldown
				jumpCooldownStart = now;
				jumpOnCooldown = true;

			}
			
			// Check if cooldown is over
			if (jumpOnCooldown && (now - jumpCooldownStart >= jumpCooldownDuration)) {
				jumpOnCooldown = false;
			}
			

		}
		// Update camera rotation (yaw and pitch)
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
	
	}
};

