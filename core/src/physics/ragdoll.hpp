#pragma once

#include "core/src/pch.h"

#define JPH_OBJECT_STREAM

#include <Jolt/Physics/Collision/Shape/ScaledShape.h>

#include "AssetStream.hpp"

namespace JPH {
	class RagdollSettings;
	enum class EMotionType : uint8;
}


enum class Attachment
{
	Top,        // +Y
	Bottom,     // -Y
	Left,       // -X
	Right,      // +X
	Front,      // +Z 
	Back        // -Z
};

#ifdef JPH_OBJECT_STREAM

enum class EConstraintOverride
{
	TypeFixed,
	TypePoint,
	TypeHinge,
	TypeSlider,
	TypeCone,
	TypeRagdoll,
};

#endif // JPH_OBJECT_STREAM

#ifdef JPH_OBJECT_STREAM

class RagdollLoader {

public:

	static JPH::RagdollSettings* load(const char* inFileName, JPH::EMotionType inMotionType, EConstraintOverride inConstraintOverride)
	{
		// Read the ragdoll
		RagdollSettings* ragdoll = nullptr;
		AssetStream stream(inFileName, std::ios::in);
		if (!ObjectStreamIn::sReadObject(stream.Get(), ragdoll))
			cout << "ERROR!!!!\n";
			

		for (RagdollSettings::Part& p : ragdoll->mParts)
		{
            
			// Update motion PipelineType
			p.mMotionType = inMotionType;
			// Override layer
			p.mObjectLayer = Layers::MOVING;


			// Create new constraint
			Ref<JPH::SwingTwistConstraintSettings> original = JPH::DynamicCast<JPH::SwingTwistConstraintSettings>(p.mToParent);
			if (original != nullptr)
				switch (inConstraintOverride)
				{
				case EConstraintOverride::TypeFixed:
				{
					JPH::FixedConstraintSettings* settings = new JPH::FixedConstraintSettings();
					settings->mPoint1 = settings->mPoint2 = original->mPosition1;
					p.mToParent = settings;
					break;
				}

				case EConstraintOverride::TypePoint:
				{
					JPH::PointConstraintSettings* settings = new JPH::PointConstraintSettings();
					settings->mPoint1 = settings->mPoint2 = original->mPosition1;
					p.mToParent = settings;
					break;
				}

				case EConstraintOverride::TypeHinge:
				{
					JPH::HingeConstraintSettings* settings = new JPH::HingeConstraintSettings();
					settings->mPoint1 = original->mPosition1;
					settings->mHingeAxis1 = original->mPlaneAxis1;
					settings->mNormalAxis1 = original->mTwistAxis1;
					settings->mPoint2 = original->mPosition2;
					settings->mHingeAxis2 = original->mPlaneAxis2;
					settings->mNormalAxis2 = original->mTwistAxis2;
					settings->mLimitsMin = -original->mNormalHalfConeAngle;
					settings->mLimitsMax = original->mNormalHalfConeAngle;
					settings->mMaxFrictionTorque = original->mMaxFrictionTorque;
					settings->mMotorSettings = original->mSwingMotorSettings;
					p.mToParent = settings;
					break;
				}

				case EConstraintOverride::TypeSlider:
				{
					JPH::SliderConstraintSettings* settings = new JPH::SliderConstraintSettings();
					settings->mPoint1 = settings->mPoint2 = original->mPosition1;
					settings->mSliderAxis1 = settings->mSliderAxis2 = original->mTwistAxis1;
					settings->mNormalAxis1 = settings->mNormalAxis2 = original->mTwistAxis1.GetNormalizedPerpendicular();
					settings->mLimitsMin = -1.0f;
					settings->mLimitsMax = 1.0f;
					settings->mMaxFrictionForce = original->mMaxFrictionTorque;
					settings->mMotorSettings = original->mSwingMotorSettings;
					p.mToParent = settings;
					break;
				}

				case EConstraintOverride::TypeCone:
				{
					JPH::ConeConstraintSettings* settings = new JPH::ConeConstraintSettings();
					settings->mPoint1 = original->mPosition1;
					settings->mTwistAxis1 = original->mTwistAxis1;
					settings->mPoint2 = original->mPosition2;
					settings->mTwistAxis2 = original->mTwistAxis2;
					settings->mHalfConeAngle = original->mNormalHalfConeAngle;
					p.mToParent = settings;
					break;
				}

				case EConstraintOverride::TypeRagdoll:
					break;
				}

		}

		// Initialize the skeleton
		ragdoll->GetSkeleton()->CalculateParentJointIndices();

		// Stabilize the constraints of the ragdoll
		ragdoll->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		//ragdoll->CalculateConstraintPriorities();

		// Calculate body <-> constraint map
		ragdoll->CalculateBodyIndexToConstraintIndex();
		ragdoll->CalculateConstraintIndexToBodyIdxPair();

		return ragdoll;
	}

#endif // JPH_OBJECT_STREAM

    static Quat CalculateRotationToVector(Vec3 inVector)
    {
        if (inVector.LengthSq() == 0) return Quat::sIdentity();
        inVector = inVector.Normalized();

        // Capsules are Y-aligned by default in Jolt
        return Quat::sFromTo(Vec3::sAxisY(), inVector);
    }

	static RagdollSettings* create(float scale = 1.0f)
	{
		// Create skeleton
		Ref<Skeleton> skeleton = new Skeleton;
		uint lower_body = skeleton->AddJoint("LowerBody");
		uint mid_body = skeleton->AddJoint("MidBody", lower_body);
		uint upper_body = skeleton->AddJoint("UpperBody", mid_body);
		uint head = skeleton->AddJoint("Head", upper_body);
		uint upper_arm_l = skeleton->AddJoint("UpperArmL", upper_body);
		uint upper_arm_r = skeleton->AddJoint("UpperArmR", upper_body);
		uint lower_arm_l = skeleton->AddJoint("LowerArmL", upper_arm_l);
		uint lower_arm_r = skeleton->AddJoint("LowerArmR", upper_arm_r);
		uint upper_leg_l = skeleton->AddJoint("UpperLegL", lower_body);
		uint upper_leg_r = skeleton->AddJoint("UpperLegR", lower_body);
		uint lower_leg_l = skeleton->AddJoint("LowerLegL", upper_leg_l);
		uint lower_leg_r = skeleton->AddJoint("LowerLegR", upper_leg_r);

		uint foot_l = skeleton->AddJoint("FootL", lower_leg_l);
		uint foot_R = skeleton->AddJoint("FootR", lower_leg_r);

		// Create shapes for limbs (scaled)
		Ref<Shape> shapes[] = {
			new CapsuleShape(0.15f * scale, 0.10f * scale),
			new CapsuleShape(0.15f * scale, 0.10f * scale),		// Mid Body
			new CapsuleShape(0.15f * scale, 0.10f * scale),		// Upper Body
			new CapsuleShape(0.075f * scale, 0.10f * scale),	// Head
			new CapsuleShape(0.15f * scale, 0.06f * scale),		// Upper Arm L
			new CapsuleShape(0.15f * scale, 0.06f * scale),		// Upper Arm R
			new CapsuleShape(0.15f * scale, 0.05f * scale),		// Lower Arm L
			new CapsuleShape(0.15f * scale, 0.05f * scale),		// Lower Arm R
			new CapsuleShape(0.2f * scale, 0.075f * scale),		// Upper Leg L
			new CapsuleShape(0.2f * scale, 0.075f * scale),		// Upper Leg R
			new CapsuleShape(0.2f * scale, 0.06f * scale),		// Lower Leg L
			new CapsuleShape(0.2f * scale, 0.06f * scale),		// Lower Leg R

			new BoxShape(Vec3(0.0444f, 0.0361f, 0.1125f) * scale,0.00361290015f),		// LEFT FOOT
			new BoxShape(Vec3(0.0444f, 0.0361f, 0.1125f) * scale,0.00361290015f),		// RIGHT FOOT
		};

		// Positions of body parts in world space (scaled)
		RVec3 positions[] = {
			RVec3(0, 1.15f * scale, 0),					// Lower Body
			RVec3(0, 1.35f * scale, 0),					// Mid Body
			RVec3(0, 1.55f * scale, 0),					// Upper Body
			RVec3(0, 1.825f * scale, 0),				// Head
			RVec3(-0.425f * scale, 1.55f * scale, 0),	// Upper Arm L
			RVec3(0.425f * scale, 1.55f * scale, 0),	// Upper Arm R
			RVec3(-0.8f * scale, 1.55f * scale, 0),		// Lower Arm L
			RVec3(0.8f * scale, 1.55f * scale, 0),		// Lower Arm R
			RVec3(-0.15f * scale, 0.8f * scale, 0),		// Upper Leg L
			RVec3(0.15f * scale, 0.8f * scale, 0),		// Upper Leg R
			RVec3(-0.15f * scale, 0.3f * scale, 0),		// Lower Leg L
			RVec3(0.15f * scale, 0.3f * scale, 0),		// Lower Leg R
			RVec3(0.145158f * scale, 0.083797f * scale, 0.0027870f * scale),
			RVec3(-0.145157f * scale, 0.083798f * scale, 0.0027870f * scale),
		};

		// Rotations of body parts in world space (unchanged)
		Quat rotations[] = {
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Lower Body
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Mid Body
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Upper Body
			Quat::sIdentity(),									 // Head
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Upper Arm L
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Upper Arm R
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Lower Arm L
			Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),		 // Lower Arm R
			Quat::sIdentity(),									 // Upper Leg L
			Quat::sIdentity(),									 // Upper Leg R
			Quat::sIdentity(),									 // Lower Leg L
			Quat::sIdentity(),									 // Lower Leg R
			Quat(0, -0.506379f, 0.862311f, 0),
			Quat(0, 0.862311f, 0.506379f, 0),
		};

		// World space constraint positions (scaled)
		RVec3 constraint_positions[] = {
			RVec3::sZero(),								// Lower Body (unused, there's no parent)
			RVec3(0, 1.25f * scale, 0),					// Mid Body
			RVec3(0, 1.45f * scale, 0),					// Upper Body
			RVec3(0, 1.65f * scale, 0),					// Head
			RVec3(-0.225f * scale, 1.55f * scale, 0),	// Upper Arm L
			RVec3(0.225f * scale, 1.55f * scale, 0),	// Upper Arm R
			RVec3(-0.65f * scale, 1.55f * scale, 0),	// Lower Arm L
			RVec3(0.65f * scale, 1.55f * scale, 0),		// Lower Arm R
			RVec3(-0.15f * scale, 1.05f * scale, 0),	// Upper Leg L
			RVec3(0.15f * scale, 1.05f * scale, 0),		// Upper Leg R
			RVec3(-0.15f * scale, 0.55f * scale, 0),	// Lower Leg L
			RVec3(0.15f * scale, 0.55f * scale, 0),		// Lower Leg R
			RVec3(0.145158f * scale, 0.083797f * scale, 0.0027870f * scale),			// 3: R Foot
			RVec3(-0.145157f * scale, 0.083798f * scale, 0.0027870f * scale),			// 6: L Foot
		};

		// World space twist axis directions (unchanged - these are normalized directions)
		Vec3 twist_axis[] = {
			Vec3::sZero(),				// Lower Body (unused, there's no parent)
			Vec3::sAxisY(),				// Mid Body
			Vec3::sAxisY(),				// Upper Body
			Vec3::sAxisY(),				// Head
			-Vec3::sAxisX(),			// Upper Arm L
			Vec3::sAxisX(),				// Upper Arm R
			-Vec3::sAxisX(),			// Lower Arm L
			Vec3::sAxisX(),				// Lower Arm R
			-Vec3::sAxisY(),			// Upper Leg L
			-Vec3::sAxisY(),			// Upper Leg R
			-Vec3::sAxisY(),			// Lower Leg L
			-Vec3::sAxisY(),			// Lower Leg R
			-Vec3::sAxisY(),			// Lower Leg L
			-Vec3::sAxisY(),			// Lower Leg R
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
		RagdollSettings* settings = new RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{
				SwingTwistConstraintSettings* constraint = new SwingTwistConstraintSettings;
				constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ();
				constraint->mTwistMinAngle = -DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = DegreesToRadians(plane_angle[p]);
				part.mToParent = constraint;
			}
		}

		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		//settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;
	}


	static RagdollSettings* createArm(RVec3 position,float scale = 1.0f) {

		// Create skeleton
		//Jolts skeleton is a vector of joints, each joint know its name, parentName and ParentIndex
		Ref<Skeleton> skeleton = new Skeleton;

		uint base = skeleton->AddJoint("Base");
		uint upperArm = skeleton->AddJoint("UpperArm", base);
		uint lowerArm = skeleton->AddJoint("LowerArm", upperArm);
	

		// Create shapes
		Ref<Shape> baseShape = new BoxShape(Vec3(1.5f, 1.5f, 1.5f) * scale);
		Ref<Shape> upperArmShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> lowerArmShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		

		Ref<Shape> shapes[] = { baseShape, upperArmShape,lowerArmShape };
		
		// Calculate positions automatically
		Vec3 basePos = position;
		Vec3 upperArmPos = ShapePlacementHelper::PlaceOnTop(baseShape, basePos, Quat::sIdentity(),
			upperArmShape, Quat::sIdentity());
		Vec3 lowerArmPos = ShapePlacementHelper::PlaceOnTop(upperArmShape, upperArmPos, Quat::sIdentity(),
			lowerArmShape, Quat::sIdentity());

	

		RVec3 positions[] = { basePos, upperArmPos,lowerArmPos };
		Quat rotations[] = { Quat::sIdentity(), Quat::sIdentity(),Quat::sIdentity()};


		// Calculate constraint positions automatically
		RVec3 constraint_positions[] = {
			RVec3::sZero(),  // Base has no parent
			ShapePlacementHelper::GetConstraintPosition(baseShape, basePos, rotations[0],
														 upperArmShape, upperArmPos, rotations[1]),			
			ShapePlacementHelper::GetConstraintPosition(upperArmShape, upperArmPos, rotations[1],
													 lowerArmShape, lowerArmPos, rotations[2]),
		};


		// Create ragdoll settings
		RagdollSettings* settings = new RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());


		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			RagdollSettings::Part& part = settings->mParts[p];

			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = EMotionType::Dynamic;
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
					constraint->mHingeAxis1 = constraint->mHingeAxis2 = Vec3::sAxisX();
					constraint->mNormalAxis1 = constraint->mNormalAxis2 = Vec3::sAxisY();

					constraint->mLimitsMin = DegreesToRadians(0.0f);
					constraint->mLimitsMax = DegreesToRadians(120.0f);

					// Configure motor settings
					constraint->mMaxFrictionTorque = 0.0f;  // Disable friction when motor is active

					part.mToParent = constraint;
				}

				
			}
		}

		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		// TODO update jolt to the latest version so we have this feature
		//settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;

	}
	
	static RagdollSettings* createSnake(RVec3 position, float scale = 1.0f) {

		Ref<Skeleton> skeleton = new Skeleton;

		uint head = skeleton->AddJoint("Head");
		uint middlePiece1 = skeleton->AddJoint("MiddlePiece1", head);
		uint middlePiece2 = skeleton->AddJoint("MiddlePiece2", middlePiece1);
		uint middlePiece3 = skeleton->AddJoint("MiddlePiece3", middlePiece2);
		uint tail = skeleton->AddJoint("Tail", middlePiece3);
		
		// Create shapes
		//Ref<Shape> rootShape = new SphereShape(1.0f);

		Ref<Shape> headShape = new CapsuleShape(1.0f * scale, 0.35f * scale);
		Ref<Shape> middlePiece1Shape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> middlePiece2Shape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> middlePiece3Shape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> tailPieceShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
	
		Ref<Shape> shapes[] = { headShape, middlePiece1Shape, middlePiece2Shape, middlePiece3Shape, tailPieceShape };

		// Calculate positions automatically

		//Vec3 rootPos = position;

		Vec3 headPos = position;

		Vec3 middlePiece1Pos = ShapePlacementHelper::PlaceOnTop(headShape, headPos, Quat::sIdentity(),
			middlePiece1Shape, Quat::sIdentity());

		Vec3 middlePiece2Pos = ShapePlacementHelper::PlaceOnTop(middlePiece1Shape, middlePiece1Pos, Quat::sIdentity(),
			middlePiece2Shape, Quat::sIdentity());

		Vec3 middlePiece3Pos = ShapePlacementHelper::PlaceOnTop(middlePiece2Shape, middlePiece2Pos, Quat::sIdentity(),
			middlePiece3Shape, Quat::sIdentity());

		Vec3 tailPiecePos = ShapePlacementHelper::PlaceOnTop(middlePiece3Shape, middlePiece3Pos, Quat::sIdentity(),
			tailPieceShape, Quat::sIdentity());

		RVec3 positions[] = { headPos, middlePiece1Pos, middlePiece2Pos, middlePiece3Pos, tailPiecePos };
		Quat rotations[] = { Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity(), Quat::sIdentity(),Quat::sIdentity() };

	
		RVec3 constraint_positions[] = {
	RVec3::sZero(),  //

	ShapePlacementHelper::GetConstraintPosition(headShape, headPos, rotations[0],
												 middlePiece1Shape, middlePiece1Pos, rotations[1]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece1Shape, middlePiece1Pos, rotations[1],
												 middlePiece2Shape, middlePiece2Pos, rotations[2]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece2Shape, middlePiece2Pos, rotations[2],
												 middlePiece3Shape, middlePiece3Pos, rotations[3]),

	ShapePlacementHelper::GetConstraintPosition(middlePiece3Shape, middlePiece3Pos, rotations[3],
												 tailPieceShape, tailPiecePos, rotations[4]),
		};

		Vec3 twist_axis[] = {
		Vec3::sZero(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
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
		RagdollSettings* settings = new RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;

			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{

				SwingTwistConstraintSettings* constraint = new SwingTwistConstraintSettings;
				constraint->mDrawConstraintSize = 0.1f * scale;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ();
				constraint->mTwistMinAngle = -DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = DegreesToRadians(plane_angle[p]);

				part.mToParent = constraint;

			}
		}


		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		// TODO update jolt to the latest version so we have this feature
		//settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;

	}

	static RagdollSettings* createHumanoid(RVec3 position, float scale = 1.0f) {

		Ref<Skeleton> skeleton = new Skeleton;

		uint hips = skeleton->AddJoint("hips");

		uint leftHipJoint = skeleton->AddJoint("LeftHipJoint", hips);
		uint leftUpperLeg = skeleton->AddJoint("LeftUpperLeg", leftHipJoint);
		uint leftLowerLeg = skeleton->AddJoint("LeftLowerLeg", leftUpperLeg);
		uint leftFoot = skeleton->AddJoint("LeftFoot", leftLowerLeg);

		uint rightHipJoint = skeleton->AddJoint("RightHipJoint", hips);
		uint rightUpperLeg = skeleton->AddJoint("RightUpperLeg", rightHipJoint);
		uint rightLowerLeg = skeleton->AddJoint("RightLowerLeg", rightUpperLeg);
		uint rightFoot = skeleton->AddJoint("RightFoot", rightLowerLeg);


		Ref<Shape> hipShape = new CapsuleShape(0.5f * scale, 0.35f * scale);

		Ref<Shape> leftHipJointShape = new SphereShape(0.2f * scale);
		Ref<Shape> leftUpperLegShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> leftLowerLegShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> leftFootShape = new BoxShape(Vec3(0.1f,0.1f,0.1f) * scale);

		Ref<Shape> rightHipJointShape = new SphereShape(0.2f * scale);
		Ref<Shape> rightUpperLegShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> rightLowerLegShape = new CapsuleShape(1.0f * scale, 0.25f * scale);
		Ref<Shape> rightFootShape = new BoxShape(Vec3(0.1f, 0.1f, 0.1f) * scale);

		Ref<Shape> shapes[] = { hipShape,
			leftHipJointShape,leftUpperLegShape,leftLowerLegShape,leftFootShape,
			rightHipJointShape,rightUpperLegShape, rightLowerLegShape, rightFootShape

		};

		Quat rotations[] = { Quat::sRotation(Vec3::sAxisZ(), 0.5f * JPH_PI),

			Quat::sIdentity(),Quat::sIdentity(),Quat::sIdentity(),Quat::sIdentity(),

			Quat::sIdentity(),Quat::sIdentity(),Quat::sIdentity(),Quat::sIdentity()
		};


		Vec3 hipPos = position;

		Vec3 leftHipJointPos = ShapePlacementHelper::PlaceOnLeft(hipShape, hipPos, rotations[0],
			leftHipJointShape, Quat::sIdentity());

		Vec3 leftUpperLegPos = ShapePlacementHelper::PlaceOnBottom(leftHipJointShape, leftHipJointPos, Quat::sIdentity(),
			leftUpperLegShape, Quat::sIdentity());

		Vec3 leftLowerLegPos = ShapePlacementHelper::PlaceOnBottom(leftUpperLegShape, leftUpperLegPos, Quat::sIdentity(),
			leftLowerLegShape, Quat::sIdentity());

		Vec3 leftFootPos = ShapePlacementHelper::PlaceOnBottom(leftLowerLegShape, leftLowerLegPos, Quat::sIdentity(),
			leftFootShape, Quat::sIdentity());



		Vec3 rightHipJointPos = ShapePlacementHelper::PlaceOnRight(hipShape, hipPos, rotations[0],
			rightHipJointShape, Quat::sIdentity());

		Vec3 rightUpperLegPos = ShapePlacementHelper::PlaceOnBottomRight(rightHipJointShape, rightHipJointPos, rotations[0],
			rightUpperLegShape, Quat::sIdentity());

		Vec3 rightLowerLegPos = ShapePlacementHelper::PlaceOnBottom(rightUpperLegShape, rightUpperLegPos, Quat::sIdentity(),
			rightLowerLegShape, Quat::sIdentity());

		Vec3 rightFootPos = ShapePlacementHelper::PlaceOnBottom(rightLowerLegShape, rightLowerLegPos, Quat::sIdentity(),
			rightFootShape, Quat::sIdentity());

		RVec3 positions[] = { hipPos,
			leftHipJointPos, leftUpperLegPos, leftLowerLegPos, leftFootPos,
			rightHipJointPos, rightUpperLegPos, rightLowerLegPos, rightFootPos };

		RVec3 constraint_positions[] = {
			RVec3::sZero(), 

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

		Vec3 twist_axis[] = {
		Vec3::sZero(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
		Vec3::sAxisY(),
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
		RagdollSettings* settings = new RagdollSettings;
		settings->mSkeleton = skeleton;
		settings->mParts.resize(skeleton->GetJointCount());
		for (int p = 0; p < skeleton->GetJointCount(); ++p)
		{
			
			RagdollSettings::Part& part = settings->mParts[p];
			part.SetShape(shapes[p]);
			part.mPosition = positions[p];
			part.mRotation = rotations[p];
			part.mMotionType = EMotionType::Dynamic;
			part.mObjectLayer = Layers::MOVING;
	
			// First part is the root, doesn't have a parent and doesn't have a constraint
			if (p > 0)
			{


				SwingTwistConstraintSettings* constraint = new SwingTwistConstraintSettings;
				//constraint->mDrawConstraintSize = ;  // Scale constraint visualization
				constraint->mPosition1 = constraint->mPosition2 = constraint_positions[p];
				constraint->mTwistAxis1 = constraint->mTwistAxis2 = twist_axis[p];
				constraint->mPlaneAxis1 = constraint->mPlaneAxis2 = Vec3::sAxisZ();
				constraint->mTwistMinAngle = -DegreesToRadians(twist_angle[p]);
				constraint->mTwistMaxAngle = DegreesToRadians(twist_angle[p]);
				constraint->mNormalHalfConeAngle = DegreesToRadians(normal_angle[p]);
				constraint->mPlaneHalfConeAngle = DegreesToRadians(plane_angle[p]);

				part.mToParent = constraint;

			}
		}


		// Optional: Stabilize the inertia of the limbs
		settings->Stabilize();

		// Optional: Calculate constraint priorities to give more priority to the root
		// TODO update jolt to the latest version so we have this feature
		//settings->CalculateConstraintPriorities();

		// Disable parent child collisions so that we don't get collisions between constrained bodies
		settings->DisableParentChildCollisions();

		// Calculate the map needed for GetBodyIndexToConstraintIndex()
		settings->CalculateBodyIndexToConstraintIndex();

		return settings;
	}


	struct ShapePlacementHelper {

		// Get the top position of a shape in world space
		static Vec3 GetTopPosition(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {
			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(currentPosition.GetX(), bounds.mMax.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static Vec3 GetBottomPosition(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {

			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(currentPosition.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static Vec3 GetRightPosition(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {

			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(bounds.mMax.GetX(), currentPosition.GetY(), currentPosition.GetZ());
		}

		// Get the bottom position of a shape in world space
		static Vec3 GetLeftPosition(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {

			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(bounds.mMin.GetX(), currentPosition.GetY(), currentPosition.GetZ());
		}

		static Vec3 GetBottomRightPos(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {

			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(bounds.mMax.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		static Vec3 GetBottomLeftPos(const Shape* shape, Vec3 currentPosition, Quat rotation = Quat::sIdentity()) {

			Mat44 transform = Mat44::sRotationTranslation(rotation, currentPosition);
			AABox bounds = shape->GetWorldSpaceBounds(transform, Vec3::sReplicate(1.0f));
			return Vec3(bounds.mMin.GetX(), bounds.mMin.GetY(), currentPosition.GetZ());
		}

		// Place a shape on top of another shape
		static Vec3 PlaceOnTop(const Shape* baseShape, Vec3 basePosition, Quat baseRotation,
			const Shape* newShape, Quat newRotation = Quat::sIdentity(),
			float gap = 0.01f) {
			// Get top of base shape
			Vec3 topOfBase = GetTopPosition(baseShape, basePosition, baseRotation);

			// Get how far the new shape's center is from its bottom
			Mat44 newTransform = Mat44::sRotationTranslation(newRotation, Vec3::sZero());
			AABox newBounds = newShape->GetLocalBounds();
			float newShapeCenterToBottom = -newBounds.mMin.GetY();

			// Place new shape so its bottom aligns with top of base
			return Vec3(basePosition.GetX(),
				topOfBase.GetY() + newShapeCenterToBottom + gap,
				basePosition.GetZ());
		}

		static Vec3 PlaceOnBottom(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Quat childRotation = Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			Vec3 bottomOfBase = GetBottomPosition(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTransform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		static Vec3 PlaceOnRight(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Quat childRotation = Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			Vec3 rightOfBase = GetRightPosition(parentShape, parentPos, parentRot);

			return Vec3(rightOfBase.GetX(), rightOfBase.GetY() - gap, rightOfBase.GetZ());

		}

		static Vec3 PlaceOnLeft(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Quat childRotation = Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			Vec3 leftofBase = GetLeftPosition(parentShape, parentPos, parentRot);

			return Vec3(leftofBase.GetX(), leftofBase.GetY() + gap, leftofBase.GetZ());

		}

	
		static Vec3 PlaceOnBottomRight(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Quat childRotation = Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			Vec3 bottomOfBase = GetBottomRightPos(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTransform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		static Vec3 PlaceOnBottomLeft(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Quat childRotation = Quat::sIdentity(),
			float gap = 0.01f) {

			// Get top of base shape
			Vec3 bottomOfBase = GetBottomLeftPos(parentShape, parentPos, parentRot);

			// Get how far the new shape's center is from its top
			//Mat44 childTramsform = Mat44::sRotationTranslation(childRotation, Vec3::sZero());
			AABox newBounds = childShape->GetLocalBounds();
			float newShapeCenterToTop = newBounds.mMax.GetY();


			return Vec3(bottomOfBase.GetX(), bottomOfBase.GetY() - newShapeCenterToTop - gap, bottomOfBase.GetZ());

		}

		// Get the constraint position (midpoint between two connected bodies)
		static Vec3 GetConstraintPosition(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Vec3 childPos, Quat childRot) {
			Vec3 parentTop = GetTopPosition(parentShape, parentPos, parentRot);
			Vec3 childBottom = GetBottomPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentTop + childBottom) * 0.5f;
		}

		// Get the constraint position (midpoint between two connected bodies)
		static Vec3 constraintPosLeftSide(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Vec3 childPos, Quat childRot) {
			Vec3 parentTop = GetTopPosition(parentShape, parentPos, parentRot);
			Vec3 childBottom = GetBottomPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentTop + childBottom) * 0.5f;
		}

		static Vec3 placeConstraintBottom(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Vec3 childPos, Quat childRot) {
			Vec3 parentBottom = GetBottomPosition(parentShape, parentPos, parentRot);
			Vec3 childTop = GetTopPosition(childShape, childPos, childRot);

			// Constraint at midpoint
			return (parentBottom + childTop) * 0.5f;
		}

		static Vec3 placeConstraintRight(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Vec3 childPos, Quat childRot) {
			Vec3 parentRight = GetRightPosition(parentShape, parentPos, parentRot);

			return (parentRight) * 0.5f;
		}

		static Vec3 placeConstraintLeft(const Shape* parentShape, Vec3 parentPos, Quat parentRot,
			const Shape* childShape, Vec3 childPos, Quat childRot) {
			Vec3 parentLeft = GetLeftPosition(parentShape, parentPos, parentRot);


			// Constraint at midpoint
			return (parentLeft) * 0.5f;
		}
	};
};

struct BodyPart {

	string name;
	uint skeletonJointIndex;
	Ref<Shape> shape;
	Body* bodyPtr;
	JPH::BodyID id;
	BodyPart* parent;
	Attachment attachment;
	std::vector<BodyPart*> children;
	Ref<TwoBodyConstraintSettings> constraintSettings;
	EConstraintSubType constraintType;
};


static RagdollSettings* createHumanoid2(vector< BodyPart> bodyParts, float scale = 1.0f) {

	Ref<Skeleton> skeleton = new Skeleton;

	// Create ragdoll settings
	RagdollSettings* settings = new RagdollSettings;
	settings->mSkeleton = skeleton;
	settings->mParts.resize(skeleton->GetJointCount());
	for (int i = 0; i < bodyParts.size(); i++)
	{

		RagdollSettings::Part& part = settings->mParts[i];
		part.SetShape(bodyParts[i].shape);
		//part.mPosition = bodyParts[i].position;
		//part.mRotation = bodyParts[i].rotation;
		part.mMotionType = EMotionType::Dynamic;
		part.mObjectLayer = Layers::MOVING;

		// First part is the root, doesn't have a parent and doesn't have a constraint
		if (i > 0)
		{

			//constraint->mDrawConstraintSize = ;  // Scale constraint visualization
			//Change to 
			if (bodyParts[i].constraintType == EConstraintSubType::SwingTwist) {

				Ref<JPH::SwingTwistConstraintSettings> constraint = JPH::DynamicCast<JPH::SwingTwistConstraintSettings>(bodyParts[i].constraintSettings);
				part.mToParent = constraint;
			}
			if (bodyParts[i].constraintType == EConstraintSubType::Hinge) {

				Ref<JPH::HingeConstraintSettings> constraint = JPH::DynamicCast<JPH::HingeConstraintSettings>(bodyParts[i].constraintSettings);
				part.mToParent = constraint;
			}
			if (bodyParts[i].constraintType == EConstraintSubType::Fixed) {

				Ref<JPH::FixedConstraintSettings> constraint = JPH::DynamicCast<JPH::FixedConstraintSettings>(bodyParts[i].constraintSettings);
				part.mToParent = constraint;
			}
			


		}
	}


	// Optional: Stabilize the inertia of the limbs
	settings->Stabilize();

	// Optional: Calculate constraint priorities to give more priority to the root
	// TODO update jolt to the latest version so we have this feature
	//settings->CalculateConstraintPriorities();

	// Disable parent child collisions so that we don't get collisions between constrained bodies
	settings->DisableParentChildCollisions();

	// Calculate the map needed for GetBodyIndexToConstraintIndex()
	settings->CalculateBodyIndexToConstraintIndex();

	return settings;
}


