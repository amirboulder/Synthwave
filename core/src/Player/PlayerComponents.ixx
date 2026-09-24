module;

#include <flecs.h>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/RayCast.h>
#endif

export module PlayerComponents;

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

export class PlayerContactListener : public JPH::CharacterContactListener {

public:

	flecs::world& ecs;

	PlayerContactListener(flecs::world& ecs)
		:ecs(ecs)
	{


	}

	// Callback to adjust the velocity of a body as seen by the character.
	virtual void OnAdjustBodyVelocity(const JPH::CharacterVirtual* inCharacter, const JPH::Body& inBody2,
		JPH::Vec3& ioLinearVelocity,
		JPH::Vec3& ioAngularVelocity) override {

		//	cout << "player2:: OnAdjustBodyVelocity\n";

	};


	// Called whenever the character collides with a body.
	virtual void			OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2,
		JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnContactAdded \n";

		//ioSettings.mCanReceiveImpulses = true;
		//fisiks.physicsSystem.GetBodyInterface().AddImpulse(inBodyID2, Vec3(0, 20.0f, 0));

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		bodyInterface.SetLinearVelocity(inBodyID2, inContactNormal * 10);


	};

	// Called whenever the character persists colliding with a body.
	virtual void			OnContactPersisted(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
		const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnContactPersisted \n";
	};

	// Called whenever the character loses contact with a body.
	virtual void			OnContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2,
		const JPH::SubShapeID& inSubShapeID2) override {

		//cout << "player2:: OnContactRemoved \n";
	};

	// Called whenever the character collides with a virtual character.
	virtual void			OnCharacterContactAdded(const JPH::CharacterVirtual* inCharacter,
		const JPH::CharacterVirtual* inOtherCharacter,
		const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal,
		JPH::CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnCharacterContactAdded \n";

	};

	// Called whenever the character persists colliding with a virtual character.
	virtual void			OnCharacterContactPersisted(const JPH::CharacterVirtual* inCharacter,
		const JPH::CharacterVirtual* inOtherCharacter,
		const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal,
		JPH::CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnCharacterContactAdded \n";

	};

	// Called whenever the character loses contact with a virtual character.
	virtual void			OnCharacterContactRemoved(const JPH::CharacterVirtual* inCharacter,
		const JPH::CharacterID& inOtherCharacterID,
		const JPH::SubShapeID& inSubShapeID2) override {

		//cout << "player2:: OnCharacterContactRemoved \n";

	};

	// Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
	virtual void			OnContactSolve(const JPH::CharacterVirtual* inCharacter,
		const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2,
		JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity,
		const JPH::PhysicsMaterial* inContactMaterial,
		JPH::Vec3Arg inCharacterVelocity, JPH::Vec3& ioNewCharacterVelocity) override {


		//cout << "player2:: OnContactSolve \n";


	};

};


export class Player {

public: 

	JPH::TempAllocatorImpl* temp_allocator;

	PlayerContactListener contactListener;

	//Maybe not needed
	//CharacterVsCharacterCollisionSimple mCharacterVsCharacterCollision;

	flecs::world& ecs;

	JPH::Ref<JPH::CharacterVirtual>	mCharacter;
	JPH::BodyID innerBodyID;
	JPH::Ref<JPH::Shape> bodyShape = new JPH::CapsuleShape(2.0f, 1.0f);

	JPH::Vec3					mDesiredVelocity = JPH::Vec3::sZero();
	JPH::Vec3 position = JPH::Vec3(1.0f, 15.0f, 0.0f);
	JPH::Quat rotation = JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f);

	// Movement state
	JPH::Vec3 mVerticalVelocity = JPH::Vec3::sZero();
	float moveSpeed = 16.0f;
	float jumpSpeed = 8.0f;
	float terminalVelocity = -50.0f;
	JPH::Vec3 gravity = JPH::Vec3(0, -20.0f, 0);

	float timeStep = 1.0f / 60.0f;

	glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 0.0f);

	// Input state
	JPH::Vec3 movementDirection = JPH::Vec3::sZero();
	bool mJumpPressed = false;

	uint32_t ballCounter = 0;

	flecs::entity interactEventEnt;
	flecs::entity attackEventEnt;
	flecs::entity forwardMVMTEnt;
	flecs::entity backwardMVMTEnt;
	flecs::entity leftMVMTEnt;
	flecs::entity rightMVMTEnt;
	flecs::entity jumpMVMTEnt;

	UserInput input;

	Player(flecs::world& ecs, JPH::Vec3Arg position, JPH::QuatArg rotation, float height, float radius, uint64_t entityID, bool createInnerBody = false)
		:ecs(ecs), contactListener(ecs)
	{

		temp_allocator = new JPH::TempAllocatorImpl(1 * 1024 * 1024);

		init(position, rotation, height, radius, entityID, createInnerBody);

	}

	void init(JPH::Vec3Arg position, JPH::QuatArg rotation, float height, float radius, uint64_t entityID, bool createInnerBody = false) {


		JPH::EBackFaceMode sBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
		//float		sUpRotationX = 0;
		//float		sUpRotationZ = 0;
		float		sMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
		float		sMaxStrength = 10000.0f;
		float		sMass = 70;
		float		sCharacterPadding = 0.02f;
		float		sPenetrationRecoverySpeed = 1.0f;
		float		sPredictiveContactDistance = 0.1f;
		//bool		sEnableWalkStairs = true;
		//bool		sEnableStickToFloor = true;
		bool		sEnhancedInternalEdgeRemoval = false;
		//bool		sCreateInnerBody = true;
		//bool		sPlayerCanPushOtherCharacters = true;
		//bool		sOtherCharactersCanPushPlayer = true;

		// Create 'player' character
		JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
		settings->mMaxSlopeAngle = sMaxSlopeAngle;
		settings->mMaxStrength = sMaxStrength;
		settings->mMass = sMass;
		settings->mShape = bodyShape;
		settings->mBackFaceMode = sBackFaceMode;
		settings->mCharacterPadding = sCharacterPadding;
		settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		settings->mPredictiveContactDistance = sPredictiveContactDistance;

		settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius); // Accept contacts that touch the lower sphere of the capsule
		settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
		settings->mInnerBodyShape = createInnerBody ? bodyShape : nullptr;
		settings->mInnerBodyLayer = Layers::MOVING;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		mCharacter = new JPH::CharacterVirtual(settings, position, rotation, entityID, &physicsSystem);
		//mCharacter->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
		//mCharacterVsCharacterCollision.Add(mCharacter);

		innerBodyID = mCharacter->GetInnerBodyID();

		mCharacter->SetListener(&contactListener);

		lookupEventEnts();
	}

	bool lookupEventEnts() {

		//Get the required Event entities.

		interactEventEnt = ecs.lookup("InteractEvent");
		if (!interactEventEnt) {
			LogError(LOG_APP, "interactEventEnt is null");
			return false;
		}

		attackEventEnt = ecs.lookup("Attack1EventEnt");
		if (!interactEventEnt) {
			LogError(LOG_APP, "Attack1EventEnt is null");
			return false;
		}


		forwardMVMTEnt = ecs.lookup("forwardMVMTEnt");
		if (!forwardMVMTEnt) {
			LogError(LOG_APP, "forwardMVMTEnt is null");
			return false;
		}

		backwardMVMTEnt = ecs.lookup("backwardMVMTEnt");
		if (!backwardMVMTEnt) {
			LogError(LOG_APP, "backwardMVMTEnt is null");
			return false;
		}

		leftMVMTEnt = ecs.lookup("leftMVMTEnt");
		if (!leftMVMTEnt) {
			LogError(LOG_APP, "leftMVMTEnt is null");
			return false;
		}

		rightMVMTEnt = ecs.lookup("rightMVMTEnt");
		if (!rightMVMTEnt) {
			LogError(LOG_APP, "rightMVMTEnt is null");
			return false;
		}

		jumpMVMTEnt = ecs.lookup("jumpMVMTEnt");
		if (!jumpMVMTEnt) {
			LogError(LOG_APP, "jumpMVMTEnt is null");
			return false;
		}


		return true;
	}

};


export struct PlayerRef { flecs::entity value = flecs::entity::null(); };
export struct PlayerCamRef { flecs::entity value = flecs::entity::null(); };

