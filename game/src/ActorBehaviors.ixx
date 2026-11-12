module;

#include <cmath>
#include <algorithm> 

#include <flecs.h>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif

#include <magic_enum\magic_enum.hpp>

export module ActorBehaviors;

import constants;
import Logger;
import Jolt;
import PhysicsComponents;
import GraphicsComponents;
import PhysicsAnimation;
import PhysicsUtil;
import Ragdoll;
import GLM;
import Components;
import PlayerComponents;
import Util;
import Registry;

export namespace Scripts {

	export void actor1Update(flecs::world& ecs, flecs::entity self) {

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

	

	void releasePose(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint) {

		for (int i = 0; i < ragdoll->GetConstraintCount(); ++i)
		{
			//TODO handle other types of constraints
			JPH::EConstraintSubType sub_type = ragdoll->GetConstraint(i)->GetSubType();
			if (sub_type == JPH::EConstraintSubType::SwingTwist)
			{
				JPH::SwingTwistConstraint* st_constraint = static_cast<JPH::SwingTwistConstraint*>(ragdoll->GetConstraint(i));
				st_constraint->SetSwingMotorState(JPH::EMotorState::Off);
				st_constraint->SetTwistMotorState(JPH::EMotorState::Off);
			}
			else {
				LogError(LOG_PHYSICS, "%s constraint is present in ragdoll and currently not handled", magic_enum::enum_name(sub_type).data());
			}
		}
	}

	//Not strictly necessary because DriveToPoseUsingMotors turns on the motors anyways.
	void regainPose(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint) {

		for (int i = 0; i < ragdoll->GetConstraintCount(); ++i)
		{
			//TODO handle oth
			JPH::EConstraintSubType sub_type = ragdoll->GetConstraint(i)->GetSubType();
			if (sub_type == JPH::EConstraintSubType::SwingTwist)
			{
				JPH::SwingTwistConstraint* st_constraint = static_cast<JPH::SwingTwistConstraint*>(hipConstraint);
				st_constraint->SetSwingMotorState(JPH::EMotorState::Position);
				st_constraint->SetTwistMotorState(JPH::EMotorState::Position);
			}
			else {
				LogError(LOG_PHYSICS, "%s constraint is present in ragdoll and currently not handled", magic_enum::enum_name(sub_type).data());
			}
		}
	}


	//TODO if we just want to disabled the constraint then maybe we don't need to cast it to sub_type.
	void disableConstraint(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* constraint) {

		JPH::EConstraintSubType sub_type = constraint->GetSubType();

		if (sub_type != JPH::EConstraintSubType::SixDOF) {
			LogError(LOG_PHYSICS, "Expected SixDOF constraint here but got : %s ", magic_enum::enum_name(sub_type).data());
			return;
		}

		JPH::SixDOFConstraint* st_constraint = static_cast<JPH::SixDOFConstraint*>(constraint);

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = JPH::SixDOFConstraintSettings::EAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX + i);
			st_constraint->SetMotorState(axisRot, JPH::EMotorState::Off);
		}

		st_constraint->SetEnabled(false);

	}

	void enableConstraint(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* constraint) {

		JPH::EConstraintSubType sub_type = constraint->GetSubType();

		if (sub_type != JPH::EConstraintSubType::SixDOF) {
			LogError(LOG_PHYSICS, "Expected SixDOF constraint here but got : %s ", magic_enum::enum_name(sub_type).data());
			return;
		}

		JPH::SixDOFConstraint* st_constraint = static_cast<JPH::SixDOFConstraint*>(constraint);

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = JPH::SixDOFConstraintSettings::EAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX + i);
			st_constraint->SetMotorState(axisRot, JPH::EMotorState::Position);
		}

		st_constraint->SetEnabled(true);

	}


	void driveToPose(JPH::PhysicsSystem& physicsSystem, JPH::Ragdoll* ragdoll, JPH::SixDOFConstraint* hipConstraint, const JPH::Quat& characterRot, const JPH::SkeletalAnimation* animation, JPH::SkeletonPose& pose, float& animTime) {

		float animDuration = animation->GetDuration();

		animTime += timeStep;// Advance animation time

		animation->Sample(animTime, pose);

		JPH::RVec3 root_offset;
		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = JPH::Vec3::sZero();
		ragdoll->GetRootTransform(root_offset, joint.mRotation);

		joint.mRotation = characterRot;

		JPH::BodyID rootID = ragdoll->GetBodyID(0);

		physicsSystem.GetBodyInterface().SetRotation(rootID, characterRot, JPH::EActivation::Activate);

		pose.SetRootOffset(root_offset);

		ragdoll->DriveToPoseUsingMotors(pose); //This will active motors

	}

	void driveToPose2(JPH::PhysicsSystem& physicsSystem,
		JPH::Ragdoll* ragdoll,
		const JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {

		float animDuration = animation->GetDuration();

		animTime += timeStep;// Advance animation time

		animation->Sample(animTime, pose);

		JPH::RVec3 root_offset;
		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = JPH::Vec3::sZero();
		ragdoll->GetRootTransform(root_offset, joint.mRotation);

		JPH::BodyID rootID = ragdoll->GetBodyID(0);

		pose.SetRootOffset(root_offset);

		ragdoll->DriveToPoseUsingMotors(pose); //This will active motors

	}

	bool checkVisibility(
		JPH::PhysicsSystem& physicsSystem,
		const JPH::IgnoreSingleBodyFilterChained& bodyFilter,
		float maxVisibilityRange,
		JPH::Vec3 sourceEyePos,
		JPH::Quat sourceRotation,
		JPH::Vec3 targetPos,
		JPH::BodyID srcBodyID,
		JPH::BodyID targetBodyID) {

		JPH::Vec3 sourceForward = sourceRotation.RotateAxisZ();
		JPH::Vec3 toTarget = targetPos - sourceEyePos;

		float distanceToTarget = toTarget.Length();


		// Test 1. Distance
		if (distanceToTarget > maxVisibilityRange)
			return false;

		// Test 2. FOV check (Is the target within a 90-degree cone forward?)
		JPH::Vec3 dirToTarget = toTarget / distanceToTarget; // Normalized
		float dotProduct = sourceForward.Dot(dirToTarget);

		// cos(45 degrees) is ~0.707. Higher dot product means closer to center of view cone.
		if (dotProduct < 0.707f) //TODO parametrize this 
			return false;

		JPH::Vec3 direction = dirToTarget * maxVisibilityRange; // unnormalized displacement vector 

		// Perform the raycast
		//We are casting one ray should we cast more maybe 3 ?
		JPH::RRayCast ray{ sourceEyePos, direction };
		JPH::RayCastResult hit;


		//Filter out sensors
		ExcludeObjectLayerFilter excludeFilter(Layers::Sensors);

		bool didHit = physicsSystem.GetNarrowPhaseQuery()
			.CastRay(ray, hit, {}, excludeFilter, bodyFilter);

		//If casting a ray then visualize it for debugging
#ifdef JPH_DEBUG_RENDERER
		if (didHit) {
			JPH::RVec3 hitPosition = sourceEyePos + hit.mFraction * direction;
			JPH::DebugRenderer::sInstance->DrawLine(sourceEyePos, hitPosition, JPH::Color::sGreen);
			JPH::DebugRenderer::sInstance->DrawMarker(hitPosition, JPH::Color::sYellow, 0.1f);
		}
		else {
			JPH::DebugRenderer::sInstance->DrawLine(sourceEyePos, sourceEyePos + direction, JPH::Color::sRed);
		}
#endif

		if (!didHit)
			return false;

		if (hit.mBodyID == targetBodyID) {
			return true;
		}

		return false;
	}

	//TODO parameterize this more
	void rotateCharacterTowardsTarget(JPH::Character* joltCharacter, JPH::Quat currentRot, JPH::Vec3 dirToTarget) {

		float deltaTime = 0.016666;//TODO Parameterize
		//Turn towards the player
		float turnSpeed = 1.0f;
		JPH::Vec3 flatDir(dirToTarget.GetX(), 0.0f, dirToTarget.GetZ());

		JPH::Quat targetRot = targetRot = dirToQuat(flatDir);

		float t;
		t = 1.0f - expf(-8.0f * deltaTime);

		JPH::Quat newRot = currentRot.SLERP(targetRot, t);
		joltCharacter->SetRotation(newRot);
	}


	


	void switchAnimation(const JoltAnimationList& joltAnimationList, JoltAnimation& animation, const std::string& newAnimationName) {

		JPH::SkeletalAnimation* newAnimPtr = joltAnimationList.find(newAnimationName);

		if (!newAnimPtr) {
			LogError(LOG_APP, "Unable to switch Animation because Animation name %s is not present in animation list for entity", newAnimationName.c_str());
			return;
		}


		animation.animationPtr = newAnimPtr;
		LogInfo(LOG_APP, "Switched to animation %s ", newAnimationName.c_str());
	}

	void updateSleep() {


	}

	//Let go of constraints
	void OnEnterSleep(JPH::Ragdoll* ragdoll, JPH::SixDOFConstraint* hipConstraint) {

		releasePose(ragdoll, hipConstraint);
		disableConstraint(ragdoll, hipConstraint);
		
	}

	
	void onExitSleep() {

	}

	//Check Visibility
	//IF visible then Transition to chase
	//IF not visible then keep maintaining idle pose
	void updateIdle(
		JPH::PhysicsSystem& physicsSystem,
		flecs::entity self,
		JPH::SixDOFConstraint* hipConstraint,
		const JPH::Quat& characterRot,
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter,
		JPH::Character* joltCharacter,
		const Player& player, JPH::Ragdoll* ragdoll,
		JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {
		
		
		JPH::Vec3 actorPos = joltCharacter->GetPosition();
		JPH::Quat actorRot = joltCharacter->GetRotation();
		float actorEyeHeight = 1.3f;
		JPH::Vec3 actorEyePos = actorPos + JPH::Vec3(0.0f, actorEyeHeight, 0.0f);


		JPH::Vec3 playerPos = player.position;
		JPH::Vec3 toPlayer = playerPos - actorEyePos;

		float distanceToPlayer = toPlayer.Length();
		float maxVisibilityRange = 50.0f; //TODO parameterize
		JPH::Vec3 dirToPlayer = toPlayer / distanceToPlayer; // Normalized

		JPH::IgnoreSingleBodyFilterChained bodyFilter(joltCharacter->GetBodyID(), *ragdollFilter);

		if (checkVisibility(
			physicsSystem,
			bodyFilter,
			maxVisibilityRange,
			actorEyePos,
			actorRot,
			playerPos,
			joltCharacter->GetBodyID(),
			player.innerBodyID))
		{
			self.set<EnemyState>(EnemyState::CHASE);
			return;
		}

		
		driveToPose(physicsSystem, ragdoll, hipConstraint, characterRot, animation, pose, animTime);
	}

	void OnEnterIdle(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint, const JoltAnimationList& joltAnimationList, JoltAnimation& animation) {

		switchAnimation(joltAnimationList, animation, "idle");
		enableConstraint(ragdoll, hipConstraint);
	}

	void onExitIdle() {

	}

	void updateChase(
		JPH::PhysicsSystem& physicsSystem,
		flecs::entity self,
		JPH::SixDOFConstraint* hipConstraint,
		const JPH::Quat& characterRot,
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter,
		JPH::Character* joltCharacter,
		const Player& player, JPH::Ragdoll* ragdoll,
		JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {

		//Check Visibility
		// IF visible then rotateTowards player and chase
		//IF not visible then transition to 'ToLastKnownLocation'
		
		//get all the actor info
		JPH::Vec3 actorPos = joltCharacter->GetPosition();
		JPH::Quat actorRot = joltCharacter->GetRotation();
		float actorEyeHeight = 1.3f;
		JPH::Vec3 actorEyePos = actorPos + JPH::Vec3(0.0f, actorEyeHeight, 0.0f);


		JPH::Vec3 playerPos = player.position;
		JPH::Vec3 toPlayer = playerPos - actorEyePos;

		float distanceToPlayer = toPlayer.Length();
		float maxVisibilityRange = 50.0f; //TODO parameterize
		JPH::Vec3 dirToPlayer = toPlayer / distanceToPlayer; // Normalized

		JPH::IgnoreSingleBodyFilterChained bodyFilter(joltCharacter->GetBodyID(), *ragdollFilter);

		if (!checkVisibility(
			physicsSystem,
			bodyFilter,
			maxVisibilityRange,
			actorEyePos,
			actorRot,
			playerPos,
			joltCharacter->GetBodyID(),
			player.innerBodyID))
		{
			self.set<EnemyState>(EnemyState::IDLE); 
			return;
		}

		rotateCharacterTowardsTarget(joltCharacter, actorRot, dirToPlayer);

		driveToPose(physicsSystem, ragdoll, hipConstraint, characterRot, animation, pose, animTime);

	}

	void OnEnterChase(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint, const JoltAnimationList& joltAnimationList, JoltAnimation& animation) {

		switchAnimation(joltAnimationList, animation, "sprint");
		enableConstraint(ragdoll, hipConstraint);

		
	}

	void onExitChase() {

	}

	void onEnterCorpse(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint, const JoltAnimationList& joltAnimationList, JoltAnimation& animation) {
		releasePose(ragdoll, hipConstraint);
		disableConstraint(ragdoll, hipConstraint);
	}

	void onExitCorpse() {


	}

	void corpseUpdate() {
		
	}

	void onEnterBroken(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint, const JoltAnimationList& joltAnimationList, JoltAnimation& animation) {
		//releasePose(ragdoll, hipConstraint);
		//switchAnimation(joltAnimationList, animation, "sprint");
		disableConstraint(ragdoll, hipConstraint);
	}

	void onExitBroken() {


	}

	void updateBroken(JPH::PhysicsSystem& physicsSystem,
		flecs::entity self,
		JPH::SixDOFConstraint* hipConstraint,
		const JPH::Quat& characterRot,
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter,
		JPH::Character* joltCharacter,
		const Player& player, JPH::Ragdoll* ragdoll,
		JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {

		//get all the actor info
		JPH::Vec3 actorPos = joltCharacter->GetPosition();
		JPH::Quat actorRot = joltCharacter->GetRotation();
		float actorEyeHeight = 1.3f;
		JPH::Vec3 actorEyePos = actorPos + JPH::Vec3(0.0f, actorEyeHeight, 0.0f);

		JPH::Vec3 playerPos = player.position;
		JPH::Vec3 toPlayer = playerPos - actorEyePos;

		float distanceToPlayer = toPlayer.Length();
		float maxVisibilityRange = 50.0f; //TODO parameterize
		JPH::Vec3 dirToPlayer = toPlayer / distanceToPlayer; // Normalized

		JPH::IgnoreSingleBodyFilterChained bodyFilter(joltCharacter->GetBodyID(), *ragdollFilter);

		/*
		if (!checkVisibility(
			physicsSystem,
			bodyFilter,
			maxVisibilityRange,
			actorEyePos,
			actorRot,
			playerPos,
			joltCharacter->GetBodyID(),
			player.innerBodyID))
		{
			self.set<EnemyState>(EnemyState::IDLE);
			return;
		}
		*/

		//rotateCharacterTowardsTarget(joltCharacter, actorRot, dirToPlayer);

		driveToPose2(physicsSystem, ragdoll, animation, pose, animTime);

	}

	void updateRagdollMotor(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Character* joltCharacter = self.get<JoltCharacter>().characterPtr;
		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SixDOFConstraint* hipConstraint = self.get_mut<PhysicsConstraint>().constraint;
		JoltAnimationList joltAnimationList = self.get_mut<JoltAnimationList>();
		JoltAnimation joltAnimation = self.get_mut<JoltAnimation>();
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter = self.get_mut<JoltRagdollFilter>().filter;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		const EnemyState& state = self.get_mut<EnemyState>();
		float& animTime = self.get_mut<AnimationTime>().time;
		const Player& player = ecs.get<PlayerRef>().value.get<Player>();
		const std::vector<ContactData> & contactList = self.get<ContactDataList>().contacts;

		joltCharacter->PostSimulation(0.1f); // PostSimulation update must happen first

		JPH::Quat characterRot = joltCharacter->GetRotation();

		float totalImpulse = 0.0f;
		for (const ContactData& contactData : contactList) {
			totalImpulse += contactData.impulse;
		}

		//Instead of total impulse robot should die when the head is damaged
		//Hitting them in the legs should break them
		if (totalImpulse >= 1800.0f) {
			LogInfo(LOG_APP, "Total Impulse for %s : %f", self.name().c_str(), totalImpulse);
			self.set<EnemyState>(EnemyState::CORPSE);
		}

		switch (state)
		{
			case EnemyState::SLEEP:
			{
				break;
			}
			case EnemyState::IDLE:
			{
				updateIdle(physicsSystem, self, hipConstraint, characterRot, ragdollFilter, joltCharacter, player, ragdoll, animation, pose, animTime);
				break;
			}
			case EnemyState::SEARCH:
			{
				break;
			}
			case EnemyState::CHASE:
			{
				updateChase(physicsSystem, self, hipConstraint, characterRot, ragdollFilter, joltCharacter, player, ragdoll, animation, pose, animTime);
				break;
			}
			case EnemyState::FIGHT:
			{
				break;
			}
			case EnemyState::CORPSE:
			{	
				
				return;
			}
			case EnemyState::BROKEN:
			{
				updateBroken(physicsSystem, self, hipConstraint, characterRot, ragdollFilter, joltCharacter, player, ragdoll, animation, pose, animTime);
				return;
			}
			default:
			{	
				break;
			}

		}

#ifdef JPH_DEBUG_RENDERER
		pose.CalculateJointMatrices();
		pose.Draw({}, JPH::DebugRenderer::sInstance);

		hipConstraint->DrawConstraint(JPH::DebugRenderer::sInstance);
		hipConstraint->DrawConstraintLimits(JPH::DebugRenderer::sInstance);
#endif

	}

	void RegisterRagdollObserver(flecs::world& ecs) {

		ecs.observer<EnemyState, PhysicsConstraint, JoltRagdoll, JoltAnimation, JoltAnimationList>("OnEnterNewEnemyState")
			.term_at(0).event(flecs::OnSet)
			.each([](
				flecs::entity e,
				EnemyState state,
				PhysicsConstraint& physicsConstraint,
				JoltRagdoll& joltRagdoll,
				JoltAnimation& animation,
				const JoltAnimationList& animationList) {

			JPH::Ragdoll* ragdoll = joltRagdoll.ragdollPtr;
			JPH::SixDOFConstraint* hipConstraint = physicsConstraint.constraint;
			
			LogInfo(LOG_APP, "state is %s", magic_enum::enum_name(state).data());

			switch (state)
			{
			case EnemyState::SLEEP:
				OnEnterSleep(ragdoll, hipConstraint);
				break;
			
			case EnemyState::IDLE:

				OnEnterIdle(ragdoll, hipConstraint, animationList, animation);
				break;
			case EnemyState::SEARCH:

				break;

			case EnemyState::CHASE:

				OnEnterChase(ragdoll, hipConstraint, animationList, animation);
				break;

			case EnemyState::FIGHT:

				break;

			case EnemyState::CORPSE:

				onEnterCorpse(ragdoll, hipConstraint, animationList, animation);

				break;
			case EnemyState::DISABLED:

				break;

			case EnemyState::BROKEN:
				onEnterBroken(ragdoll, hipConstraint, animationList, animation);
				break;

			default:

				break;
			}

		});

	}

	const AutoRegister registerRagdollObserver{ &RegisterRagdollObserver };


	void updateRagdollPD(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();
		const JPH::BodyLockInterface& biLock = physicsSystem.GetBodyLockInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter = self.get<JoltRagdollFilter>().filter;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JPH::SkeletonPose>();
		const JPH::Skeleton* skeleton = pose.GetSkeleton();

		const RagdollLocalUps& localUps = self.get<RagdollLocalUps>();

		HumanoidJointIndices jointIndices = self.get<HumanoidJointIndices>();

		const std::vector<ContactData>& contactDataList = self.get<ContactDataList>().contacts;

		std::vector<JPH::Vec3>& supportHull = self.get_mut<SupportHull>().hull;
		std::vector<JPH::Vec3>& contactPoints = self.get_mut<GroundContactPoints>().points;
	
		JPH::Vec3 gravity = physicsSystem.GetGravity();

		float& animTime = self.get_mut<AnimationTime>().time;
		animTime += timeStep;
		animation->Sample(animTime, pose);
		
		JPH::SkeletonPose::JointState& hipJoint = pose.GetJoint(0);

		//Place the skeletons root joint on the ragdolls first body so that we draw the pose in the right place
		JPH::RVec3 root_offset;
		ragdoll->GetRootTransform(root_offset, hipJoint.mRotation);
		hipJoint.mTranslation = JPH::Vec3::sZero(); // strip baked root translation
		pose.SetRootOffset(root_offset);

		JPH::BodyID rootID = ragdoll->GetBodyID(0);

		JPH::Vec3 rootPos = bi.GetPosition(rootID);
		JPH::Quat rootRot = bi.GetRotation(rootID);
		JPH::Vec3 rootLinearVelocity = bi.GetLinearVelocity(rootID);
		//JPH::Vec3 rootAngularVelocity = bi.GetAngularVelocity(rootID);


		//////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////

		GroundInfo groundInfo = Utils::Phys::CheckGround(physicsSystem, rootPos, excludeSensors, *ragdollFilter);

		//Calculate the ragdolls inertia
		RagdollInertia ragdollInertia = ragdoll::computeRagdollInertia(ragdoll, biLock, groundInfo.groundPoint);

	
		JPH::BodyID	footLBodyId = ragdoll->GetBodyID(jointIndices.ankleL);
		JPH::BodyID	footRBodyId = ragdoll->GetBodyID(jointIndices.ankleR);

		JPH::BodyID	lowerLegLBodyId = ragdoll->GetBodyID(jointIndices.lowerLegL);
		JPH::BodyID	lowerLegRBodyId = ragdoll->GetBodyID(jointIndices.lowerLegR);


		JPH::Vec3 groundNormalFootL{};
		JPH::Vec3 groundNormalFootR{};

		uint16_t numGroundContactsFootL{};
		uint16_t numGroundContactsFootR{};

		JPH::Vec3 supportVelocity {};
		size_t numValidContactsCounter { 0 };


		contactPoints.clear();
		for (const ContactData& contactData : contactDataList) {

			if (contactData.selfBodyID != footLBodyId && contactData.selfBodyID != footRBodyId) {
				continue;
			}

			//remove contacts between ragdoll parts, Note parent to child contacts are already ignored
			if (contactData.selfEntity == contactData.otherEntity) {
				continue;
			}

			if (contactData.selfBodyID == footLBodyId) {

				groundNormalFootL += contactData.normal;
				numGroundContactsFootL++;
			}

			if (contactData.selfBodyID == footRBodyId) {

				groundNormalFootR += contactData.normal;
				numGroundContactsFootR++;
			}

			for (int i = 0; i < contactData.contactPointsData.numValidPoints; i++) {

				contactPoints.emplace_back(contactData.contactPointsData.points[i]);

				supportVelocity += contactData.otherLinearVelocity;
				numValidContactsCounter++;
			}
		}

		if (numValidContactsCounter > 0) {
			supportVelocity /= numValidContactsCounter;
		}

		if (numGroundContactsFootL > 0) {
			groundNormalFootL /= numGroundContactsFootL;
		}

		if (numGroundContactsFootR > 0) {
			groundNormalFootR /= numGroundContactsFootR;
		}

		
		supportHull = Utils::Phys::buildConvexHull(contactPoints);

		const flecs::world_info_t* info = ecs.get_info();
		int64_t frame = info->frame_count_total;

		if (supportHull.size() == 0) {

			//just hold the animation
			//TODO switch to a falling animation if greater than X distance from the ground.
			ragdoll->DriveToPoseUsingMotors(pose);
			return;
		}

		float stabilityMargin = -FLT_MAX;

		stabilityMargin = Utils::Phys::calculateStabilityMargin(supportHull, ragdollInertia.com);

		//LogInfo(LOG_APP, "Stability Margin : %f", stabilityMargin);

		std::optional<JPH::Vec3> centroidOpt = Utils::Phys::calculateCentroid(supportHull);

		//somewhat redundant because we already checked for supportHull.size() == 0.
		if (!centroidOpt.has_value()) {

			LogInfo(LOG_APP, "somehow centroidOpt has no value ¯\(ツ)/¯");
			return;
		}

		JPH::Vec3 centroid = centroidOpt.value();

		JPH::Vec3 relativeVelocity = ragdollInertia.comVelocity - supportVelocity;
		relativeVelocity.SetY(0.0f);

		
		JPH::Vec3 comOffset = centroid - ragdollInertia.com;
		comOffset.SetY(0.0f);

		
		//Turn the error into a lean angle(PD)
		float positionGain = 1.1f; 
		float velocityGain = positionGain * std::sqrt((ragdollInertia.com.GetY() - centroid.GetY()) / gravity.Length());
		// Ankle authority differs per direction (rad). Starting values, tune from the log.
		const float maxLeanBackward = 0.25f; // body leaning forward -> pressure moves to the toes (long lever)
		const float maxLeanForward = 0.12f;  // body leaning back    -> pressure moves to the heel (short lever)
		const float maxLeanSide = 0.05f;     // pressure moves to the foot's side edge (shortest lever)

		float torsoGain = 0.4f;
		float maxTorsoLean = 0.5f;



		//how much to correct described as a horizontal vector
		// The ragdoll's own horizontal axes. Sideways = left foot -> right foot, flattened.
		JPH::Vec3 footPosL = JPH::Vec3(bi.GetCenterOfMassPosition(footLBodyId));
		JPH::Vec3 footPosR = JPH::Vec3(bi.GetCenterOfMassPosition(footRBodyId));
		JPH::Vec3 sideAxis = footPosR - footPosL;
		sideAxis.SetY(0.0f);
		sideAxis = sideAxis.NormalizedOr(JPH::Vec3::sAxisX());
		JPH::Vec3 fwdAxis = JPH::Vec3::sAxisY().Cross(sideAxis);

		// Uncapped PD output: the direction and amount the CoM should move
		JPH::Vec3 rawCorrection = positionGain * comOffset - velocityGain * relativeVelocity;

		// Split into the ragdoll's axes and cap each direction separately
		float corrFwd = rawCorrection.Dot(fwdAxis);   // > 0: CoM must move forward (body is leaning back)
		float corrSide = rawCorrection.Dot(sideAxis); // > 0: CoM must move toward the right foot

		corrFwd = std::clamp(corrFwd, -maxLeanBackward, maxLeanForward);
		corrSide = std::clamp(corrSide, -maxLeanSide, maxLeanSide);

		// Capped correction, back in world space
		JPH::Vec3 correctionVector = corrFwd * fwdAxis + corrSide * sideAxis;

		// Share of body weight on each foot (lever rule), 0..1
		float stanceWidth = (footPosR - footPosL).Dot(sideAxis);
		float loadR = 0.5f;
		if (stanceWidth > 1e-3f) {
			loadR = std::clamp((ragdollInertia.com - footPosL).Dot(sideAxis) / stanceWidth, 0.0f, 1.0f);
		}
		float loadL = 1.0f - loadR;

		// Full authority while a foot carries at least half the weight, fading to zero as it unloads
		float ankleWeightL = std::min(2.0f * loadL, 1.0f);
		float ankleWeightR = std::min(2.0f * loadR, 1.0f);

		//Turn the correction vector into a hinge
		JPH::Vec3 axisWorld = JPH::Vec3::sAxisY().Cross(correctionVector).NormalizedOr(JPH::Vec3::sZero());

		// correctionVector is already capped per axis; the remainder is what the ankles could not cover
		float ankleAngle = correctionVector.Length();
		float excessLean = std::max(rawCorrection.Length() - ankleAngle, 0.0f);
		float torsoAngle = std::min(excessLean * torsoGain, maxTorsoLean);


		




		if (ankleAngle > 1e-4f)
		{
			if (numGroundContactsFootL > 0) {

				JPH::Quat shinRotL = bi.GetRotation(ragdoll->GetBodyID(jointIndices.lowerLegL));
				JPH::Vec3 axisLocalL = (shinRotL.Conjugated() * axisWorld).Normalized();

				JPH::SkeletonPose::JointState& ankleJointL = pose.GetJoint(jointIndices.ankleL);

				JPH::Quat footLActualRotation = bi.GetRotation(footLBodyId);

				JPH::Quat LowerLegLActualRotation = bi.GetRotation(lowerLegLBodyId);

				JPH::Vec3 footLWorldUp = footLActualRotation * localUps.footL;

				JPH::Vec3 lowerLegLWorldUp = bi.GetRotation(lowerLegLBodyId) * localUps.lowerLegL;

				JPH::Quat footLDesired = JPH::Quat::sFromTo(footLWorldUp, -groundNormalFootL) * footLActualRotation;


				JPH::Quat shinLUpright = JPH::Quat::sFromTo(lowerLegLWorldUp, JPH::Vec3::sAxisY()) * LowerLegLActualRotation;
				JPH::Quat desiredShinL = JPH::Quat::sRotation(axisWorld, ankleAngle * ankleWeightL) * shinLUpright;

				ankleJointL.mRotation = desiredShinL.Conjugated() * footLDesired;
			}

			if (numGroundContactsFootR > 0) {

				JPH::Quat shinRotR = bi.GetRotation(ragdoll->GetBodyID(jointIndices.lowerLegR));
				JPH::Vec3 axisLocalR = (shinRotR.Conjugated() * axisWorld).Normalized();

				JPH::SkeletonPose::JointState& ankleJointR = pose.GetJoint(jointIndices.ankleR);

				JPH::Quat footRActualRotation = bi.GetRotation(footRBodyId);

				JPH::Quat LowerLegRActualRotation = bi.GetRotation(lowerLegRBodyId);

				JPH::Vec3 footRWorldUp = footRActualRotation * localUps.footR;

				JPH::Vec3 lowerLegRWorldUp = bi.GetRotation(lowerLegRBodyId) * localUps.lowerLegR;

				JPH::Quat footRDesired = JPH::Quat::sFromTo(footRWorldUp, -groundNormalFootR) * footRActualRotation;

				JPH::Quat shinRUpright = JPH::Quat::sFromTo(lowerLegRWorldUp, JPH::Vec3::sAxisY()) * LowerLegRActualRotation;

				JPH::Quat desiredShinR = JPH::Quat::sRotation(axisWorld, ankleAngle * ankleWeightR) * shinRUpright;

				ankleJointR.mRotation = desiredShinR.Conjugated() * footRDesired;
			}

		}

		if (torsoAngle > 1e-4f) {
			
			//Commented out on purpose for now We want ankle strategy to be good enough to hold up the ragdoll when no external force is applied
			//JPH::Quat hipRot = bi.GetRotation(ragdoll->GetBodyID(jointIndices.hips));
			//JPH::Vec3 axisHipsLocal = (hipRot.Conjugated() * axisWorld).Normalized();
			

			//JPH::SkeletonPose::JointState& spine0 = pose.GetJoint(jointIndices.spine0);
			//spine0.mRotation = JPH::Quat::sRotation(axisHipsLocal, torsoAngle) * spine0.mRotation;

		}

		
	// --- debug: express the controller's quantities in the ragdoll's own frame ---
	// Sideways axis = left foot -> right foot, flattened. Forward = perpendicular to it.
	// (sideAxis / fwdAxis are computed above, next to the correction.)

		// Actual shin lean (rad), measured along the direction the controller is asking for,
		// so it compares directly against ankleAngle.
		JPH::Vec3 corrDir = correctionVector.NormalizedOr(JPH::Vec3::sZero());
		JPH::Vec3 shinUpL = bi.GetRotation(lowerLegLBodyId) * localUps.lowerLegL;
		JPH::Vec3 shinUpR = bi.GetRotation(lowerLegRBodyId) * localUps.lowerLegR;
		float actualLeanL = std::atan2(shinUpL.Dot(corrDir), shinUpL.GetY());
		float actualLeanR = std::atan2(shinUpR.Dot(corrDir), shinUpR.GetY());

		LogInfo(LOG_APP,
			"pts %zu hull %zu (L%u R%u) | margin %.3f | off f %.3f s %.3f | vel f %.3f s %.3f | corr f %.3f s %.3f | lean cmd %.3f actL %.3f actR %.3f | torso %.3f",
			contactPoints.size(), supportHull.size(),
			(unsigned)numGroundContactsFootL, (unsigned)numGroundContactsFootR,
			stabilityMargin,
			comOffset.Dot(fwdAxis), comOffset.Dot(sideAxis),
			relativeVelocity.Dot(fwdAxis), relativeVelocity.Dot(sideAxis),
			correctionVector.Dot(fwdAxis), correctionVector.Dot(sideAxis),
			ankleAngle, actualLeanL, actualLeanR,
			torsoAngle);
		
		
		if (!ragdoll->IsActive()) {
			LogInfo(LOG_APP, "Ragdoll is sleeping!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			ragdoll->Activate();
		}

		// --- ankle motor strength (experiment) ---
		// Size the ankle springs from the load they carry, not from the foot's inertia.
		const float bodyMass = ragdollInertia.totalMass;                                   // kg
		const float g = gravity.Length();                                           // m/s^2
		const float comHeight = std::max(ragdollInertia.com.GetY() - centroid.GetY(), 0.1f); // m

		const float tippingStiffness = bodyMass * g * comHeight;      // N*m/rad gravity "pulls" with per rad of lean
		const float ankleStiffness = 2.0f * tippingStiffness;       // per ankle -> 4x the threshold in total
		const float ankleDamping = 2.0f * bodyMass * comHeight * std::sqrt(g * comHeight); // per ankle, ~critical
		const float ankleTorqueLimit = 10.0f * tippingStiffness;      // N*m

		const JPH::RagdollSettings* ragdollSettings = ragdoll->GetRagdollSettings();
		const int ankleBodies[] = { jointIndices.ankleL, jointIndices.ankleR };

		for (int ankleBody : ankleBodies)
		{
			int constraintIdx = ragdollSettings->GetConstraintIndexForBodyIndex(ankleBody);
			if (constraintIdx < 0)
				continue;

			JPH::SwingTwistConstraint* ankle =
				static_cast<JPH::SwingTwistConstraint*>(ragdoll->GetConstraint(constraintIdx));

			JPH::MotorSettings* motors[] = { &ankle->GetSwingMotorSettings(), &ankle->GetTwistMotorSettings() };

			for (JPH::MotorSettings* motor : motors)
			{
				motor->mSpringSettings.mMode = JPH::ESpringMode::StiffnessAndDamping;
				motor->mSpringSettings.mStiffness = ankleStiffness;
				motor->mSpringSettings.mDamping = ankleDamping;
				motor->SetTorqueLimit(ankleTorqueLimit);
			}
		}

		

		ragdoll->DriveToPoseUsingMotors(pose);

#ifdef JPH_DEBUG_RENDERER

		pose.CalculateJointMatrices();
		pose.Draw({}, JPH::DebugRenderer::sInstance);

		JPH::DebugRenderer* dr = JPH::DebugRenderer::sInstance;
		const JPH::Vec3 lift(0.0f, 0.03f, 0.0f); // keeps lines from z-fighting with the ground
		// Raw contact points (small)
		for (const JPH::Vec3& p : contactPoints)
			dr->DrawMarker(p + lift, JPH::Color::sGrey, 0.03f);
		// Hull edges: green while CoM is inside, red once it's outside
		JPH::Color hullColor = stabilityMargin > 0.0f ? JPH::Color::sGreen : JPH::Color::sRed;
		for (size_t i = 0; i < supportHull.size(); i++) {
			const JPH::Vec3& a = supportHull[i];
			const JPH::Vec3& b = supportHull[(i + 1) % supportHull.size()];
			dr->DrawLine(a + lift, b + lift, hullColor);
		}
		// Centroid, and the CoM projected straight down to the same height
		JPH::Vec3 comGround = ragdollInertia.com;
		comGround.SetY(centroid.GetY());
		dr->DrawMarker(centroid + lift, JPH::Color::sYellow, 0.1f);
		dr->DrawMarker(comGround + lift, JPH::Color::sCyan, 0.1f);
		dr->DrawLine(ragdollInertia.com, comGround, JPH::Color::sCyan);
		// Where the controller is pushing
		dr->DrawArrow(comGround + lift, comGround + lift + correctionVector, JPH::Color::sOrange, 0.05f);


#endif

	}



	void ragdollUpdateUsingMotors(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;

		float& animTime = self.get_mut<AnimationTime>().time;

		const float dt = 1.0f / 60.0f;
		animTime += dt;

		animation->Sample(animTime, pose);

		//Place the root joint on the first body so that we draw the pose in the right place
		JPH::RVec3 root_offset;
		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);

		joint.mTranslation = JPH::Vec3::sZero();
		ragdoll->GetRootTransform(root_offset, joint.mRotation);

		pose.SetRootOffset(root_offset);

#ifdef JPH_DEBUG_RENDERER
		pose.CalculateJointMatrices();
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif 
		ragdoll->DriveToPoseUsingMotors(pose);
	}

	//Updates ragdoll using Kinematics allowing the ragdoll to move in the world using animationData,
	//Jolts default animations contain root motion, which is why the ragdoll moves.
	void updateRagdollKinematicRootMotionLoop(flecs::world& ecs, flecs::entity self) {

		//JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		//JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;

		const float dt = 1.0f / 60.0f;
		animTime += dt;

		animation->Sample(animTime, pose);

		// Sets the root offset to the position we spawned ragdoll to rather than the origin.
		pose.SetRootOffset(root_offset); 

		pose.CalculateJointMatrices();

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif

		ragdoll->DriveToPoseUsingKinematics(pose, dt);
	}

	// Ragdoll runs in place, driven to pose using Kinematics
	//This overwrites the root position and rotation
	void updateRagdollInPlaceKinematicPinnedNoRot(flecs::world& ecs, flecs::entity self) {

		//JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		//JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;

		const float dt = 1.0f / 60.0f;
		animTime += dt;

		animation->Sample(animTime, pose);

		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = JPH::Vec3::sZero(); // strip baked root translation

		//Set the rotation of root to the rotation of ragdoll
		JPH::Quat physicsRootRot;
		ragdoll->GetRootTransform(root_offset, physicsRootRot);
		joint.mRotation = physicsRootRot;

		pose.SetRootOffset(root_offset);

		pose.CalculateJointMatrices();

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif // JPH_DEBUG_RENDERER

		ragdoll->DriveToPoseUsingKinematics(pose, dt);
	}

	// Ragdoll runs in place, driven to pose using Kinematics
	//This overwrites the root position
	void updateRagdollInPlaceKinematicPinned(flecs::world& ecs, flecs::entity self) {

	//	JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
	//	JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;

		const float dt = 1.0f / 60.0f;
		animTime += dt;

		animation->Sample(animTime, pose);

		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = JPH::Vec3::sZero(); // strip baked root translation

		pose.SetRootOffset(root_offset);

		pose.CalculateJointMatrices();

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif

		ragdoll->DriveToPoseUsingKinematics(pose, dt);
	}

	// Ragdoll runs in place, driven to pose using Motors
	//This overwrites the root position
	void updateRagdollInPlaceMotorPinned(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;
		const float dt = 1.0f / 60.0f;

		animTime += dt;
		animation->Sample(animTime, pose);

		JPH::SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = JPH::Vec3::sZero(); // strip baked root translation
		pose.SetRootOffset(root_offset);

		//bi.SetPosition(ragdoll->GetBodyID(0), root_offset, JPH::EActivation::Activate);

		bi.AddForce(ragdoll->GetBodyID(0), root_offset, JPH::EActivation::Activate);



#ifdef JPH_DEBUG_RENDERER
		pose.CalculateJointMatrices();
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif

		ragdoll->DriveToPoseUsingMotors(pose);
	}

	//Updates ragdoll using sePosition allowing the ragdoll to move in the world using animationData,
	//Jolts default animations contain root motion, which is why the ragdoll moves.
	void updateRagdollSetPoseRootMotionLoop(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;

		const float dt = 1.0f / 60.0f;
		animTime += dt;

		animation->Sample(animTime, pose);

		// Sets the root offset to the position we spawned ragdoll to rather than the origin.
		pose.SetRootOffset(root_offset);

		pose.CalculateJointMatrices();

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif

		ragdoll->SetPose(pose, true);
	}


	void SnakeUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		JPH::BodyInterface& bi = physicsSystem.GetBodyInterface();

		// Get snake head (first body)
		JPH::BodyID headID = ragdoll->GetBodyID(0);
		JPH::Vec3 headPos = bi.GetPosition(headID);
		JPH::Quat headRot = bi.GetRotation(headID);

		JPH::Vec3 playerPos = ecs.get<PlayerRef>().value.get_mut<Player>().position;


		JPH::Vec3 toPlayer = playerPos - headPos;
		float distance = toPlayer.Length();
		JPH::Vec3 dirToPlayer = toPlayer / distance;

		//Apply force to head
		float chaseForce = 50000.0f;  // Tune this value
		bi.AddForce(headID, dirToPlayer * chaseForce);



	}




	void armUpdate(flecs::world& ecs, flecs::entity self) {

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		physicsSystem.GetBodyInterface().SetRotation(ragdoll->GetBodyID(0), JPH::Quat::sIdentity(), JPH::EActivation::Activate);
		JPH::TwoBodyConstraint* constraint0 = ragdoll->GetConstraint(1);
		JPH::HingeConstraint* hinge = static_cast<JPH::HingeConstraint*>(constraint0);

		Direction& direction = ecs.get_mut<Direction>();

		float angle = hinge->GetCurrentAngle();
		float min = hinge->GetLimitsMin();
		float max = hinge->GetLimitsMax();

		float range = abs(max - min);

		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!physicsSystem.GetBodyInterface().IsActive(id)) {

				LogWarn(LOG_APP, "BODY IS NOT ACTIVE");
				physicsSystem.GetBodyInterface().ActivateBody(id);
			}
		}

		JPH::MotorSettings& motor = hinge->GetMotorSettings();

		JPH::EMotorState motorState = hinge->GetMotorState();

		// Check if the hinge is already at the limit
		constexpr float tolerance = JPH::DegreesToRadians(1.0f);

		if (direction == Direction::forward) {
			float targetAngle = min;

			// Only set motor if we're not already there
			if (abs(angle - targetAngle) > tolerance) {
				hinge->SetMotorState(JPH::EMotorState::Position);
				hinge->SetTargetAngle(targetAngle);
			}
			else {
				// We're at the target, turn off motor to save energy
				//hinge->SetMotorState(EMotorState::Off);
				direction = Direction::backward;
			}
			LogWarn(LOG_APP, "Dir::forward");
		}
		else {
			float targetAngle = max;

			if (abs(angle - targetAngle) > tolerance) {
				hinge->SetMotorState(JPH::EMotorState::Position);
				hinge->SetTargetAngle(targetAngle);
			}
			else {
				//hinge->SetMotorState(EMotorState::Off);
				direction = Direction::forward;

			}
			LogWarn(LOG_APP, "Dir::backward");
		}


		//cout << "Hinge angle : " << JPH::RadiansToDegrees(angle) << "\n";
		//cout << "Hinge min : " << JPH::RadiansToDegrees(min) << "\n";
		//cout << "Hinge max : " << JPH::RadiansToDegrees(max) << "\n";
		//cout << "Motor State  : " << int(motorState) << "\n";
		//cout << "  " << "\n";

	}



	//placeholder or used for passive ragdoll
	void empty(flecs::world& ecs, flecs::entity self) {

	}

}