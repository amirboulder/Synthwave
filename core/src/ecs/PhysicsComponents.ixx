module;

#include <vector>
#include <string>
#include <functional>

#include <flecs.h>

export module PhysicsComponents;

import Jolt;


//=============================================
// Physics Components
// All components that are related to physics should go here
// All components that are dependent on jolt Physics should go here.
//=============================================


// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
export namespace Layers
{
	inline constexpr JPH::ObjectLayer NON_MOVING = 0;
	inline constexpr JPH::ObjectLayer MOVING = 1;
	inline constexpr JPH::ObjectLayer Sensors = 2;
	inline constexpr JPH::ObjectLayer CHARACTER_ANCHOR = 3;
	inline constexpr JPH::ObjectLayer NUM_LAYERS = 4;
};



/// <summary>
/// Reference to the physics system which allows other system query it from the ECS
/// instead of having to pass around references.
/// </summary>
export struct PhysicsSystemRef {
	JPH::PhysicsSystem& physicsSystem;
};

export struct PhysicsBody {
	JPH::BodyID ID;
};

export struct PhysicsBodyGroup {
	std::vector<JPH::BodyID> IDs;
};

export struct JoltCharacter {
	JPH::Character* characterPtr = nullptr;
};

export struct JoltRagdoll {
	JPH::Ragdoll* ragdollPtr = nullptr;
};

export struct JoltRagdollFilter {
	JPH::IgnoreMultipleBodiesFilter* filter = nullptr;
};

export struct JoltAnimation {
	JPH::SkeletalAnimation* animationPtr = nullptr;
};


export struct JoltAnimationList {

	std::vector<std::pair<std::string, JPH::SkeletalAnimation*>> animations;

	JPH::SkeletalAnimation* find(const std::string& name) const {
		for (auto& [n, a] : animations)
			if (n == name) return a;
		return nullptr;
	}
};

export struct JoltPose {
	JPH::SkeletonPose pose;
	JPH::Vec3 root_offset;
};

//TDO delete
export struct JoltPose2 {
	JPH::SkeletonPose pose;
	float hipsFromSoles = 0.0f;
};


export struct BipedalRagdollData {

	float hipsFromSoles = 0.0f;
};

export struct AnimationTime {
	float time = 0.0f;
};


export struct PhysicsConstraint {
	JPH::Ref<JPH::SixDOFConstraint> constraint;
};

export struct JoltAnchorBody {
	JPH::BodyID bodyID;
	JPH::Ref<JPH::FixedConstraint> constraint;
};

export enum class ContactPhase : std::uint8_t { Added, Persisted, Sleeping };

export struct ContactData {
	flecs::entity self;
	flecs::entity other;
	JPH::BodyID selfBodyID;
	JPH::BodyID otherBodyID;
	JPH::RVec3               baseOffset;
	JPH::Vec3                normal;
	JPH::Vec3                centroid;

	JPH::Vec3                selfLinearVelocity; // for ContactPhase::Added Velocity is for after collision
	JPH::Vec3                otherLinearVelocity; // for ContactPhase::Added Velocity is for after collision

	JPH::Vec3                selfAngularVelocity; // for ContactPhase::Added Velocity is for after collision
	JPH::Vec3                otherAngularVelocity; // for ContactPhase::Added Velocity is for after collision

	float                    impulse;
	float                    penetrationDepth;
	ContactPhase             phase;
	bool                     frameStamp; // used to remove stale contacts

};

export struct HasContactScript {};

export using ContactFunction = std::function<void(const ContactData& contactData)>;

export struct ContactDataList {
	std::vector<ContactData> contacts;
};