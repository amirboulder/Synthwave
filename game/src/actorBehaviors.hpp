#pragma once

#include "../../core/src/ecs/components.hpp"

enum class Direction { forward, backward };

namespace Scripts {

	void actor1Update(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::Character* joltCharacter = self.get<JoltCharacter>().characterPtr;

		// 1. PostSimulation update must happen first
		joltCharacter->PostSimulation(0.1f);
		JPH::Vec3 actorPos = joltCharacter->GetPosition();

		const Player& player = ecs.get<PlayerRef>().value.get<Player>();
		JPH::Vec3 playerPos = player.position;

		// 2. Establish ray origin (eye level)
		float eyeHeight = 1.3f;
		JPH::Vec3 rayOrigin = actorPos + JPH::Vec3(0.0f, eyeHeight, 0.0f);

		// 3. Get forward direction and scale it by the maximum visibility range
		JPH::Vec3 actorForward = joltCharacter->GetRotation().RotateAxisZ();
		float maxVisibilityRange = 10.0f;

		//unnormalized displacement vector
		JPH::Vec3 direction = actorForward * maxVisibilityRange;

		// 4. Perform the raycast
		JPH::RRayCast ray{ rayOrigin, direction };
		JPH::RayCastResult hit;
		JPH::IgnoreSingleBodyFilter bodyFilter(joltCharacter->GetBodyID());

		ExcludeObjectLayerFilter excludeFilter(Layers::Sensors);

		bool didHit = physicsSystem.GetNarrowPhaseQuery()
			.CastRay(ray, hit, {}, excludeFilter, bodyFilter);


		if (hit.mBodyID == player.innerBodyID) {
			self.set<ActorDebugInfo>({ true });
		}
		else {
			self.set<ActorDebugInfo>({ false });

		}

#ifdef JPH_DEBUG_RENDERER
		if (didHit) {
			JPH::RVec3 hitPosition = rayOrigin + hit.mFraction * direction;
			JPH::DebugRenderer::sInstance->DrawLine(rayOrigin, hitPosition, JPH::Color::sGreen);
			JPH::DebugRenderer::sInstance->DrawMarker(hitPosition, JPH::Color::sYellow, 0.1f);
		}
		else {
			JPH::DebugRenderer::sInstance->DrawLine(rayOrigin, rayOrigin + direction, JPH::Color::sRed);
		}
#endif
	}

	//TODO turn this into a system so we can query some info just once, because all actors will share this system.
	void enemyUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::Character* joltCharacter = self.get<JoltCharacter>().characterPtr;

		joltCharacter->PostSimulation(0.1f); // PostSimulation update must happen first

		//get all the actor info
		JPH::Vec3 actorPos = joltCharacter->GetPosition();
		JPH::Quat actorRot = joltCharacter->GetRotation();
		float actorEyeHeight = 1.3f;
		JPH::Vec3 actorEyePos = actorPos + JPH::Vec3(0.0f, actorEyeHeight, 0.0f);
		JPH::Vec3 actorForward = actorRot.RotateAxisZ();

		//get all the player info
		const Player& player = ecs.get<PlayerRef>().value.get<Player>();
		JPH::Vec3 playerPos = player.position;
		//float playerEyeHeight = 1.3f;
		//JPH::Vec3 playerEyePos = actorPos + JPH::Vec3(0.0f, playerEyeHeight, 0.0f);

		JPH::Vec3 toPlayer = playerPos - actorEyePos;

		float distanceToPlayer = toPlayer.Length();
		float maxVisibilityRange = 50.0f;

		bool playerVisible = false;

		//runs on function exit
		//We want this code to run but don't want to repeat ourselves
		scope_exit sendVisibilityInfo([&]() {
			self.set<ActorDebugInfo>({ playerVisible });
		});

		// Test 1. Distance
		if (distanceToPlayer > maxVisibilityRange)
			return;

		// Test 2. FOV check (Is the player within a 90-degree cone forward?)
		JPH::Vec3 dirToPlayer = toPlayer / distanceToPlayer; // Normalized
		float dotProduct = actorForward.Dot(dirToPlayer);

		// cos(45 degrees) is ~0.707. Higher dot product means closer to center of view cone.
		if (dotProduct < 0.707f) //TODO parametrized this 
			return;

		// unnormalized displacement vector 
		JPH::Vec3 direction = dirToPlayer * maxVisibilityRange;

		// Perform the raycast
		//We are casting one ray should we cast more maybe 3 ?
		JPH::RRayCast ray{ actorEyePos, direction };
		JPH::RayCastResult hit;
		JPH::IgnoreSingleBodyFilter bodyFilter(joltCharacter->GetBodyID());

		//Filter out sensors
		ExcludeObjectLayerFilter excludeFilter(Layers::Sensors);

		bool didHit = physicsSystem.GetNarrowPhaseQuery()
			.CastRay(ray, hit, {}, excludeFilter, bodyFilter);

		//If casting a ray then visualize it for debugging
#ifdef JPH_DEBUG_RENDERER
		if (didHit) {
			JPH::RVec3 hitPosition = actorEyePos + hit.mFraction * direction;
			JPH::DebugRenderer::sInstance->DrawLine(actorEyePos, hitPosition, JPH::Color::sGreen);
			JPH::DebugRenderer::sInstance->DrawMarker(hitPosition, JPH::Color::sYellow, 0.1f);
		}
		else {
			JPH::DebugRenderer::sInstance->DrawLine(actorEyePos, actorEyePos + direction, JPH::Color::sRed);
		}
#endif

		if (!didHit)
			return;

		if (hit.mBodyID == player.innerBodyID) {
			playerVisible = true;
		}
		else {
			return;
		}

		float deltaTime = ecs.delta_time();

		//Turn towards the player
		float turnSpeed = 1.0f;
		JPH::Vec3 flatDir(dirToPlayer.GetX(), 0.0f, dirToPlayer.GetZ());

		JPH::Quat targetRot = targetRot = dirToQuat(flatDir);

		float t;
		t = 1.0f - expf(-8.0f * deltaTime);
		
		JPH::Quat newRot = actorRot.SLERP(targetRot, t);
		joltCharacter->SetRotation(newRot);
		
		//Move towards the player

		float actorSpeed = 10.0f; //TODO parameterize this
		JPH::Vec3 newPos = dirToPlayer.Normalized() * actorSpeed;
		joltCharacter->SetLinearVelocity(newPos);
	}

	void ragdollUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose* pose = self.get<JoltPose>().posePtr;

		float& animTime = self.get_mut<AnimationTime>().time;

		// Advance animation time
		animTime += ecs.delta_time();

		// Loop animation if needed
		float animDuration = animation->GetDuration();
		if (animTime > animDuration) {
			//howanimTime = fmod(animTime, animDuration);
			return;
		}

		// Position ragdoll
		animation->Sample(animTime, *pose);
		pose->CalculateJointMatrices();



		const JPH::RagdollSettings* ragSettings = ragdoll->GetRagdollSettings();

		//ragdoll->ResetWarmStart();
		//ragdoll->SetGroupID

		ragdoll->SetPose(*pose);

	}

	void ragdollUpdateDMS(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose* pose = self.get<JoltPose>().posePtr;

		float& animTime = self.get_mut<AnimationTime>().time;

		// Advance animation time
		animTime += ecs.delta_time();

		// Loop animation if needed
		float animDuration = animation->GetDuration();
		if (animTime > animDuration) {
			animTime = fmod(animTime, animDuration);
		}

		// Position ragdoll
		animation->Sample(animTime, *pose);
		pose->CalculateJointMatrices();

		//JPH::RagdollSettings ragSettings;
		//motorSettings.mMaxTorqueLimit = 1000.0f; // Increase this

		const JPH::RagdollSettings* ragSettings = ragdoll->GetRagdollSettings();


		ragdoll->DriveToPoseUsingMotors(*pose);
		//ragdoll->DriveToPoseUsingKinematics(*pose,1.0f/60.0f);

	}

	void SnakeUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		// Get snake head (first body)
		BodyID headID = ragdoll->GetBodyID(0);
		Vec3 headPos = bi.GetPosition(headID);
		Quat headRot = bi.GetRotation(headID);

		Vec3 playerPos = ecs.get<PlayerRef>().value.get_mut<Player>().position;


		Vec3 toPlayer = playerPos - headPos;
		float distance = toPlayer.Length();
		Vec3 dirToPlayer = toPlayer / distance;

		//Apply force to head
		float chaseForce = 50000.0f;  // Tune this value
		bi.AddForce(headID, dirToPlayer * chaseForce);



	}




	void armUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		physicsSystem.GetBodyInterface().SetRotation(ragdoll->GetBodyID(0), Quat::sIdentity(), EActivation::Activate);
		TwoBodyConstraint* constraint0 = ragdoll->GetConstraint(1);
		HingeConstraint* hinge = static_cast<HingeConstraint*>(constraint0);

		Direction& direction = ecs.get_mut<Direction>();

		float angle = hinge->GetCurrentAngle();
		float min = hinge->GetLimitsMin();
		float max = hinge->GetLimitsMax();

		float range = abs(max - min);

		for (BodyID id : ragdoll->GetBodyIDs()) {

			if (!physicsSystem.GetBodyInterface().IsActive(id)) {

				cout << WARN "BODY IS NOT ACTIVE" << RESET "\n";
				physicsSystem.GetBodyInterface().ActivateBody(id);
			}
		}

		MotorSettings& motor = hinge->GetMotorSettings();

		EMotorState motorState = hinge->GetMotorState();

		// Check if the hinge is already at the limit
		constexpr float tolerance = DegreesToRadians(1.0f);

		if (direction == Direction::forward) {
			float targetAngle = min;

			// Only set motor if we're not already there
			if (abs(angle - targetAngle) > tolerance) {
				hinge->SetMotorState(EMotorState::Position);
				hinge->SetTargetAngle(targetAngle);
			}
			else {
				// We're at the target, turn off motor to save energy
				//hinge->SetMotorState(EMotorState::Off);
				direction = Direction::backward;
			}
			cout << WARN "Dir::forward" << RESET "\n";
		}
		else {
			float targetAngle = max;

			if (abs(angle - targetAngle) > tolerance) {
				hinge->SetMotorState(EMotorState::Position);
				hinge->SetTargetAngle(targetAngle);
			}
			else {
				//hinge->SetMotorState(EMotorState::Off);
				direction = Direction::forward;

			}
			cout << WARN "Dir::backward" << RESET "\n";
		}


		cout << "Hinge angle : " << RadiansToDegrees(angle) << "\n";
		cout << "Hinge min : " << RadiansToDegrees(min) << "\n";
		cout << "Hinge max : " << RadiansToDegrees(max) << "\n";
		cout << "Motor State  : " << int(motorState) << "\n";
		cout << "  " << "\n";


	}



	//placeholder
	void empty(flecs::world& ecs, flecs::entity self) {

	}

}