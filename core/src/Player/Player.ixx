module;

#include <string>
#include <format>

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



//TODO create class PlayerContactListener : public JPH::CharacterContactListener 
export class Player : public JPH::CharacterContactListener {


public:

	JPH::TempAllocatorImpl* temp_allocator;

	//Maybe not needed
	//CharacterVsCharacterCollisionSimple mCharacterVsCharacterCollision;

	flecs::world& ecs;

	JPH::Ref<JPH::CharacterVirtual>	mCharacter;
	JPH::Vec3					mDesiredVelocity = JPH::Vec3::sZero();
	JPH::BodyID innerBodyID;
	JPH::Ref<JPH::Shape> bodyShape = new JPH::CapsuleShape(2.0f, 1.0f);

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

	Player(flecs::world& ecs)
		:ecs(ecs)
	{
		//TODO Player can create it own phase here

		timeStep = ecs.get<TimeStep>().step;

		temp_allocator = new JPH::TempAllocatorImpl(1 * 1024 * 1024);

	}

	Player(flecs::world& ecs, JPH::Vec3Arg position, JPH::QuatArg rotation, float height, float radius, uint64_t entityID, bool sCreateInnerBody = false)
		:ecs(ecs)
	{

		temp_allocator = new JPH::TempAllocatorImpl(1 * 1024 * 1024);

		init(position, rotation, height, radius, entityID, sCreateInnerBody);

	}

	~Player() {

		// Clean up custom allocator
		if (temp_allocator != nullptr) {
			delete temp_allocator;
			temp_allocator = nullptr;
		}
	}

	void init(JPH::Vec3Arg position, JPH::QuatArg rotation, float height, float radius, uint64_t entityID, bool sCreateInnerBody = false) {


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
		settings->mInnerBodyShape = sCreateInnerBody ? bodyShape : nullptr;
		settings->mInnerBodyLayer = Layers::MOVING;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		mCharacter = new JPH::CharacterVirtual(settings, position, rotation, entityID, &physicsSystem);
		//mCharacter->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
		//mCharacterVsCharacterCollision.Add(mCharacter);

		innerBodyID = mCharacter->GetInnerBodyID();

		mCharacter->SetListener(this);

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

	void reset() {

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

	void update() {

		if (ecs.get<CameraState>() != CameraState::PLAYER) return;

		getMovementState();

		flecs::entity cameraEnt = ecs.get<PlayerCamRef>().value;
		Camera& camera = cameraEnt.get_mut<Camera>();

		glm::vec3 forward = glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
		glm::vec3 right = glm::normalize(glm::vec3(camera.right.x, 0.0f, camera.right.z));


		input.offsetX;
		input.offsetY;

		forward *= input.direction.y;
		right *= input.direction.x;

		glm::vec3 playerInput = glm::vec3(0);

		playerInput += forward;
		playerInput += right;

		movementDirection.SetX(playerInput.x);
		movementDirection.SetY(playerInput.y);
		movementDirection.SetZ(playerInput.z);

		if (input.jump) {
			//attempts jump if player is grounded.
			mJumpPressed = true;
		}

		UpdateVelocity();
		UpdateCharacter();
		updatePlayerCam();

		shootBall();
	}

	void getMovementState() {

		const ActionState& forwardState = forwardMVMTEnt.get<ActionState>();
		const ActionState& backwardState = backwardMVMTEnt.get<ActionState>();
		const ActionState& leftState = leftMVMTEnt.get<ActionState>();
		const ActionState& rightState = rightMVMTEnt.get<ActionState>();
		const ActionState& jumpState = jumpMVMTEnt.get<ActionState>();

		const MouseMovementState& mouseMovement = ecs.get<MouseMovementState>();

		// Reset each frame before accumulating
		input.direction = glm::vec2(0);
		input.jump = false;

		if (forwardState.occurred) {

			input.direction.y += 1;
		}
		if (backwardState.occurred) {

			input.direction.y -= 1;
		}
		if (leftState.occurred) {

			input.direction.x -= 1;
		}
		if (rightState.occurred) {

			input.direction.x += 1;
		}

		//cannot jump again until jump is consumed,prevents bunny hopping.
		if (jumpState.occurred && input.jumpConsumed) {
			input.jump = true;
			input.jumpConsumed = false;
		}
		if (!jumpState.occurred)
			input.jumpConsumed = true; // ready to jump again

		// Normalize direction to prevent faster diagonal movement
		if (glm::length2(input.direction) > 0.0f) {
			input.direction = glm::normalize(input.direction);
		}

		//TODO parameterize
		float smoothingFactor = 0.7f; // Adjust between 0-1 (lower = smoother)
		static float smoothedXOffset = 0.0f, smoothedYOffset = 0.0f;

		// Apply smoothing
		smoothedXOffset = smoothedXOffset * (1.0f - smoothingFactor) + mouseMovement.deltaX * smoothingFactor;
		smoothedYOffset = smoothedYOffset * (1.0f - smoothingFactor) + mouseMovement.deltaY * smoothingFactor;

		input.offsetX = smoothedXOffset;
		input.offsetY = smoothedYOffset;

	}

	void UpdateVelocity() {
		JPH::CharacterVirtual::EGroundState groundState = mCharacter->GetGroundState();

		if (groundState == JPH::CharacterVirtual::EGroundState::OnGround) {
			// On ground
			mVerticalVelocity = JPH::Vec3::sZero();

			// Jump
			if (mJumpPressed) {
				if (mCharacter->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround) {
					mVerticalVelocity = JPH::Vec3(0, jumpSpeed, 0);
					mJumpPressed = false;  // Consume jump input
				}
			}
		}
		else {
			// In air: Apply gravity manually
			mVerticalVelocity += gravity * timeStep;

			// Clamp to terminal velocity
			if (mVerticalVelocity.GetY() < terminalVelocity) {
				mVerticalVelocity.SetY(terminalVelocity);
			}
		}
	}


	void UpdateCharacter() {

		// Horizontal movement (player controlled)
		JPH::Vec3 horizontalVelocity = movementDirection * moveSpeed;
		horizontalVelocity.SetY(0);  // Keep horizontal only

		// Combine with vertical velocity (gravity/jump)
		JPH::Vec3 totalVelocity = horizontalVelocity + mVerticalVelocity;

		mCharacter->SetLinearVelocity(totalVelocity);

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		const JPH::DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = physicsSystem.GetDefaultBroadPhaseLayerFilter(1);
		const JPH::BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;

		const JPH::DefaultObjectLayerFilter default_object_layer_filter = physicsSystem.GetDefaultLayerFilter(Layers::MOVING);
		const JPH::ObjectLayerFilter& object_layer_filter = default_object_layer_filter;

		const JPH::BodyFilter body_filter;
		const JPH::ShapeFilter shapeFilter;


		mCharacter->Update(timeStep, gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *temp_allocator);

		//mCharacter->ExtendedUpdate(physicsTickRate, gravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *fisiks.temp_allocator);

		position = mCharacter->GetPosition();
		rotation = mCharacter->GetRotation();

	}

	void updatePlayerCam() {

		// TODO keep a ref instead
		flecs::entity cameraEnt = ecs.get<PlayerCamRef>().value;

		if (!cameraEnt) {
			return;
		}

		Camera& camera = cameraEnt.get_mut<Camera>();

		position = mCharacter->GetPosition();

		camera.rotateCamera(input.offsetX, input.offsetY);

		glm::vec3 characterPosGLM = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
		camera.position = characterPosGLM + glm::vec3(cameraOffset.x, cameraOffset.y, cameraOffset.z);


		//Rotate player's physics body based on the camera Yaw.
		float cameraYaw = glm::radians(camera.yaw);
		rotation = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), cameraYaw);
		mCharacter->SetRotation(rotation);


		camera.updateVectors();

	}

	void shootBall() {

		const ActionState& interactEventState = interactEventEnt.get<ActionState>();

		const ecs_world_info_t* info = ecs.get_info();
		int64_t current_frame = info->frame_count_total;

		if (interactEventState.occurred && interactEventState.justPressed) {

			std::string ballName = std::format("Ball {} ", ballCounter);
			ballCounter++;

			Transform ballTransform;

			flecs::entity parent = ecs.get<PlayerRef>().value.parent();

			//LogInfo(LOG_APP, "Interact NEW Event occurred in frame %d", current_frame);
			//LogInfo(LOG_APP, "LastOccurred :  %s", interactEventState.occurredLast ? "true" : "false");
			//LogInfo(LOG_APP, "justPressed :  %s", interactEventState.justPressed ? "true" : "false");
			//LogInfo(LOG_APP, "heldTime :  %f", interactEventState.heldTime);
			//LogInfo(LOG_APP, "---------------------");
			//EntityFactory::createSphereEntity(ecs, parent, ballName, ballTransform);
		}

		const ActionState& atttackEventState = attackEventEnt.get<ActionState>();

		//Single fire
		if (atttackEventState.occurred && atttackEventState.justPressed) {

			std::string ballName = std::format("Ball {} ", ballCounter);
			ballCounter++;

			Transform ballTransform;

			flecs::entity parent = ecs.get<PlayerRef>().value.parent();
			//EntityFactory::createSphereEntity(ecs, parent, ballName, ballTransform);
		}
		// Auto fire
		if (atttackEventState.occurred) {
			//LogInfo(LOG_APP, "heldTime :  %f", atttackEventState.heldTime);
		}

	}
};




