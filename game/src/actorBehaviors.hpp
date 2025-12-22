#pragma once

#include "../../core/src/ecs/components.hpp"

void actor1Update(flecs::world& ecs, flecs::entity self) {

	JPH::Character* joltCharacter = self.get<JoltCharacter>().characterPtr;

	JPH::Vec3 position = joltCharacter->GetPosition();

	// Perform character post-simulation update this has to happen before anything joltCharacter related is updated
	joltCharacter->PostSimulation(0.1f);


	/*JPH::Vec3 currentVelocity = joltCharacter->GetLinearVelocity();
	JPH::Vec3 desiredVelocity = JPH::Vec3(0.0f, 5.0f,0.0f);

	if (joltCharacter->IsSupported()) {
		joltCharacter->SetLinearVelocity(desiredVelocity);
	}*/

	//SDL_Log("Actor1 velocity  x: %.3f  y: %.3f z: %.3f ", currentVelocity.GetX(), currentVelocity.GetY(), currentVelocity.GetZ());

}

void ragdollUpdate(flecs::world& ecs, flecs::entity self) {

	JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
	JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
	JPH::SkeletonPose* pose = self.get<JoltPose>().posePtr;

	float & animTime = self.get_mut<AnimationTime>().time;

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


enum class Direction{ forward,backward };

void armUpdate(flecs::world& ecs, flecs::entity self) {

	JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;

	JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

	physicsSystem.GetBodyInterface().SetRotation(ragdoll->GetBodyID(0), Quat::sIdentity(), EActivation::Activate);
	TwoBodyConstraint* constraint0 = ragdoll->GetConstraint(1);
	HingeConstraint* hinge = static_cast<HingeConstraint*>(constraint0);

	Direction & direction = ecs.get_mut<Direction>();

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

void humanoidUpdate(flecs::world& ecs, flecs::entity self) {

	
	JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
	JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
	BodyInterface& bi = physicsSystem.GetBodyInterface();

	// Get snake head (first body)
	BodyID hip = ragdoll->GetBodyID(0);
	Vec3 headPos = bi.GetPosition(hip);
	Quat headRot = bi.GetRotation(hip);


	Vec3 playerPos = ecs.get<PlayerRef>().value.get_mut<Player>().position;

	Vec3 targetPos = Vec3(1.0f, 10.0f, 1.0f);

	Vec3 toTarget = targetPos - headPos;
	float distance = toTarget.Length();
	Vec3 dirTorTaget = toTarget / distance;

	//Apply force to head
	float chaseForce = 50000.0f;  // Tune this value
	bi.AddForce(hip, dirTorTaget * chaseForce);

	// Fix hip
	//physicsSystem.GetBodyInterface().SetPosition(ragdoll->GetBodyID(0),Vec3(1,10,1), EActivation::Activate);

	//Vec3 playerPos = ecs.get<PlayerRef>().value.get_mut<Player>().position;


	/*Vec3 toPlayer = playerPos - headPos;
	float distance = toPlayer.Length();
	Vec3 dirToPlayer = toPlayer / distance;*/

	//Apply force to head
	//float chaseForce = 50000.0f;  // Tune this value
	//bi.AddForce(headID, dirToPlayer * chaseForce);



}