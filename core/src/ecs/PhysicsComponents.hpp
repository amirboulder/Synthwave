#pragma once
//=============================================
// Physics Components
// All components that are related to physics should go here
// All components that are dependent on jolt Physics should go here.
//=============================================

/// <summary>
/// Reference to the physics system which allows other system query it from the ECS
/// instead of having to pass around references.
/// </summary>
struct PhysicsSystemRef {
	JPH::PhysicsSystem& physicsSystem;
};

struct PhysicsBody {
	JPH::BodyID ID;
};

struct PhysicsBodyGroup {
	std::vector<JPH::BodyID> IDs;
};

struct JoltCharacter {
	JPH::Character* characterPtr = nullptr;
};

struct JoltRagdoll {
	JPH::Ragdoll* ragdollPtr = nullptr;
};

struct JoltRagdollFilter {
	JPH::IgnoreMultipleBodiesFilter* filter = nullptr;
};

struct JoltAnimation {
	JPH::SkeletalAnimation* animationPtr = nullptr;
};


struct JoltAnimationList {

	std::vector<std::pair<std::string, JPH::SkeletalAnimation*>> animations;

	JPH::SkeletalAnimation* find(const std::string& name) const {
		for (auto& [n, a] : animations)
			if (n == name) return a;
		return nullptr;
	}
};

struct JoltPose {
	JPH::SkeletonPose pose;
	JPH::Vec3 root_offset;
};

//TDO delete
struct JoltPose2 {
	JPH::SkeletonPose pose;
	float hipsFromSoles = 0.0f;
};


struct BipedalRagdollData {

	float hipsFromSoles = 0.0f;
};

struct AnimationTime {
	float time = 0.0f;
};


struct PhysicsConstraint {
	JPH::Ref<JPH::SixDOFConstraint> constraint;
};

struct JoltAnchorBody {
	JPH::BodyID bodyID;
	JPH::Ref<JPH::FixedConstraint> constraint;
};