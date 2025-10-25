#pragma once 

#include "renderer/Model.hpp"
#include "renderer/Camera.hpp"

#include "util/serializationHelpers.hpp"

using namespace JPH;

// Forward declare Player
class Player;

// Define conversions before use
void to_json(json& j, const Player& p);
void from_json(const json& j, Player& p);

class Player : public CharacterContactListener {


public:

	//Maybe not needed
	//CharacterVsCharacterCollisionSimple mCharacterVsCharacterCollision;


	flecs::world& ecs;
	Fisiks& fisiks;

	Ref<CharacterVirtual>	mCharacter;
	Vec3					mDesiredVelocity = Vec3::sZero();


	JPH::Vec3 position = JPH::Vec3(1.0f, 15.0f, 0.0f);
	JPH::Quat rotation = JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f);

	// Movement state
	Vec3 mVerticalVelocity = Vec3::sZero();
	float mMoveSpeed = 16.0f;
	float mJumpSpeed = 8.0f;
	float mTerminalVelocity = -50.0f;
	Vec3 mGravity = Vec3(0, -20.0f, 0);

	//FIX FIX FIX
	float physicsTickRate = 1.0f / 120.0f;


	glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 0.0f);


	// Input state
	Vec3 mMoveInput = Vec3::sZero();
	bool mJumpPressed = false;
	
	//how many pixel to rotate the camera
	float offsetX = 0.0f;
	float offsetY = 0.0f;


	Player(flecs::world& ecs, Fisiks& fisiks)
		:ecs(ecs), fisiks(fisiks)
	{

	
	
	}

	~Player() {
		
	}

	void init(JPH::Vec3Arg position,JPH::QuatArg rotation,float height, float raduis, uint64_t entityID) {

		
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
		settings->mShape = new JPH::CapsuleShape(height / 2.0, raduis);
		settings->mBackFaceMode = sBackFaceMode;
		settings->mCharacterPadding = sCharacterPadding;
		settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		settings->mPredictiveContactDistance = sPredictiveContactDistance;

		settings->mSupportingVolume = Plane(Vec3::sAxisY(), -raduis); // Accept contacts that touch the lower sphere of the capsule
		settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
		settings->mInnerBodyShape = sCreateInnerBody ? new JPH::CapsuleShape(height / 2.0, raduis) : nullptr;
		settings->mInnerBodyLayer = Layers::MOVING;

		mCharacter = new CharacterVirtual(settings, position, rotation, entityID, &fisiks.physicsSystem);
		//mCharacter->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
		//mCharacterVsCharacterCollision.Add(mCharacter);


		mCharacter->SetListener(this);
	}

	void reset() {

	}

	bool load(json & j) {

		if (j.contains("player")) {
			j["player"].get_to(*this);  // Calls from_json

			mCharacter->SetPosition(position);
			mCharacter->SetRotation(rotation);

			return true;
		}
		else
		{
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "player is not in save file");
			return false;
		}

	}

	void save(json & j) const {
		j["player"] = *this;  // Calls to_json
		
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


		fisiks.physicsSystem.GetBodyInterface().SetLinearVelocity(inBodyID2, inContactNormal * 10);
		

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
				mVerticalVelocity = Vec3(0, mJumpSpeed, 0);
				mJumpPressed = false;  // Consume jump input
			}
		}
		else {
			// In air: Apply gravity manually
			mVerticalVelocity += mGravity * physicsTickRate;

			// Clamp to terminal velocity
			if (mVerticalVelocity.GetY() < mTerminalVelocity) {
				mVerticalVelocity.SetY(mTerminalVelocity);
			}
		}
	}

	void PrePhysicsUpdate() {

		UpdateVelocity();
		UpdateCharacter();
		updatePlayerCam();

		renderCharacter();
	}

	void UpdateCharacter() {

		// Horizontal movement (player controlled)
		Vec3 horizontalVelocity = mMoveInput * mMoveSpeed;
		horizontalVelocity.SetY(0);  // Keep horizontal only

		// Combine with vertical velocity (gravity/jump)
		Vec3 totalVelocity = horizontalVelocity + mVerticalVelocity;

		mCharacter->SetLinearVelocity(totalVelocity);

		const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = fisiks.physicsSystem.GetDefaultBroadPhaseLayerFilter(1);
		const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;

		const DefaultObjectLayerFilter default_object_layer_filter = fisiks.physicsSystem.GetDefaultLayerFilter(1);
		const ObjectLayerFilter& object_layer_filter = default_object_layer_filter;

		const BodyFilter body_filter;
		const ShapeFilter shapeFilter;

		mCharacter->Update(physicsTickRate, mGravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *fisiks.temp_allocator);

		//mCharacter->ExtendedUpdate(physicsTickRate, mGravity, broadphase_layer_filter, object_layer_filter, body_filter, shapeFilter, *fisiks.temp_allocator);

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
		Camera& camera = ecs.lookup("PlayerCam").get_mut<Camera>();

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

	void renderCharacter() {

#ifdef JPH_DEBUG_RENDERER

		fisiksDebugRenderer& fisiksRenderer = ecs.get_mut<fisiksDebugRenderer>();

		RMat44 com = mCharacter->GetCenterOfMassTransform();

		mCharacter->GetShape()->Draw(& fisiksRenderer, com, Vec3::sOne(), Color::sWhite, false, true);

#endif

	}

};

// serialization

inline void to_json(json& j, const Vec3& v) {
	j = json{ v.GetX(), v.GetY(), v.GetZ() };
}
inline void from_json(const json& j, Vec3& v) {
	if (j.is_array() && j.size() >= 3)
		v = Vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}


void to_json(json& j, const Player& p) {
	j = json{
		{"position", {p.position.GetX(), p.position.GetY(), p.position.GetZ()}},
		{"rotation", {p.rotation.GetX(), p.rotation.GetY(), p.rotation.GetZ(), p.rotation.GetW()}},
		{"moveSpeed", p.mMoveSpeed},
		{"jumpSpeed", p.mJumpSpeed},
		{"terminalVelocity", p.mTerminalVelocity},
		{"cameraOffset", {p.cameraOffset.x, p.cameraOffset.y, p.cameraOffset.z}},

	};
}

void from_json(const json& j, Player& p) {
	p.position = j["position"].get<JPH::Vec3>();
	p.rotation = j["rotation"].get<JPH::Quat>();
	p.mMoveSpeed = j.value("moveSpeed", p.mMoveSpeed);
	p.mJumpSpeed = j.value("jumpSpeed", p.mJumpSpeed);
	p.mTerminalVelocity = j.value("terminalVelocity", p.mTerminalVelocity);


	if (j.contains("cameraOffset"))
		p.cameraOffset = j["cameraOffset"].get<glm::vec3>();
}

