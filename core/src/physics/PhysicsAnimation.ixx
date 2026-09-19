module;

#include <iostream>

export module PhysicsAnimation;

import Jolt;
import Logger;
import JoltAssetStream;


// Loads a SkeletalAnimation from a .tof object stream and optionally applies a
// uniform runtime scale via Jolt's SkeletalAnimation::ScaleJoints, which
// multiplies every keyframe's local translation (bone offsets / root position)
// by inScale and leaves rotations and keyframe times untouched. This must
// match the scale passed to RagdollLoader::load so the driven pose skeleton is
// the same size as the ragdoll bodies.
export class AnimationLoader {

public:

	//TODO this should take in a fs::path instead of const char *
	static JPH::SkeletalAnimation* load(const char* inFileName, float scale = 1.0f)
	{
		JPH::SkeletalAnimation* animation = nullptr;
		AssetStream stream(inFileName, std::ios::in);
		if (!JPH::ObjectStreamIn::sReadObject(stream.Get(), animation)) {
			LogError(LOG_PHYSICS, "Failed reading in animation data in file %s", inFileName);
			return animation;
		}

		if (scale != 1.0f)
			animation->ScaleJoints(scale);

		return animation;
	}

};