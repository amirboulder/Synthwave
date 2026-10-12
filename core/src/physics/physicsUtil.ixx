module;

#include <string>
#include <cmath>
#include <algorithm>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/RayCast.h>
#endif

export module PhysicsUtil;

import Jolt;

export struct GroundInfo {
    JPH::Vec3 groundPoint;
    JPH::Vec3 groundNormal;
    float distanceToGround = 999999.0f;
    JPH::BodyID groundBodyID;
    bool isGrounded = false;
};

export namespace Utils::Phys {

    export GroundInfo CheckGround(
        JPH::PhysicsSystem& physicsSystem, 
        const JPH::Vec3& rayStart, 
        const JPH::ObjectLayerFilter& inObjectLayerFilter, 
        const JPH::BodyFilter& filter) {
        GroundInfo info;

        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

        JPH::Vec3 rayDirection = JPH::Vec3(0, -1, 0);
        float howFarToCheck = 10.0f;

        rayDirection *= howFarToCheck;

        JPH::RRayCast ray(rayStart, rayDirection);
        JPH::RayCastResult result;

        bool didHit = physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result, {}, inObjectLayerFilter, filter);

        if (didHit) {
            info.groundPoint = ray.mOrigin + ray.mDirection * result.mFraction;
            //info.groundNormal = result.mSurfaceNormal;
            info.distanceToGround = result.mFraction * howFarToCheck;
            info.groundBodyID = result.mBodyID;
            info.isGrounded = info.distanceToGround <= 0.1f;
        }


#ifdef JPH_DEBUG_RENDERER
        if (didHit) {
            JPH::RVec3 hitPosition = rayStart + result.mFraction * rayDirection;
            JPH::DebugRenderer::sInstance->DrawLine(rayStart, hitPosition, JPH::Color::sGreen);
            JPH::DebugRenderer::sInstance->DrawMarker(hitPosition, JPH::Color::sYellow, 0.1f);
        }
        else {
            JPH::DebugRenderer::sInstance->DrawLine(rayStart, rayStart + rayDirection, JPH::Color::sRed);
        }
#endif

        return info;
    }




    export bool isPlayerVisible(JPH::PhysicsSystem* physicsSystem,
        JPH::Vec3 fromPos,
        JPH::Vec3 toPos,
        JPH::BodyID actorBodyID,
        JPH::BodyID playerBodyID)
    {
        JPH::Vec3 dir = toPos - fromPos;

        // RRayCast takes origin + direction (not normalized, length = max distance)
        JPH::RRayCast ray{ fromPos, dir };
        JPH::RayCastResult hit;

        // IgnoreMultipleBodiesFilter lets us skip the actor's own body
        JPH::IgnoreSingleBodyFilter bodyFilter(actorBodyID);

        if (physicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit, {}, {}, bodyFilter))
        {
            // Hit something - check if it's the player
            return hit.mBodyID == playerBodyID;
        }

        return true; // Nothing in the way
    }


    export bool checkVisibilityRayCast(const JPH::PhysicsSystem& physicsSystem,
        JPH::Vec3 fromPos,
        JPH::Vec3 toPos,
        JPH::BodyID sourceBody,
        JPH::BodyID targetBody)
    {
       JPH::Vec3 dir = toPos - fromPos;

        // RRayCast takes origin + direction (not normalized, length = max distance)
        JPH::RRayCast ray{ dir, toPos };
        JPH::RayCastResult hit;

        // IgnoreMultipleBodiesFilter lets us skip the actor's own body
        JPH::IgnoreSingleBodyFilter bodyFilter(sourceBody);

        if (physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {}, bodyFilter))
        {
            // Hit something - check if it's the player
            return hit.mBodyID == targetBody;
        }

        return true; // Nothing in the way
    }


    export void disableCollisions(JPH::BodyID bodyID1, JPH::BodyID bodyID2, JPH::BodyInterface& bi) {

        // Disable collisions between Ragdoll and JoltCharacter
        JPH::Ref<JPH::GroupFilterTable> filterTable = new JPH::GroupFilterTable(2);
        filterTable->DisableCollision(0, 1);

    
        JPH::CollisionGroup ragdollCollisionGroup = bi.GetCollisionGroup(bodyID1);
        ragdollCollisionGroup.SetGroupFilter(filterTable);
        ragdollCollisionGroup.SetGroupID(1);
        ragdollCollisionGroup.SetSubGroupID(0);
        bi.SetCollisionGroup(bodyID1, ragdollCollisionGroup);

        JPH::CollisionGroup characterCollisionGroup = bi.GetCollisionGroup(bodyID2);
        characterCollisionGroup.SetGroupFilter(filterTable);
        characterCollisionGroup.SetGroupID(1);
        characterCollisionGroup.SetSubGroupID(1);
        bi.SetCollisionGroup(bodyID2, characterCollisionGroup);

    }
}
