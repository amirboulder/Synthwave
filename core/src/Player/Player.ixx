module;

#include <string>
#include <format>
#include <algorithm>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif

#include <flecs.h>

export module Player;

import Logger;
import Camera;
import GLM;
import Jolt;
import PhysicsComponents;
import TimeManager;
import Components;
import EventComponents;
import GraphicsComponents;
import InputComponents;
import PlayerComponents;
import EntityCreator;
import Util;

import Phases;
import Registry;

	
export struct PlayerSystems {


	PlayerSystems(flecs::world& ecs) {

		//Create PlayerSystems module which will later be imported by Registry
		// must be first
		ecs.module<PlayerSystems>("PlayerSystems"); 

		flecs::system playerUpdateSys = ecs.system<Player>("PlayerUpdateSys")
			.kind<PlayerPhase>()
			.each([&](flecs::entity e, Player& p) {

			update(ecs,e, p);
		});
	}


	void update(flecs::world& ecs, flecs::entity playerEntity, Player& player) {



		getMovementState(ecs, player);

		flecs::entity cameraEnt = ecs.get<PlayerCamRef>().value;
		Camera& camera = cameraEnt.get_mut<Camera>();

		glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
		glm::vec3 right = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));


		player.input.offsetX;
		player.input.offsetY;

		forward *= player.input.direction.y;
		right *= player.input.direction.x;

		glm::vec3 playerInput = glm::vec3(0);

		playerInput += forward;
		playerInput += right;

		player.movementDirection.SetX(playerInput.x);
		player.movementDirection.SetY(playerInput.y);
		player.movementDirection.SetZ(playerInput.z);

		if (player.input.jump) {
			//attempts jump if player is grounded.
			player.mJumpPressed = true;
		}

		UpdateVelocity(ecs, player);
		UpdateCharacter(ecs, player);
		updatePlayerCam(ecs, player);

		shootBall(ecs, playerEntity, player);
	}

	void getMovementState(flecs::world& ecs, Player& player) {

		if (ecs.get<CameraState>() != CameraState::PLAYER) return;


		const ActionState& forwardState = player.forwardMVMTEnt.get<ActionState>();
		const ActionState& backwardState = player.backwardMVMTEnt.get<ActionState>();
		const ActionState& leftState = player.leftMVMTEnt.get<ActionState>();
		const ActionState& rightState = player.rightMVMTEnt.get<ActionState>();
		const ActionState& jumpState = player.jumpMVMTEnt.get<ActionState>();

		const MouseMovementState& mouseMovement = ecs.get<MouseMovementState>();

		// Reset each frame before accumulating
		player.input.direction = glm::vec2(0);
		player.input.jump = false;

		if (forwardState.occurred) {

			player.input.direction.y += 1;
		}
		if (backwardState.occurred) {

			player.input.direction.y -= 1;
		}
		if (leftState.occurred) {

			player.input.direction.x -= 1;
		}
		if (rightState.occurred) {

			player.input.direction.x += 1;
		}

		//cannot jump again until jump is consumed,prevents bunny hopping.
		if (jumpState.occurred && player.input.jumpConsumed) {
			player.input.jump = true;
			player.input.jumpConsumed = false;
		}
		if (!jumpState.occurred)
			player.input.jumpConsumed = true; // ready to jump again

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(player.input.direction) > 0.0f) {
			player.input.direction = glm::normalize(player.input.direction);
		}

		//TODO parameterize
		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + mouseMovement.deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + mouseMovement.deltaY * smoothingFactor;

		player.input.offsetX = smoothedXOffset;
		player.input.offsetY = smoothedYOffset;

	}

	void UpdateVelocity(flecs::world& ecs, Player& player) {
		JPH::CharacterVirtual::EGroundState groundState = player.mCharacter->GetGroundState();

		if (groundState == JPH::CharacterVirtual::EGroundState::OnGround) {
			// On ground
			player.mVerticalVelocity = JPH::Vec3::sZero();

			// Jump
			if (player.mJumpPressed) {
				if (player.mCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
					player.mVerticalVelocity = JPH::Vec3(0, player.jumpSpeed, 0);
					player.mJumpPressed = false;  // Consume jump input
				}
			}
		}
		else {
			// In air: Apply gravity manually
			player.mVerticalVelocity += player.gravity * player.timeStep;

			// Clamp to terminal velocity
			if (player.mVerticalVelocity.GetY() < player.terminalVelocity) {
				player.mVerticalVelocity.SetY(player.terminalVelocity);
			}
		}
	}


	void UpdateCharacter(flecs::world& ecs, Player& player) {

		// Horizontal movement (player controlled)
		JPH::Vec3 horizontalVelocity = player.movementDirection * player.moveSpeed;
		horizontalVelocity.SetY(0);  // Keep horizontal only

		// Combine with vertical velocity (gravity/jump)
		JPH::Vec3 totalVelocity = horizontalVelocity + player.mVerticalVelocity;

		player.mCharacter->SetLinearVelocity(totalVelocity);

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		//TODO create all of these filters just once
		const JPH::DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = physicsSystem.GetDefaultBroadPhaseLayerFilter(1);
		const JPH::BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;

		const JPH::DefaultObjectLayerFilter default_object_layer_filter = physicsSystem.GetDefaultLayerFilter(Layers::MOVING);
		const JPH::ObjectLayerFilter& object_layer_filter = default_object_layer_filter;

		const JPH::BodyFilter body_filter;
		const JPH::ShapeFilter shapeFilter;


		player.mCharacter->Update(player.timeStep, player.gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *player.temp_allocator);

		//mCharacter->ExtendedUpdate(physicsTickRate, gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *fisiks.temp_allocator);

		player.position = player.mCharacter->GetPosition();
		player.rotation = player.mCharacter->GetRotation();

	}

	void updatePlayerCam(flecs::world& ecs, Player& player) {

		// TODO keep a ref instead
		flecs::entity cameraEnt = ecs.get<PlayerCamRef>().value;

		if (!cameraEnt) {
			return;
		}

		Camera& camera = cameraEnt.get_mut<Camera>();

		player.position = player.mCharacter->GetPosition();

		camera.rotateCamera(player.input.offsetX, player.input.offsetY);

		glm::vec3 characterPosGLM = glm::vec3(player.position.GetX(), player.position.GetY(), player.position.GetZ());
		camera.position = characterPosGLM + glm::vec3(player.cameraOffset.x, player.cameraOffset.y, player.cameraOffset.z);


		//Rotate player's physics body based on the camera Yaw.
		float cameraYaw = -glm::radians(camera.yaw + 90.0f);
		player.rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), cameraYaw);
		player.mCharacter->SetRotation(player.rotation);

		camera.updateVectors();

	}

	void shootBall(flecs::world& ecs,flecs::entity playerEnt, Player& player) {

		const ActionState & interactEvent = player.interactEventEnt.get<ActionState>();

		if (interactEvent.justReleased) {
			Camera& camera = ecs.get<PlayerCamRef>().value.get_mut<Camera>();
			glm::vec3 playerCamDir = camera.front;
			glm::vec3 playerCamPos = camera.position;

			// Camera is inside the capsule; step out past its surface plus the ball's radius.
			const float capsuleDiameter = player.bodyShape->GetInnerRadius() * 2;  
			const float ballRadius = 1.0f;  
			const float margin = 0.1f; //TODO should account for current speed

			float spawnDist = capsuleDiameter + ballRadius + margin;

			std::string ballName = std::format("Ball {}", player.ballCounter);
			player.ballCounter++;

			Transform ballTransform = {
				.position = (playerCamDir * spawnDist) + playerCamPos,
				.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
				.scale = glm::vec3(ballRadius)
			};

			flecs::entity parent = playerEnt.parent();

			float multiplier = 20.0f;

			multiplier = multiplier * interactEvent.heldTime * 3.33;

			multiplier = std::clamp(multiplier, 25.0f, 100.0f);

			LogInfo(LOG_APP, "Shot a ball with velocity %f", multiplier);

			glm::vec3 linearVelocity = playerCamDir * multiplier;
			glm::vec3 angularVelocity = glm::vec3(0);
			EntityType entityType = EntityType::Sphere;

			ecs.get_mut<EntityCreationQueue>()
				.queue.emplace_back(ballName, parent, ballTransform, linearVelocity, angularVelocity, entityType);
		}

	}
};

const RegisterModule<PlayerSystems> registerPlayerSystems;
