#pragma once 

#include "renderer/Model.hpp"
#include "renderer/Camera.hpp"


//TODO create class PlayerContactListener : public JPH::CharacterContactListener 
class Player : public CharacterContactListener {


public:

	TempAllocatorImpl* temp_allocator;

	//Maybe not needed
	//CharacterVsCharacterCollisionSimple mCharacterVsCharacterCollision;

	flecs::world& ecs;

	Ref<CharacterVirtual>	mCharacter;
	Vec3					mDesiredVelocity = Vec3::sZero();

	JPH::Vec3 position = JPH::Vec3(1.0f, 15.0f, 0.0f);
	JPH::Quat rotation = JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f);

	// Movement state
	JPH::Vec3 mVerticalVelocity = Vec3::sZero();
	float moveSpeed = 16.0f;
	float jumpSpeed = 8.0f;
	float terminalVelocity = -50.0f;
	JPH::Vec3 gravity = Vec3(0, -20.0f, 0);

	//TODO query it from FISIKS FIX FIX FIX
	float physicsTickRate = 1.0f / 60.0f;

	glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 0.0f);

	// Input state
	Vec3 mMoveInput = Vec3::sZero();
	bool mJumpPressed = false;
	
	//how many pixel to rotate the camera
	float offsetX = 0.0f;
	float offsetY = 0.0f;


	Player(flecs::world& ecs)
		:ecs(ecs)
	{

		temp_allocator = new TempAllocatorImpl(1 * 1024 * 1024);
	
	}

	Player(flecs::world& ecs, JPH::Vec3Arg position, JPH::QuatArg rotation, float height, float radius, uint64_t entityID)
		:ecs(ecs) 
	{

		temp_allocator = new TempAllocatorImpl(1 * 1024 * 1024);

		init(position, rotation, height, radius, entityID);

	}

	~Player() {
		
	}

	void init(JPH::Vec3Arg position,JPH::QuatArg rotation,float height, float radius, uint64_t entityID) {

		
		EBackFaceMode sBackFaceMode = EBackFaceMode::CollideWithBackFaces;
		//float		sUpRotationX = 0;
		//float		sUpRotationZ = 0;
		float		sMaxSlopeAngle = DegreesToRadians(45.0f);
		float		sMaxStrength = 10000.0f;
		float		sMass = 70;
		float		sCharacterPadding = 0.02f;
		float		sPenetrationRecoverySpeed = 1.0f;
		float		sPredictiveContactDistance = 0.1f;
		//bool		sEnableWalkStairs = true;
		//bool		sEnableStickToFloor = true;
		bool		sEnhancedInternalEdgeRemoval = false;
		// sCreateInnerBody = true breaks it
		bool		sCreateInnerBody = false;
		//bool		sPlayerCanPushOtherCharacters = true;
		//bool		sOtherCharactersCanPushPlayer = true;

		// Create 'player' character
		Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
		settings->mMaxSlopeAngle = sMaxSlopeAngle;
		settings->mMaxStrength = sMaxStrength;
		settings->mMass = sMass;
		settings->mShape = new JPH::CapsuleShape(height / 2.0, radius);
		settings->mBackFaceMode = sBackFaceMode;
		settings->mCharacterPadding = sCharacterPadding;
		settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		settings->mPredictiveContactDistance = sPredictiveContactDistance;

		settings->mSupportingVolume = Plane(Vec3::sAxisY(), -radius); // Accept contacts that touch the lower sphere of the capsule
		settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
		settings->mInnerBodyShape = sCreateInnerBody ? new JPH::CapsuleShape(height / 2.0, radius) : nullptr;
		settings->mInnerBodyLayer = Layers::MOVING;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		mCharacter = new CharacterVirtual(settings, position, rotation, entityID, &physicsSystem);
		//mCharacter->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
		//mCharacterVsCharacterCollision.Add(mCharacter);


		mCharacter->SetListener(this);
	}

	void reset() {

	}

	// Callback to adjust the velocity of a body as seen by the character.
	virtual void OnAdjustBodyVelocity( const CharacterVirtual* inCharacter, const Body& inBody2,
		Vec3& ioLinearVelocity, 
		Vec3& ioAngularVelocity) override {
		
	//	cout << "player2:: OnAdjustBodyVelocity\n";
	
	};


	// Called whenever the character collides with a body.
	virtual void			OnContactAdded(const CharacterVirtual* inCharacter, const BodyID& inBodyID2, const SubShapeID& inSubShapeID2,
		RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
		
		//cout << "player2:: OnContactAdded \n";

		//ioSettings.mCanReceiveImpulses = true;
		//fisiks.physicsSystem.GetBodyInterface().AddImpulse(inBodyID2, Vec3(0, 20.0f, 0));

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		bodyInterface.SetLinearVelocity(inBodyID2, inContactNormal * 10);
		

	};

	// Called whenever the character persists colliding with a body.
	virtual void			OnContactPersisted(const CharacterVirtual* inCharacter, const BodyID& inBodyID2,
		const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings& ioSettings) override {
		
		//cout << "player2:: OnContactPersisted \n";
	};

	// Called whenever the character loses contact with a body.
	virtual void			OnContactRemoved(const CharacterVirtual* inCharacter, const BodyID& inBodyID2,
		const SubShapeID& inSubShapeID2) override {
		
		//cout << "player2:: OnContactRemoved \n";
	};

	// Called whenever the character collides with a virtual character.
	virtual void			OnCharacterContactAdded(const CharacterVirtual* inCharacter, 
		const CharacterVirtual* inOtherCharacter,
		const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, 
		CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnCharacterContactAdded \n";
	
	};

	// Called whenever the character persists colliding with a virtual character.
	virtual void			OnCharacterContactPersisted(const CharacterVirtual* inCharacter, 
		const CharacterVirtual* inOtherCharacter,
		const SubShapeID& inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, 
		CharacterContactSettings& ioSettings) override {

		//cout << "player2:: OnCharacterContactAdded \n";
	
	};

	// Called whenever the character loses contact with a virtual character.
	virtual void			OnCharacterContactRemoved(const CharacterVirtual* inCharacter, 
		const CharacterID& inOtherCharacterID,
		const SubShapeID& inSubShapeID2) override {

		//cout << "player2:: OnCharacterContactRemoved \n";
	
	};

	// Called whenever the character movement is solved and a constraint is hit. Allows the listener to override the resulting character velocity (e.g. by preventing sliding along certain surfaces).
	virtual void			OnContactSolve(const CharacterVirtual* inCharacter, 
		const BodyID& inBodyID2, const SubShapeID& inSubShapeID2,
		RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, 
		const PhysicsMaterial* inContactMaterial,
		Vec3Arg inCharacterVelocity, Vec3& ioNewCharacterVelocity) override {


		//cout << "player2:: OnContactSolve \n";
	

	};

	void UpdateVelocity() {
		CharacterVirtual::EGroundState groundState = mCharacter->GetGroundState();

		if (groundState == CharacterVirtual::EGroundState::OnGround) {
			// On ground
			mVerticalVelocity = Vec3::sZero();

			// Jump
			if (mJumpPressed) {
				mVerticalVelocity = Vec3(0, jumpSpeed, 0);
				mJumpPressed = false;  // Consume jump input
			}
		}
		else {
			// In air: Apply gravity manually
			mVerticalVelocity += gravity * physicsTickRate;

			// Clamp to terminal velocity
			if (mVerticalVelocity.GetY() < terminalVelocity) {
				mVerticalVelocity.SetY(terminalVelocity);
			}
		}
	}

	void update() {

		UpdateVelocity();
		UpdateCharacter();
		updatePlayerCam();
	}

	
	void UpdateCharacter() {

		// Horizontal movement (player controlled)
		Vec3 horizontalVelocity = mMoveInput * moveSpeed;
		horizontalVelocity.SetY(0);  // Keep horizontal only

		// Combine with vertical velocity (gravity/jump)
		Vec3 totalVelocity = horizontalVelocity + mVerticalVelocity;

		mCharacter->SetLinearVelocity(totalVelocity);

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = physicsSystem.GetDefaultBroadPhaseLayerFilter(1);
		const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;

		const DefaultObjectLayerFilter default_object_layer_filter = physicsSystem.GetDefaultLayerFilter(1);
		const ObjectLayerFilter& object_layer_filter = default_object_layer_filter;

		const BodyFilter body_filter;
		const ShapeFilter shapeFilter;


		mCharacter->Update(physicsTickRate, gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *temp_allocator);

		//mCharacter->ExtendedUpdate(physicsTickRate, gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *fisiks.temp_allocator);

		position = mCharacter->GetPosition();
		rotation = mCharacter->GetRotation();

	}

	void SetMoveInput(Vec3 input) {
		mMoveInput = input;
	}

	void Jump() {
		if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround) {
			mJumpPressed = true;
		}
	}


	void updatePlayerCam() {

		// keep a ref instead
		flecs::entity cameraEnt = ecs.get<PlayerCamRef>().value;

		if (!cameraEnt) {
			return;
		}

		Camera& camera = cameraEnt.get_mut<Camera>();

		position = mCharacter->GetPosition();

		camera.rotateCamera(offsetX,offsetY);

		glm::vec3 characterPosGLM = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		camera.position = characterPosGLM + glm::vec3(cameraOffset.x, cameraOffset.y, cameraOffset.z);


		//Rotate player's physics body based on the camera Yaw.
		float cameraYaw = glm::radians(camera.yaw);
		rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), cameraYaw);
		mCharacter->SetRotation(rotation);


		camera.updateVectors();

	}


};




