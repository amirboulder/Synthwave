#pragma once

#include "../../core/src/ecs/components.hpp"
#include "../../core/src/Registery/registry.hpp"

enum class Direction { forward, backward };

/*
struct GameModule {

	GameModule(flecs::world& world) {
		world.module<GameModule>("GameModule");
	}
};
*/


void RegisterPlayerSystems(flecs::world& ecs) {
	
	//ecs.component<EnemyState>()
	//	.on_set([](EnemyState& newState) {
	//	LogInfo(LOG_APP, "new State %s", magic_enum::enum_name(newState).data());
	//});

	/*ecs.component<EnemyState>()
		.on_replace([](EnemyState& prev, EnemyState& next) {

		LogInfo(LOG_APP, "%s replace with %s", magic_enum::enum_name(prev).data(), magic_enum::enum_name(next).data());
	});*/


}
//REGISTER_GAME_MODULE(RegisterPlayerSystems)


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

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;


		float& animTime = self.get_mut<AnimationTime>().time;

		float dt = ecs.delta_time();

		
		// Advance animation time
		animTime += dt;

		// Loop animation if needed
		float animDuration = animation->GetDuration();
		if (animTime > animDuration) {
			animTime = fmod(animTime, animDuration);
			return;
		}
		// Position ragdoll
		//animation->Sample(animTime, pose);
		animation->Sample(animTime, pose);
		
		//Place the root joint on the first body so that we draw the pose in the right place
		RVec3 root_offset;
		SkeletonPose::JointState& joint = pose.GetJoint(0);

		joint.mTranslation = Vec3::sZero(); // All the translation goes into the root offset
		ragdoll->GetRootTransform(root_offset, joint.mRotation);


		pose.SetRootOffset(root_offset);
		pose.CalculateJointMatrices();
#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif // JPH_DEBUG_RENDERER


		//ragdoll->DriveToPoseUsingKinematics(pose, dt, true);
		ragdoll->DriveToPoseUsingMotors(pose);
		//ragdoll->SetPose(pose);


	}

	void releasePose(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint) {

		for (int i = 0; i < ragdoll->GetConstraintCount(); ++i)
		{
			//TODO handle other types of constraints
			EConstraintSubType sub_type = ragdoll->GetConstraint(i)->GetSubType();
			if (sub_type == EConstraintSubType::SwingTwist)
			{
				SwingTwistConstraint* st_constraint = static_cast<SwingTwistConstraint*>(hipConstraint);
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
			EConstraintSubType sub_type = ragdoll->GetConstraint(i)->GetSubType();
			if (sub_type == EConstraintSubType::SwingTwist)
			{
				SwingTwistConstraint* st_constraint = static_cast<SwingTwistConstraint*>(hipConstraint);
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
		
		EConstraintSubType sub_type = constraint->GetSubType();

		if (sub_type != EConstraintSubType::SixDOF) {
			LogError(LOG_PHYSICS, "Expected SixDOF constraint here but got : %s ", magic_enum::enum_name(sub_type).data());
			return;
		}

		SixDOFConstraint* st_constraint = static_cast<SixDOFConstraint*>(constraint);

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::RotationX + i);
			st_constraint->SetMotorState(axisRot, EMotorState::Off);
		}

		st_constraint->SetEnabled(false);

	}

	void enableConstraint(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* constraint) {

		EConstraintSubType sub_type = constraint->GetSubType();

		if (sub_type != EConstraintSubType::SixDOF) {
			LogError(LOG_PHYSICS, "Expected SixDOF constraint here but got : %s ", magic_enum::enum_name(sub_type).data());
			return;
		}

		SixDOFConstraint* st_constraint = static_cast<SixDOFConstraint*>(constraint);

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::RotationX + i);
			st_constraint->SetMotorState(axisRot, EMotorState::Position);
		}

		st_constraint->SetEnabled(true);

	}


	void driveToPose(JPH::PhysicsSystem& physicsSystem, JPH::Ragdoll* ragdoll, JPH::SixDOFConstraint* hipConstraint,const JPH::Quat& characterRot, const JPH::SkeletalAnimation* animation, JPH::SkeletonPose& pose,float& animTime) {

		float animDuration = animation->GetDuration();

		//TODO dt should not be hardcoded but also should not be gotten from ecs.delta_time either
		//Since physics runs at a fixed timestep we should get that value once use its
		const float dt = 0.0166666;
		animTime += dt;// Advance animation time

		animation->Sample(animTime, pose);

		// Keep the hip's authored body frame from the pose/physics. Overwriting with
		// characterRot drives pose motors toward the upright capsule frame and leans.
		RVec3 root_offset;
		SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = Vec3::sZero();
		ragdoll->GetRootTransform(root_offset, joint.mRotation);

		joint.mRotation = characterRot;

		JPH::BodyID rootID = ragdoll->GetBodyID(0);


		physicsSystem.GetBodyInterface().SetRotation(rootID, characterRot, JPH::EActivation::Activate);


		pose.SetRootOffset(root_offset);
		pose.CalculateJointMatrices();


		pose.CalculateJointStates();

		ragdoll->DriveToPoseUsingMotors(pose); //This will active motors

		//hipConstraint->SetTargetPositionCS(Vec3::sZero());
		//hipConstraint->SetTargetOrientationCS(characterRot);

	}


	void rotateCharacterTowardsPlayer(JPH::PhysicsSystem& physicsSystem, JPH::IgnoreMultipleBodiesFilter* ragdollFilter, JPH::Character* joltCharacter, const Player& player) {

		//get all the actor info
		JPH::Vec3 actorPos = joltCharacter->GetPosition();
		JPH::Quat actorRot = joltCharacter->GetRotation();
		float actorEyeHeight = 1.3f;
		JPH::Vec3 actorEyePos = actorPos + JPH::Vec3(0.0f, actorEyeHeight, 0.0f);
		JPH::Vec3 actorForward = actorRot.RotateAxisZ();

		JPH::Vec3 playerPos = player.position;
		//float playerEyeHeight = 1.3f;
		//JPH::Vec3 playerEyePos = actorPos + JPH::Vec3(0.0f, playerEyeHeight, 0.0f);

		JPH::Vec3 toPlayer = playerPos - actorEyePos;

		float distanceToPlayer = toPlayer.Length();
		float maxVisibilityRange = 50.0f;

		bool playerVisible = false;

		//runs on function exit
		//We want this code to run but don't want to repeat ourselves
		//TODO fix
		/*scope_exit sendVisibilityInfo([&]() {
			self.set<ActorDebugInfo>({ playerVisible });
		});*/

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
		JPH::IgnoreSingleBodyFilterChained bodyFilter(joltCharacter->GetBodyID(), *ragdollFilter);


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

		float deltaTime = 0.016666;;

		//Turn towards the player
		float turnSpeed = 1.0f;
		JPH::Vec3 flatDir(dirToPlayer.GetX(), 0.0f, dirToPlayer.GetZ());

		JPH::Quat targetRot = targetRot = dirToQuat(flatDir);

		float t;
		t = 1.0f - expf(-8.0f * deltaTime);

		JPH::Quat newRot = actorRot.SLERP(targetRot, t);
		joltCharacter->SetRotation(newRot);


	}


	void switchAnimation(const JoltAnimationList& joltAnimationList, JoltAnimation& animation, const std::string& newAnimationName) {

		JPH::SkeletalAnimation* newAnimPtr = joltAnimationList.find(newAnimationName);

		if (!newAnimPtr) {
			LogError(LOG_APP, "Unable to switch Animation because Animation name %s is not present in animation list for entity", newAnimationName.c_str());
			return;
		}

		animation.animationPtr = newAnimPtr;
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

	void updateIdle(
		JPH::PhysicsSystem& physicsSystem,
		JPH::SixDOFConstraint* hipConstraint,
		const JPH::Quat& characterRot,
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter,
		JPH::Character* joltCharacter,
		const Player& player, JPH::Ragdoll* ragdoll,
		JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {

		rotateCharacterTowardsPlayer(physicsSystem, ragdollFilter, joltCharacter, player);
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
		JPH::SixDOFConstraint* hipConstraint,
		const JPH::Quat& characterRot,
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter,
		JPH::Character* joltCharacter,
		const Player& player, JPH::Ragdoll* ragdoll,
		JPH::SkeletalAnimation* animation,
		JPH::SkeletonPose& pose,
		float& animTime) {

		rotateCharacterTowardsPlayer(physicsSystem, ragdollFilter, joltCharacter, player);
		driveToPose(physicsSystem, ragdoll, hipConstraint, characterRot, animation, pose, animTime);

	}

	void OnEnterChase(JPH::Ragdoll* ragdoll, JPH::TwoBodyConstraint* hipConstraint, const JoltAnimationList& joltAnimationList, JoltAnimation& animation) {

		
		switchAnimation(joltAnimationList, animation, "sprint");
		enableConstraint(ragdoll, hipConstraint);

		
	}

	void onExitChase() {

	}

	void updateRagdollMotor(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Character* joltCharacter = self.get<JoltCharacter>().characterPtr;
		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SixDOFConstraint* hipConstraint = self.get_mut<PhysicsConstraint>().constraint;
		JoltAnimationList joltAnimationList = self.get_mut<JoltAnimationList>();
		JoltAnimation joltAnimation = self.get_mut<JoltAnimation>();
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter = self.get_mut<JoltRagdollFilter>().filter;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		const EnemyState& state = self.get<EnemyState>();
		float& animTime = self.get_mut<AnimationTime>().time;
		const Player& player = ecs.get<PlayerRef>().value.get<Player>();

		joltCharacter->PostSimulation(0.1f); // PostSimulation update must happen first

		JPH::Quat characterRot = joltCharacter->GetRotation();

		

		switch (state)
		{
			case EnemyState::SLEEP:
			{
				//deathFunction(ragdoll, hipConstraint);
				break;
			}
			case EnemyState::IDLE:
			{
				updateIdle(physicsSystem, hipConstraint, characterRot, ragdollFilter, joltCharacter, player, ragdoll, animation, pose, animTime);
				break;
			}
			case EnemyState::SEARCH:
			{
				break;
			}
			case EnemyState::CHASE:
			{
				updateChase(physicsSystem, hipConstraint, characterRot, ragdollFilter, joltCharacter, player, ragdoll, animation, pose, animTime);
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
			default:
			{	
				break;
			}

		}

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);

		hipConstraint->DrawConstraint(JPH::DebugRenderer::sInstance);
		hipConstraint->DrawConstraintLimits(JPH::DebugRenderer::sInstance);
#endif

	}

	void RegisterRagdollHooks(flecs::world& ecs) {

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

				return;
			case EnemyState::DISABLED:

				return;

			default:

				break;
			}

		});

	}
	REGISTER_GAME_MODULE(RegisterRagdollHooks)


	void updateRagdollForce(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::IgnoreMultipleBodiesFilter* ragdollFilter = self.get<JoltRagdollFilter>().filter;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose2>().pose;
		//JoltAnchorBody& anchor = self.get_mut<JoltAnchorBody>();
		float hipsFromSoles = self.get_mut<JoltPose2>().hipsFromSoles;

		float& animTime = self.get_mut<AnimationTime>().time;
		float dt = ecs.delta_time();

		// Advance animation time
		animTime += dt;

		//// Loop animation if needed
		//float animDuration = animation->GetDuration();
		//if (animTime > animDuration) {
		//	animTime = fmod(animTime, animDuration);
		//}
		
		//Just getting the first key frame of the animation
		animation->Sample(0.0f, pose);
		
		SkeletonPose::JointState& hipJoint = pose.GetJoint(0);

		//Place the root joint on the first body so that we draw the pose in the right place
		RVec3 root_offset;
		JPH::BodyID rootID = ragdoll->GetBodyID(0);
		ragdoll->GetRootTransform(root_offset, hipJoint.mRotation); //Try using a different rot 

		hipJoint.mTranslation = Vec3::sZero(); // strip baked root translation
		pose.SetRootOffset(root_offset);
		pose.CalculateJointMatrices();


		JPH::Vec3 rootPos = bi.GetPosition(rootID);
		JPH::Quat rootRot = bi.GetRotation(rootID);

		ragdoll->DriveToPoseUsingMotors(pose);


		//Set position to distance from the ground
		GroundInfo groundInfo = Utils::Phys::CheckGround(physicsSystem, rootPos, *ragdollFilter);

		//RefConst<Shape> groundShape =  bi.GetShape(groundInfo.groundBodyID);

		if (groundInfo.distanceToGround < hipsFromSoles) {

			float forceMult = 50000.0f;

			JPH::Vec3 toTarget = JPH::Vec3(0.0f,1.0f,0.0f);

			//bi.AddForce(rootID, toTarget * forceMult);
		}

		//cout << "target Height " << targetHeight << std::endl;

		//Vec3 CurrentPos = bi.GetPosition(anchor.bodyID);
		//Vec3 targetPos = Vec3(CurrentPos.GetX(), targetHeight, CurrentPos.GetZ());

		//JPH::Vec3 toTarget = targetPos - rootPos;
		//JPH::Vec3 dirToTarget = toTarget.Normalized();

		//JPH::Quat qDiff = targetRot * rootRot.Inversed();
		//JPH::Vec3 targetDir = quatToDirection(qDiff);

		

		//bi.AddForceAndTorque(anchor.bodyID, dirToTarget * forceMult, targetDir * forceMult);
		//bi.AddForce(anchor.bodyID, dirToTarget * forceMult);

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);

#endif

	}

	void updateRagdollPD(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		float& animTime = self.get_mut<AnimationTime>().time;
		float dt = ecs.delta_time();

		// Advance animation time
		animTime += dt;

		// Loop animation if needed
		float animDuration = animation->GetDuration();
		if (animTime > animDuration) {
			animTime = fmod(animTime, animDuration);
			return;
		}
		// Position ragdoll
		//animation->Sample(animTime, pose);
		animation->Sample(0.0f, pose);


		SkeletonPose::JointState& joint = pose.GetJoint(0);


		JPH::BodyID rootID = ragdoll->GetBodyID(0);

		pose.SetRootOffset(root_offset);
		pose.CalculateJointMatrices();

		ragdoll->DriveToPoseUsingMotors(pose);


#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif

	}


	void updateRagdollNoAnim(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;

#ifdef JPH_DEBUG_RENDERER
		//pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif // JPH_DEBUG_RENDERER

	}

	void updateRagdollKinematic(flecs::world& ecs, flecs::entity self) {

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		JPH::Ragdoll* ragdoll = self.get<JoltRagdoll>().ragdollPtr;
		JPH::SkeletalAnimation* animation = self.get<JoltAnimation>().animationPtr;
		JPH::SkeletonPose& pose = self.get_mut<JoltPose>().pose;
		JPH::Vec3& root_offset = self.get_mut<JoltPose>().root_offset;

		const Player& player = ecs.get<PlayerRef>().value.get<Player>();
		JPH::Vec3 playerPos = player.position;

		float& animTime = self.get_mut<AnimationTime>().time;

		float dt = ecs.delta_time(); //TODO ecs.ge<TimeStep>

		//JPH::BodyID rootBodyId = ragdoll->GetBodyID(0);

		// Position ragdoll
		animTime += dt;
		animation->Sample(animTime, pose);

		SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = Vec3::sZero(); // strip baked root translation

		//Sync world root 
		Quat physicsRootRot;
		ragdoll->GetRootTransform(root_offset, physicsRootRot);
		joint.mRotation = physicsRootRot;

		/*
		Vec3 toPlayer = playerPos - root_offset;
		float distance = toPlayer.Length();
		Vec3 dirToPlayer = toPlayer / distance;
		JPH::Vec3 flatDir(dirToPlayer.GetX(), 0.0f, dirToPlayer.GetZ());

		float moveSpeed = 5.0f;

		if (flatDir.LengthSq() > 1e-6f) {
			joint.mRotation = dirToQuat(flatDir); // or SLERP
			root_offset += flatDir * moveSpeed * dt;
		}
		*/

		float moveSpeed = 5.0f;

		Vec3 dir = -quatToDirection(physicsRootRot);
		//Vec3 dir = Vec3(0.0f, 0.0f, -1.0f);

		root_offset += dir * moveSpeed * dt;

		pose.SetRootOffset(root_offset);

		pose.CalculateJointMatrices();


#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);
#endif // JPH_DEBUG_RENDERER

		ragdoll->DriveToPoseUsingKinematics(pose, dt);
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

				LogWarn(LOG_APP, "BODY IS NOT ACTIVE");
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
			LogWarn(LOG_APP, "Dir::forward");
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
			LogWarn(LOG_APP, "Dir::backward");
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