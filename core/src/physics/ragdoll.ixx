module;

#include <vector>
#include <iostream>
#include <cstdint>
#include <queue>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif

export module Ragdoll;


import Logger;
import Jolt;
import PhysicsComponents;
import JoltAssetStream;

namespace JPH {
	class RagdollSettings;
	enum class EMotionType : uint8_t;
}

export enum class Attachment
{
	Top,        // +Y
	Bottom,     // -Y
	Left,       // -X
	Right,      // +X
	Front,      // +Z 
	Back,       // -Z
	None
};



export class RagdollLoader {

public:

	// Loads a ragdoll from a .tof object stream and optionally applies a uniform
	// runtime scale. Scaling multiplies each part's world position, wraps its
	// collision shape in a ScaledShape, and scales the SwingTwist constraint pivot
	// positions. Total mass is held at the unscaled value rather than following
	// volume, so a scaled ragdoll weighs the same as it would at scale 1.
	// Rotations, constraint axes and angles are left untouched.
	static JPH::RagdollSettings* load(const char* inFileName, JPH::EMotionType inMotionType, float scale = 1.0f)
	{
		// Read the ragdoll
		JPH::RagdollSettings* ragdoll = nullptr;
		AssetStream stream(inFileName, std::ios::in);
		if (!JPH::ObjectStreamIn::sReadObject(stream.Get(), ragdoll)) {
			LogError(LOG_PHYSICS, "failed reading in ragdoll data in file %s", inFileName);
			return ragdoll;
		}

		for (JPH::RagdollSettings::Part& p : ragdoll->mParts)
		{

			// Update motion PipelineType
			p.mMotionType = inMotionType;
			// Override layer
			p.mObjectLayer = Layers::MOVING;

			if (scale != 1.0f)
			{
				// Scale the part's world position
				p.mPosition *= scale;

				// Wrap the collision shape so its dimensions scale uniformly.
				// GetShape() realizes the deserialized ShapeSettings into a
				// runtime Shape; ScaledShape applies the factor at collide/cast
				// time.
				const float unscaledMass = p.GetMassProperties().mMass;

				p.SetShape(new JPH::ScaledShape(p.GetShape(), JPH::Vec3::sReplicate(scale)));

				// ScaledShape holds density constant, so mass would follow volume (27x
				// at 3x). Pin each part back to its unscaled mass. CalculateInertia
				// derives the tensor from the scaled geometry then rescales it to this
				// mass, which is the correct inertia for a body of this mass at this
				// size. Stabilize() below redistributes mass within a chain but
				// preserves the chain total, so this survives it.
				p.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
				p.mMassPropertiesOverride.mMass = unscaledMass;

				// Scale the parent-constraint pivot positions (world space).
				// SwingTwist is the only constraint subtype used by these ragdolls;
				// its mPosition1/mPosition2 are world-space pivots that must track
				// the scaled part positions. Axes and limit angles stay unchanged.
				if (p.mToParent)
				{
					//JPH_ASSERT(dynamic_cast<JPH::SwingTwistConstraintSettings*>(p.mToParent.GetPtr()) != nullptr);
					JPH::SwingTwistConstraintSettings* st =
						static_cast<JPH::SwingTwistConstraintSettings*>(p.mToParent.GetPtr());
					st->mPosition1 *= scale;
					st->mPosition2 *= scale;

					/*
					float MaxTorque = 5000000.0f;
					st->mSwingMotorSettings.mMaxTorqueLimit = MaxTorque;
					st->mSwingMotorSettings.mMinTorqueLimit = -MaxTorque;
					st->mTwistMotorSettings.mMaxTorqueLimit = MaxTorque;
					st->mTwistMotorSettings.mMinTorqueLimit = -MaxTorque;
					*/

				}
			}
		}

		// Initialize the skeleton
		ragdoll->GetSkeleton()->CalculateParentJointIndices();

		// Stabilize the constraints of the ragdoll
		ragdoll->Stabilize();


		ragdoll->DisableParentChildCollisions();

		ragdoll->CalculateConstraintPriorities();

		// Calculate body <-> constraint map
		ragdoll->CalculateBodyIndexToConstraintIndex();
		ragdoll->CalculateConstraintIndexToBodyIdxPair();

		return ragdoll;

	}

	static JPH::Quat CalculateRotationToVector(JPH::Vec3 inVector)
	{
		if (inVector.LengthSq() == 0) return JPH::Quat::sIdentity();
		inVector = inVector.Normalized();

		// Capsules are Y-aligned by default in Jolt
		return JPH::Quat::sFromTo(JPH::Vec3::sAxisY(), inVector);
	}

	static JPH::RagdollSettings* create(float scale = 1.0f)
	{
		// Create skeleton
		JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;
		uint32_t lower_body = skeleton->AddJoint("LowerBody");
		uint32_t mid_body = skeleton->AddJoint("MidBody", lower_body);
		uint32_t upper_body = skeleton->AddJoint("UpperBody", mid_body);
		uint32_t head = skeleton->AddJoint("Head", upper_body);
		uint32_t upper_arm_l = skeleton->AddJoint("UpperArmL", upper_body);
		uint32_t upper_arm_r = skeleton->AddJoint("UpperArmR", upper_body);
		uint32_t lower_arm_l = skeleton->AddJoint("LowerArmL", upper_arm_l);
		uint32_t lower_arm_r = skeleton->AddJoint("LowerArmR", upper_arm_r);
		uint32_t upper_leg_l = skeleton->AddJoint("UpperLegL", lower_body);
		uint32_t upper_leg_r = skeleton->AddJoint("UpperLegR", lower_body);
		uint32_t lower_leg_l = skeleton->AddJoint("LowerLegL", upper_leg_l);
		uint32_t lower_leg_r = skeleton->AddJoint("LowerLegR", upper_leg_r);

		uint32_t foot_l = skeleton->AddJoint("FootL", lower_leg_l);
		uint32_t foot_R = skeleton->AddJoint("FootR", lower_leg_r);

		// Create shapes for limbs (scaled)
		JPH::Ref<JPH::Shape> shapes[] = {
			new JPH::CapsuleShape(0.15f * scale, 0.10f * scale),
			new JPH::CapsuleShape(0.15f * scale, 0.10f * scale),		// Mid Body
			new JPH::CapsuleShape(0.15f * scale, 0.10f * scale),		// Upper Body
			new JPH::CapsuleShape(0.075f * scale, 0.10f * scale),	// Head
			new JPH::CapsuleShape(0.15f * scale, 0.06f * scale),		// Upper Arm L
			new JPH::CapsuleShape(0.15f * scale, 0.06f * scale),		// Upper Arm R
			new JPH::CapsuleShape(0.15f * scale, 0.05f * scale),		// Lower Arm L
			new JPH::CapsuleShape(0.15f * scale, 0.05f * scale),		// Lower Arm R
			new JPH::CapsuleShape(0.2f * scale, 0.075f * scale),		// Upper Leg L
			new JPH::CapsuleShape(0.2f * scale, 0.075f * scale),		// Upper Leg R
			new JPH::CapsuleShape(0.2f * scale, 0.06f * scale),		// Lower Leg L
			new JPH::CapsuleShape(0.2f * scale, 0.06f * scale),		// Lower Leg R

			new JPH::BoxShape(JPH::Vec3(0.0444f, 0.0361f, 0.1125f) * scale,0.00361290015f),		// LEFT FOOT
			new JPH::BoxShape(JPH::Vec3(0.0444f, 0.0361f, 0.1125f) * scale,0.00361290015f),		// RIGHT FOOT
		};

		// Positions of body parts in world space (scaled)
		JPH::RVec3 positions[] = {
			JPH::RVec3(0, 1.15f * scale, 0),					// Lower Body
			JPH::RVec3(0, 1.35f * scale, 0),					// Mid Body
			JPH::RVec3(0, 1.55f * scale, 0),					// Upper Body
			JPH::RVec3(0, 1.825f * scale, 0),				// Head
			JPH::RVec3(-0.425f * scale, 1.55f * scale, 0),	// Upper Arm L
			JPH::RVec3(0.425f * scale, 1.55f * scale, 0),	// Upper Arm R
			JPH::RVec3(-0.8f * scale, 1.55f * scale, 0),		// Lower Arm L
			JPH::RVec3(0.8f * scale, 1.55f * scale, 0),		// Lower Arm R
			JPH::RVec3(-0.15f * scale, 0.8f * scale, 0),		// Upper Leg L
			JPH::RVec3(0.15f * scale, 0.8f * scale, 0),		// Upper Leg R
			JPH::RVec3(-0.15f * scale, 0.3f * scale, 0),		// Lower Leg L
			JPH::RVec3(0.15f * scale, 0.3f * scale, 0),		// Lower Leg R
			JPH::RVec3(0.145158f * scale, 0.083797f * scale, 0.0027870f * scale),
			JPH::RVec3(-0.145157f * scale, 0.083798f * scale, 0.0027870f * scale),
		};

		// Rotations of body parts in world space (unchanged)
		JPH::Quat rotations[] = {
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Body
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Mid Body
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Body
			JPH::Quat::sIdentity(),									 // Head
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Arm L
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Upper Arm R
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Arm L
			JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),		 // Lower Arm R
			JPH::Quat::sIdentity(),									 // Upper Leg L
			JPH::Quat::sIdentity(),									 // Upper Leg R
			JPH::Quat::sIdentity(),									 // Lower Leg L
			JPH::Quat::sIdentity(),									 // Lower Leg R
			JPH::Quat(0, -0.506379f, 0.862311f, 0),
			JPH::Quat(0, 0.862311f, 0.506379f, 0),
		};

		// World space constraint positions (scaled)
		JPH::RVec3 constraint_positions[] = {
			JPH::RVec3::sZero(),								// Lower Body (unused, there's no parent)
			JPH::RVec3(0, 1.25f * scale, 0),					// Mid Body
			JPH::RVec3(0, 1.45f * scale, 0),					// Upper Body
			JPH::RVec3(0, 1.65f * scale, 0),					// Head
			JPH::RVec3(-0.225f * scale, 1.55f * scale, 0),	// Upper Arm L
			JPH::RVec3(0.225f * scale, 1.55f * scale, 0),	// Upper Arm R
			JPH::RVec3(-0.65f * scale, 1.55f * scale, 0),	// Lower Arm L
			JPH::RVec3(0.65f * scale, 1.55f * scale, 0),		// Lower Arm R
			JPH::RVec3(-0.15f * scale, 1.05f * scale, 0),	// Upper Leg L
			JPH::RVec3(0.15f * scale, 1.05f * scale, 0),		// Upper Leg R
			JPH::RVec3(-0.15f * scale, 0.55f * scale, 0),	// Lower Leg L
			JPH::RVec3(0.15f * scale, 0.55f * scale, 0),		// Lower Leg R
			JPH::RVec3(0.145158f * scale, 0.083797f * scale, 0.0027870f * scale),			// 3: R Foot
			JPH::RVec3(-0.145157f * scale, 0.083798f * scale, 0.0027870f * scale),			// 6: L Foot
		};

		// World space twist axis directions (unchanged - these are normalized directions)
		JPH::Vec3 twist_axis[] = {
			JPH::Vec3::sZero(),				// Lower Body (unused, there's no parent)
			JPH::Vec3::sAxisY(),				// Mid Body
			JPH::Vec3::sAxisY(),				// Upper Body
			JPH::Vec3::sAxisY(),				// Head
			-JPH::Vec3::sAxisX(),			// Upper Arm L
			JPH::Vec3::sAxisX(),				// Upper Arm R
			-JPH::Vec3::sAxisX(),			// Lower Arm L
			JPH::Vec3::sAxisX(),				// Lower Arm R
			-JPH::Vec3::sAxisY(),			// Upper Leg L
			-JPH::Vec3::sAxisY(),			// Upper Leg R
			-JPH::Vec3::sAxisY(),			// Lower Leg L
			-JPH::Vec3::sAxisY(),			// Lower Leg R
			-JPH::Vec3::sAxisY(),			// Lower Leg L
			-JPH::Vec3::sAxisY(),			// Lower Leg R
		};

		// Constraint limits (unchanged - these are angles)
		float twist_angle[] = {
			0.0f,		// Lower Body (unused, there's no parent)
			5.0f,		// Mid Body
			5.0f,		// Upper Body
			90.0f,		// Head
			45.0f,		// Upper Arm L
			45.0f,		// Upper Arm R
			45.0f,		// Lower Arm L
			45.0f,		// Lower Arm R
			45.0f,		// Upper Leg L
			45.0f,		// Upper Leg R
			45.0f,		// Lower Leg L
			45.0f,		// Lower Leg R
			45.1f,		// R Foot
			45.1f,		// R Foot
		};

		float normal_angle[] = {
			0.0f,		// Lower Body (unused, there's no parent)
			10.0f,		// Mid Body
			10.0f,		// Upper Body
			45.0f,		// Head
			90.0f,		// Upper Arm L
			90.0f,		// Upper Arm R
			0.0f,		// Lower Arm L
			0.0f,		// Lower Arm R
			45.0f,		// Upper Leg L
			45.0f,		// Upper Leg R
			0.0f,		// Lower Leg L
			0.0f,		// Lower Leg R
			59.6f,		// R Foot
			59.6f,		// R Foot
		};

		float plane_angle[] = {
			0.0f,		// Lower Body (unused, there's no parent)
			10.0f,		// Mid Body
			10.0f,		// Upper Body
			45.0f,		// Head
			45.0f,		// Upper Arm L
			45.0f,		// Upper Arm R
			90.0f,		// Lower Arm L
			90.0f,		// Lower Arm R
			45.0f,		// Upper Leg L
			45.0f,		// Upper Leg R
			60.0f,		// Lower Leg L (cheating here, a knee is not symmetric, we should have rotated the twist axis)
			60.0f,		// Lower Leg R
			28.5f,		// R Foot
			28.5f,		// R Foot
		};

		// Create ragdoll settings
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			JPH::RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = JPH::EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{
				JPH::SwingTwistConstraintSettings* constraint = new JPH::SwingTwistConstraintSettings;
				constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = JPH::Vec3::sAxisZ();
				constraint->mTwistMinAngle = -JPH::DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = JPH::DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = JPH::DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = JPH::DegreesToRadians(plane_angle[p]);
				part.mToParent = constraint;
			}
		}

		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;
	}

	static JPH::RagdollSettings* createArm(JPH::RVec3 position, float scale = 1.0f) {

		// Create skeleton
		//Jolts skeleton is a vector of joints, each joint know its name, parentName and ParentIndex
		JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;

		uint32_t base = skeleton->AddJoint("Base");
		uint32_t upperArm = skeleton->AddJoint("UpperArm", base);
		uint32_t lowerArm = skeleton->AddJoint("LowerArm", upperArm);


		// Create shapes
		JPH::Ref<JPH::Shape> baseShape = new JPH::BoxShape(JPH::Vec3(1.5f, 1.5f, 1.5f) * scale);
		JPH::Ref<JPH::Shape> upperArmShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> lowerArmShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);


		JPH::Ref<JPH::Shape> shapes[] = { baseShape, upperArmShape,lowerArmShape };

		// Calculate positions automatically
		JPH::Vec3 basePos = position;
		JPH::Vec3 upperArmPos = ShapePlacementHelper::PlaceOnTop(baseShape, basePos, JPH::Quat::sIdentity(),
			upperArmShape, JPH::Quat::sIdentity());
		JPH::Vec3 lowerArmPos = ShapePlacementHelper::PlaceOnTop(upperArmShape, upperArmPos, JPH::Quat::sIdentity(),
			lowerArmShape, JPH::Quat::sIdentity());



		JPH::RVec3 positions[] = { basePos, upperArmPos,lowerArmPos };
		JPH::Quat rotations[] = { JPH::Quat::sIdentity(), JPH::Quat::sIdentity(),JPH::Quat::sIdentity() };


		// Calculate constraint positions automatically
		JPH::RVec3 constraint_positions[] = {
			JPH::RVec3::sZero(),  // Base has no parent
			ShapePlacementHelper::GetConstraintPosition(baseShape, basePos, rotations[0],
														 upperArmShape, upperArmPos, rotations[1]),
			ShapePlacementHelper::GetConstraintPosition(upperArmShape, upperArmPos, rotations[1],
													 lowerArmShape, lowerArmPos, rotations[2]),
		};


		// Create ragdoll settings
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());


		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			JPH::RagdollSettings::Part& part = settings->mParts[p];

			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = JPH::EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;


			if (p == 0) {

				part.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
				part.mMassPropertiesOverride.mMass = 5000.1f;

			}

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{
				//Fixed consraint for now
				if (p == 1) {

					JPH::FixedConstraintSettings* constraint = new JPH::FixedConstraintSettings;

					constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization

					constraint->mPoint1 = constraint->mPoint2 = constraint_positions[p];

					part.mToParent = constraint;
				}
				if (p == 2) {
					JPH::HingeConstraintSettings* constraint = new JPH::HingeConstraintSettings;

					constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization

					constraint->mPoint1 = constraint->mPoint2 = constraint_positions[p];
					constraint->mHingeAxis1 = constraint->mHingeAxis2 = JPH::Vec3::sAxisX();
					constraint->mNormalAxis1 = constraint->mNormalAxis2 = JPH::Vec3::sAxisY();

					constraint->mLimitsMin = JPH::DegreesToRadians(0.0f);
					constraint->mLimitsMax = JPH::DegreesToRadians(120.0f);

					// Configure motor settings
					constraint->mMaxFrictionTorque = 0.0f;  // Disable friction when motor is active

					part.mToParent = constraint;
				}


			}
		}

		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;

	}

	static JPH::RagdollSettings* createSnake(JPH::RVec3 position, float scale = 1.0f) {

		JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;

		uint32_t head = skeleton->AddJoint("Head");
		uint32_t middlePiece1 = skeleton->AddJoint("MiddlePiece1", head);
		uint32_t middlePiece2 = skeleton->AddJoint("MiddlePiece2", middlePiece1);
		uint32_t middlePiece3 = skeleton->AddJoint("MiddlePiece3", middlePiece2);
		uint32_t tail = skeleton->AddJoint("Tail", middlePiece3);

		// Create shapes
		//Ref<Shape> rootShape = new SphereShape(1.0f);

		JPH::Ref<JPH::Shape> headShape = new JPH::CapsuleShape(1.0f * scale, 0.35f * scale);
		JPH::Ref<JPH::Shape> middlePiece1Shape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> middlePiece2Shape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> middlePiece3Shape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> tailPieceShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);

		JPH::Ref<JPH::Shape> shapes[] = { headShape, middlePiece1Shape, middlePiece2Shape, middlePiece3Shape, tailPieceShape };

		// Calculate positions automatically

		//Vec3 rootPos = position;

		JPH::Vec3 headPos = position;

		JPH::Vec3 middlePiece1Pos = ShapePlacementHelper::PlaceOnTop(headShape, headPos, JPH::Quat::sIdentity(),
			middlePiece1Shape, JPH::Quat::sIdentity());

		JPH::Vec3 middlePiece2Pos = ShapePlacementHelper::PlaceOnTop(middlePiece1Shape, middlePiece1Pos, JPH::Quat::sIdentity(),
			middlePiece2Shape, JPH::Quat::sIdentity());

		JPH::Vec3 middlePiece3Pos = ShapePlacementHelper::PlaceOnTop(middlePiece2Shape, middlePiece2Pos, JPH::Quat::sIdentity(),
			middlePiece3Shape, JPH::Quat::sIdentity());

		JPH::Vec3 tailPiecePos = ShapePlacementHelper::PlaceOnTop(middlePiece3Shape, middlePiece3Pos, JPH::Quat::sIdentity(),
			tailPieceShape, JPH::Quat::sIdentity());

		JPH::RVec3 positions[] = { headPos, middlePiece1Pos, middlePiece2Pos, middlePiece3Pos, tailPiecePos };
		JPH::Quat rotations[] = { JPH::Quat::sIdentity(), JPH::Quat::sIdentity(), JPH::Quat::sIdentity(), JPH::Quat::sIdentity(),JPH::Quat::sIdentity() };


		JPH::RVec3 constraint_positions[] = {
	JPH::RVec3::sZero(),  //

	ShapePlacementHelper::GetConstraintPosition(headShape, headPos, rotations[0],
												 middlePiece1Shape, middlePiece1Pos, rotations[1]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece1Shape, middlePiece1Pos, rotations[1],
												 middlePiece2Shape, middlePiece2Pos, rotations[2]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece2Shape, middlePiece2Pos, rotations[2],
												 middlePiece3Shape, middlePiece3Pos, rotations[3]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece3Shape, middlePiece3Pos, rotations[3],
												 tailPieceShape, tailPiecePos, rotations[4]),
		};

		JPH::Vec3 twist_axis[] = {
		JPH::Vec3::sZero(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		};

		// Constraint limits how much it can rotate around the twist axis
		float twist_angle[] = {
			0.0f,		// base
			0.0f,
			0.0f,
			0.0f,
			0.0f,
		};

		float normal_angle[] = {
			0.0f,		// base
			150.0f,
			150.0f,
			150.0f,
			150.0f,
		};

		float plane_angle[] = {
			0.0f,		// base
			150.0f,
			150.0f,
			150.0f,
			150.0f,
		};


		// Create ragdoll settings
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			JPH::RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = JPH::EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{

				JPH::SwingTwistConstraintSettings* constraint = new JPH::SwingTwistConstraintSettings;
				constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = JPH::Vec3::sAxisZ();
				constraint->mTwistMinAngle = -JPH::DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = JPH::DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = JPH::DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = JPH::DegreesToRadians(plane_angle[p]);

				part.mToParent = constraint;

			}
		}


		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;

	}

	static JPH::RagdollSettings* createHumanoid(JPH::RVec3 position, float scale = 1.0f) {

		JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;

		uint32_t hips = skeleton->AddJoint("hips");

		uint32_t leftHipJoint = skeleton->AddJoint("LeftHipJoint", hips);
		uint32_t leftUpperLeg = skeleton->AddJoint("LeftUpperLeg", leftHipJoint);
		uint32_t leftLowerLeg = skeleton->AddJoint("LeftLowerLeg", leftUpperLeg);
		uint32_t leftFoot = skeleton->AddJoint("LeftFoot", leftLowerLeg);

		uint32_t rightHipJoint = skeleton->AddJoint("RightHipJoint", hips);
		uint32_t rightUpperLeg = skeleton->AddJoint("RightUpperLeg", rightHipJoint);
		uint32_t rightLowerLeg = skeleton->AddJoint("RightLowerLeg", rightUpperLeg);
		uint32_t rightFoot = skeleton->AddJoint("RightFoot", rightLowerLeg);


		JPH::Ref<JPH::Shape> hipShape = new JPH::CapsuleShape(0.5f * scale, 0.35f * scale);

		JPH::Ref<JPH::Shape> leftHipJointShape = new JPH::SphereShape(0.2f * scale);
		JPH::Ref<JPH::Shape> leftUpperLegShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> leftLowerLegShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> leftFootShape = new JPH::BoxShape(JPH::Vec3(0.1f, 0.1f, 0.1f) * scale);

		JPH::Ref<JPH::Shape> rightHipJointShape = new JPH::SphereShape(0.2f * scale);
		JPH::Ref<JPH::Shape> rightUpperLegShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> rightLowerLegShape = new JPH::CapsuleShape(1.0f * scale, 0.25f * scale);
		JPH::Ref<JPH::Shape> rightFootShape = new JPH::BoxShape(JPH::Vec3(0.1f, 0.1f, 0.1f) * scale);

		JPH::Ref<JPH::Shape> shapes[] = { hipShape,
			leftHipJointShape,leftUpperLegShape,leftLowerLegShape,leftFootShape,
			rightHipJointShape,rightUpperLegShape, rightLowerLegShape, rightFootShape

		};

		JPH::Quat rotations[] = { JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), 0.5f * JPH::JPH_PI),

			JPH::Quat::sIdentity(),JPH::Quat::sIdentity(),JPH::Quat::sIdentity(),JPH::Quat::sIdentity(),

			JPH::Quat::sIdentity(),JPH::Quat::sIdentity(),JPH::Quat::sIdentity(),JPH::Quat::sIdentity()
		};


		JPH::Vec3 hipPos = position;

		JPH::Vec3 leftHipJointPos = ShapePlacementHelper::PlaceOnLeft(hipShape, hipPos, rotations[0],
			leftHipJointShape, JPH::Quat::sIdentity());

		JPH::Vec3 leftUpperLegPos = ShapePlacementHelper::PlaceOnBottom(leftHipJointShape, leftHipJointPos, JPH::Quat::sIdentity(),
			leftUpperLegShape, JPH::Quat::sIdentity());

		JPH::Vec3 leftLowerLegPos = ShapePlacementHelper::PlaceOnBottom(leftUpperLegShape, leftUpperLegPos, JPH::Quat::sIdentity(),
			leftLowerLegShape, JPH::Quat::sIdentity());

		JPH::Vec3 leftFootPos = ShapePlacementHelper::PlaceOnBottom(leftLowerLegShape, leftLowerLegPos, JPH::Quat::sIdentity(),
			leftFootShape, JPH::Quat::sIdentity());



		JPH::Vec3 rightHipJointPos = ShapePlacementHelper::PlaceOnRight(hipShape, hipPos, rotations[0],
			rightHipJointShape, JPH::Quat::sIdentity());

		JPH::Vec3 rightUpperLegPos = ShapePlacementHelper::PlaceOnBottomRight(rightHipJointShape, rightHipJointPos, rotations[0],
			rightUpperLegShape, JPH::Quat::sIdentity());

		JPH::Vec3 rightLowerLegPos = ShapePlacementHelper::PlaceOnBottom(rightUpperLegShape, rightUpperLegPos, JPH::Quat::sIdentity(),
			rightLowerLegShape, JPH::Quat::sIdentity());

		JPH::Vec3 rightFootPos = ShapePlacementHelper::PlaceOnBottom(rightLowerLegShape, rightLowerLegPos, JPH::Quat::sIdentity(),
			rightFootShape, JPH::Quat::sIdentity());

		JPH::RVec3 positions[] = { hipPos,
			leftHipJointPos, leftUpperLegPos, leftLowerLegPos, leftFootPos,
			rightHipJointPos, rightUpperLegPos, rightLowerLegPos, rightFootPos };

		JPH::RVec3 constraint_positions[] = {
			JPH::RVec3::sZero(),

			ShapePlacementHelper::placeConstraintLeft(hipShape, hipPos, rotations[0],
														 leftHipJointShape, leftHipJointPos, rotations[1]),

			ShapePlacementHelper::placeConstraintBottom(leftHipJointShape, leftHipJointPos, rotations[1],
														 leftUpperLegShape, leftUpperLegPos, rotations[2]),

			ShapePlacementHelper::placeConstraintBottom(leftUpperLegShape, leftUpperLegPos, rotations[2],
														 leftLowerLegShape, leftLowerLegPos, rotations[3]),

			ShapePlacementHelper::placeConstraintBottom(leftLowerLegShape, leftLowerLegPos, rotations[3],
														 leftFootShape, leftFootPos, rotations[4]),


			ShapePlacementHelper::placeConstraintRight(hipShape, hipPos, rotations[0],
														 rightHipJointShape, rightHipJointPos, rotations[5]),

			ShapePlacementHelper::placeConstraintBottom(rightHipJointShape, rightHipJointPos, rotations[5],
														 rightUpperLegShape, rightUpperLegPos, rotations[6]),

			ShapePlacementHelper::placeConstraintBottom(rightUpperLegShape, rightUpperLegPos, rotations[6],
														rightLowerLegShape, rightLowerLegPos, rotations[7]),

			ShapePlacementHelper::placeConstraintBottom(rightLowerLegShape, rightLowerLegPos, rotations[7],
														rightFootShape, rightFootPos, rotations[8]),

		};

		JPH::Vec3 twist_axis[] = {
		JPH::Vec3::sZero(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		JPH::Vec3::sAxisY(),
		};

		// Constraint limits how much it can rotate around the twist axis
		float twist_angle[] = {
			0.0f,		// base
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			0.0f,
		};

		float normal_angle[] = {
			0.0f,		// base
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
		};

		float plane_angle[] = {
			0.0f,		// base
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
			150.0f,
		};

		// Create ragdoll settings
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{

			JPH::RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = JPH::EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{


				JPH::SwingTwistConstraintSettings* constraint = new JPH::SwingTwistConstraintSettings;
				//constraint->mDrawConstraintSize = ;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = JPH::Vec3::sAxisZ();
				constraint->mTwistMinAngle = -JPH::DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = JPH::DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = JPH::DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = JPH::DegreesToRadians(plane_angle[p]);

				part.mToParent = constraint;

			}
		}


		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;
	}


	struct ShapePlacementHelper {

		// Get the top position of a shape in world space
		static JPH::Vec3 GetTopPosition(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {
			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(currentPosition.GetX(), bounds.mMax.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static JPH::Vec3 GetBottomPosition(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {

			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(currentPosition.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static JPH::Vec3 GetRightPosition(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {

			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(bounds.mMax.GetX(), currentPosition.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static JPH::Vec3 GetLeftPosition(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {

			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(bounds.mMin.GetX(), currentPosition.GetY(), currentPosition.GetZ());
		}

		static JPH::Vec3 GetBottomRightPos(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {

			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(bounds.mMax.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		static JPH::Vec3 GetBottomLeftPos(const JPH::Shape* shape, JPH::Vec3 currentPosition, JPH::Quat rotation = JPH::Quat::sIdentity()) {

			JPH::Mat44 transform = JPH::Mat44::sRotationTranslation(rotation, currentPosition);
			JPH::AABox bounds = shape->GetWorldSpaceBounds(transform, JPH::Vec3::sReplicate(1.0f));
			return JPH::Vec3(bounds.mMin.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		// Place a shape on top of another shape
		static JPH::Vec3 PlaceOnTop(const JPH::Shape* baseShape, JPH::Vec3 basePosition, JPH::Quat baseRotation,
			const JPH::Shape* newShape, JPH::Quat newRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {
			// Get top of base shape
			JPH::Vec3 topOfBase = GetTopPosition(baseShape, basePosition, baseRotation);

			// Get how far the new shape's center is from its bottom
			JPH::Mat44 newTransform = JPH::Mat44::sRotationTranslation(newRotation, JPH::Vec3::sZero());
			JPH::AABox newBounds = newShape->GetLocalBounds();
			float newShapeCenterToBottom = -newBounds.mMin.GetY();

			// Place new shape so its bottom aligns with top of base
			return JPH::Vec3(basePosition.GetX(),
				topOfBase.GetY() + newShapeCenterToBottom + gap,
				basePosition.GetZ());
		}

		static JPH::Vec3 PlaceOnBottom(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Quat childRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			JPH::Vec3 bottomOfBase = GetBottomPosition(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTransform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			JPH::AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return JPH::Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		static JPH::Vec3 PlaceOnRight(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Quat childRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			JPH::Vec3 rightOfBase = GetRightPosition(parentShape, parentPos, parentRot);

			return JPH::Vec3(rightOfBase.GetX(), rightOfBase.GetY() - gap, rightOfBase.GetZ());

		}

		static JPH::Vec3 PlaceOnLeft(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Quat childRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			JPH::Vec3 leftofBase = GetLeftPosition(parentShape, parentPos, parentRot);

			return JPH::Vec3(leftofBase.GetX(), leftofBase.GetY() + gap, leftofBase.GetZ());

		}


		static JPH::Vec3 PlaceOnBottomRight(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Quat childRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			JPH::Vec3 bottomOfBase = GetBottomRightPos(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTransform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			JPH::AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return JPH::Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		static JPH::Vec3 PlaceOnBottomLeft(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Quat childRotation = JPH::Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			JPH::Vec3 bottomOfBase = GetBottomLeftPos(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTramsform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			JPH::AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return JPH::Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		// Get the constraint position (midpoint between two connected bodies)
		static JPH::Vec3 GetConstraintPosition(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Vec3 childPos, JPH::Quat childRot) {
			JPH::Vec3 parentTop = GetTopPosition(parentShape, parentPos, parentRot);
			JPH::Vec3 childBottom = GetBottomPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentTop + childBottom) * 0.5f;
		}

		// Get the constraint position (midpoint between two connected bodies)
		static JPH::Vec3 constraintPosLeftSide(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Vec3 childPos, JPH::Quat childRot) {
			JPH::Vec3 parentTop = GetTopPosition(parentShape, parentPos, parentRot);
			JPH::Vec3 childBottom = GetBottomPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentTop + childBottom) * 0.5f;
		}

		static JPH::Vec3 placeConstraintBottom(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Vec3 childPos, JPH::Quat childRot) {
			JPH::Vec3 parentBottom = GetBottomPosition(parentShape, parentPos, parentRot);
			JPH::Vec3 childTop = GetTopPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentBottom + childTop) * 0.5f;
		}

		static JPH::Vec3 placeConstraintRight(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Vec3 childPos, JPH::Quat childRot) {
			JPH::Vec3 parentRight = GetRightPosition(parentShape, parentPos, parentRot);

			return (parentRight) * 0.5f;
		}

		static JPH::Vec3 placeConstraintLeft(const JPH::Shape* parentShape, JPH::Vec3 parentPos, JPH::Quat parentRot,
			const JPH::Shape* childShape, JPH::Vec3 childPos, JPH::Quat childRot) {
			JPH::Vec3 parentLeft = GetLeftPosition(parentShape, parentPos, parentRot);


			// Constraint at midpoint
			return (parentLeft) * 0.5f;
		}
	};
};

export struct BodyPart {

	std::string name;
	uint32_t skeletonJointIndex;
	JPH::Ref<JPH::Shape> shape;
	JPH::Body* bodyPtr = nullptr;
	BodyPart* parent = nullptr;
	Attachment attachmentPrimaryAxis;
	Attachment attachmentSecondaryAxis;
	Attachment attachmentTertiaryAxis;
	std::vector<BodyPart*> children;
	JPH::Ref<JPH::TwoBodyConstraintSettings> constraintSettings;
	JPH::EConstraintSubType constraintType;
	JPH::Ref<JPH::TwoBodyConstraint> constraint;
};


export namespace ragdoll {

	export uint32_t getRagdollSize(BodyPart* root) {
		if (root == nullptr) {
			return 0;  // Empty subtree has size 0
		}

		uint32_t size = 1;  // Count current node

		for (int i = 0; i < root->children.size(); i++) {
			size += getRagdollSize(root->children[i]);
		}

		return size;
	}

	export void addPart(BodyPart* part, JPH::RagdollSettings* settings, JPH::Ref<JPH::Skeleton> skeleton, uint32_t skeletonJointIndex) {

		part->skeletonJointIndex = skeletonJointIndex;

		if (part->parent) {
			skeleton->AddJoint(part->name.c_str(), part->parent->skeletonJointIndex);

		}
		else {
			skeleton->AddJoint(part->name.c_str());
		}

		JPH::RagdollSettings::Part& settingsPart = settings->mParts[part->skeletonJointIndex];
		settingsPart.SetShape(part->bodyPtr->GetShape());
		settingsPart.mPosition = part->bodyPtr->GetPosition();
		settingsPart.mRotation = part->bodyPtr->GetRotation();
		settingsPart.mMotionType = JPH::EMotionType::Dynamic;
		settingsPart.mObjectLayer = Layers::MOVING;
		settingsPart.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;

		//settings->mSpace = EConstraintSpace::LocalToBodyCOM;
		if (part->constraintType == JPH::EConstraintSubType::SwingTwist) {

			JPH::Ref<JPH::SwingTwistConstraintSettings> constraint = JPH::DynamicCast<JPH::SwingTwistConstraintSettings>(part->constraintSettings);
			settingsPart.mToParent = constraint;
		}
		if (part->constraintType == JPH::EConstraintSubType::Hinge) {

			JPH::Ref<JPH::HingeConstraintSettings> constraint = JPH::DynamicCast<JPH::HingeConstraintSettings>(part->constraintSettings);
			settingsPart.mToParent = constraint;
		}
		if (part->constraintType == JPH::EConstraintSubType::Fixed) {

			JPH::Ref<JPH::FixedConstraintSettings> constraint = JPH::DynamicCast<JPH::FixedConstraintSettings>(part->constraintSettings);
			settingsPart.mToParent = constraint;

		}
	}

	// Traverse ragdoll in breadth-first order, assigning sequential skeleton indices
	// Guarantees all joints at depth D have lower indices than joints at depth D+1
	// which is important when because the  user may create some joints at depth D+1 before all the joint at depth D
	export JPH::RagdollSettings* CreateHumanoidBFS(BodyPart* root) {

		std::queue<BodyPart*> needToVisit;

		JPH::Ref<JPH::Skeleton> skeleton = new JPH::Skeleton;
		JPH::RagdollSettings* settings = new JPH::RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(ragdoll::getRagdollSize(root));

		uint32_t skeletonJointIndex = 0;

		//Base case
		needToVisit.push(root);
		addPart(root, settings, skeleton, skeletonJointIndex);

		while (!needToVisit.empty()) {
			BodyPart* current = needToVisit.front();  
			needToVisit.pop(); 

			for (int i = 0; i < current->children.size(); i++) {

				skeletonJointIndex++;
				addPart(current->children[i], settings, skeleton, skeletonJointIndex);
				needToVisit.push(current->children[i]);
			}
		}
		return settings;

	}

}
