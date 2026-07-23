#pragma once

class Biped {

public:

	JPH::Character* joltCharacter;
	JPH::Ragdoll* ragdoll;
	JPH::SixDOFConstraint* hipConstraint;
	JPH::SkeletalAnimation* animation;
	JPH::SkeletonPose pose;
	Vec3 root_offset;
	float animTime;
	float animDuration;
	JPH::BodyID rootID;

	Biped() {

	}

	void setAnimation(JPH::SkeletalAnimation* newAnimation) {

		if (!newAnimation) {
			LogError(LOG_PHYSICS, "JPH::SkeletalAnimation is nullptr");
		}

		animation = newAnimation;
		animDuration = animation->GetDuration();
		animTime = 0;
	}

	void driveToToPose(float dt) {

		joltCharacter->PostSimulation(0.1f); // PostSimulation update must happen first

		// Advance animation time
		animTime += dt;

		if (animDuration > 0.0f) {
			// Advance and wrap in a single operational pass
			animTime = std::fmod(animTime + dt, animDuration);
		}
		// Position ragdoll
		animation->Sample(animTime, pose);


		//Set skeleton position and rotation to where the ragdoll actually is
		SkeletonPose::JointState& joint = pose.GetJoint(0);
		joint.mTranslation = Vec3::sZero();
		ragdoll->GetRootTransform(root_offset, joint.mRotation);

		pose.SetRootOffset(root_offset);
		pose.CalculateJointMatrices();

		ragdoll->DriveToPoseUsingMotors(pose);

		// Position-only hip anchor; rotation stays free (pose motors own orientation).
		for (int i = 0; i < 3; ++i)
		{
			auto axis = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::TranslationX + i);
			hipConstraint->SetMotorState(axis, EMotorState::Position);

			auto axisRot = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::RotationX + i);
			hipConstraint->SetMotorState(axisRot, EMotorState::Off);
		}

#ifdef JPH_DEBUG_RENDERER
		pose.Draw({}, JPH::DebugRenderer::sInstance);

		hipConstraint->DrawConstraint(JPH::DebugRenderer::sInstance);
		hipConstraint->DrawConstraintLimits(JPH::DebugRenderer::sInstance);
#endif
	}


	void releasePose() {


		//TODO store a list of RTTI for constraints so we know what to case each one to
		//This one could break if we stop using jolts Human.tof which uses SwingTwistConstraint for everything
		for (int i = 0; i < ragdoll->GetConstraintCount(); ++i)
		{
			TwoBodyConstraint* constraint = ragdoll->GetConstraint(i);
			SwingTwistConstraint* st =
				static_cast<SwingTwistConstraint*>(constraint);

			st->SetSwingMotorState(JPH::EMotorState::Off);
			st->SetTwistMotorState(JPH::EMotorState::Off);
		}
	}

	void stopHoldingUpRagdoll() {

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::RotationX + i);
			hipConstraint->SetMotorState(axisRot, EMotorState::Off);
		}
	}

	void startHoldingUpRagdoll() {

		for (int i = 0; i < 3; ++i)
		{
			auto axisRot = SixDOFConstraintSettings::EAxis(SixDOFConstraintSettings::EAxis::RotationX + i);
			hipConstraint->SetMotorState(axisRot, EMotorState::Position);
		}
	}
};


class BipedAI {

	EnemyState state;
	flecs::world & ecs;

	void toSLEEP() {

	}

	void updateSleep() {

	}

	void exitSleep() {

	}

public:

	void setState(const EnemyState& newState) {

		if (state == newState) {
			return;
		}

		switch (newState)
		{
		case EnemyState::SLEEP:
			break;
		case EnemyState::SEARCH:
			break;
		case EnemyState::CHASE:
			break;
		case EnemyState::FIGHT:
			break;
		case EnemyState::DISABLED:
			break;
		case EnemyState::CRAWLING:
			break;
		case EnemyState::PARALYSED:
			break;
		case EnemyState::DEAD:
			break;
		default:
			break;
		}
	}


	EnemyState getState() {

		return state;
	}

	void update() {

		switch (state)
		{
		case EnemyState::SLEEP:
			break;
		case EnemyState::SEARCH:
			break;
		case EnemyState::CHASE:
			break;
		case EnemyState::FIGHT:
			break;
		case EnemyState::DISABLED:
			break;
		case EnemyState::CRAWLING:
			break;
		case EnemyState::PARALYSED:
			break;
		case EnemyState::DEAD:
			break;
		default:
			break;
		}

	}

	void enemyStateOnSetHook() {

		ecs.observer<EnemyState>("EnemyStateObserver")
			.event(flecs::OnSet)
			.with<JoltAnimation>()
			.each([](flecs::iter& it, size_t i, EnemyState& s) {


			
			});


		/*
		ecs.component<EnemyState>()
			.on_replace([](EnemyState& oldState) {
			
			LogInfo(LOG_APP, "Prev State " , magic_enum::enum_name(oldState));
		})
			.on_set([](EnemyState& newState) {


			switch (newState)
			{
			case EnemyState::SLEEP:
				break;
			case EnemyState::SEARCH:
				break;
			case EnemyState::CHASE:
				break;
			case EnemyState::FIGHT:
				break;
			case EnemyState::DISABLED:
				break;
			case EnemyState::CRAWLING:
				break;
			case EnemyState::PARALYSED:
				break;
			case EnemyState::DEAD:
				break;
			default:
				break;
			}

		});

		*/


			
	}


};