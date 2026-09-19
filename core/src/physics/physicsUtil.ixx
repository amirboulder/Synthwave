module;

#include <string>
#include <cmath>
#include <algorithm>

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

    export GroundInfo CheckGround(JPH::PhysicsSystem& physicsSystem,const JPH::Vec3& rayStart,const JPH::BodyFilter& filter) {
        GroundInfo info;

        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

        JPH::Vec3 rayDirection = JPH::Vec3(0, -1, 0);
        float howFarToCheck = 10.0f;

        rayDirection *= howFarToCheck;

        JPH::RRayCast ray(rayStart, rayDirection);
        JPH::RayCastResult result;

        bool didHit = physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result, {}, {}, filter);

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



    export void buildRagdollFilter(JPH::Ragdoll* ragdoll, JPH::IgnoreMultipleBodiesFilter& filter) {

        filter.Reserve(ragdoll->GetBodyCount());
        for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

            filter.IgnoreBody(id);

        }
    }

    export float getHipsFromSolesDist(JPH::Ragdoll* ragdoll, JPH::Skeleton* skel, JPH::BodyInterface& bi) {
        
        //Find joint by name
        auto findJoint = [&](const char* name) -> int {
            for (int i = 0, n = (int)skel->GetJointCount(); i < n; ++i)
                if (std::strcmp(skel->GetJoint(i).mName.c_str(), name) == 0) return i;
            return -1;
        };

        int footL = findJoint("L_Foot_sjnt_0");
        int footR = findJoint("R_Foot_sjnt_0");

        skel->GetJoint(footL).mParentJointIndex;

        float minSoleY = FLT_MAX;
        for (int fi : { footL, footR }) {
            if (fi < 0) continue;
            JPH::BodyID footID = ragdoll->GetBodyID(fi);
            JPH::AABox wb = bi.GetTransformedShape(footID).GetWorldSpaceBounds();
            minSoleY = std::min(minSoleY, wb.mMin.GetY());
        }

        float hipComY = bi.GetCenterOfMassPosition(ragdoll->GetBodyID(0)).GetY();

        return hipComY - minSoleY;       
    }

    export uint32_t findJoint(JPH::Ragdoll* ragdoll, JPH::Skeleton* skel, std::string_view jointName) {

        for (int i = 0, n = (int)skel->GetJointCount(); i < n; ++i)
            if (std::strcmp(skel->GetJoint(i).mName.c_str(), jointName.data()) == 0) return i;
        return -1;

    }

    export void MoveAndRotateRagdoll(JPH::Ragdoll* ragdoll, JPH::BodyInterface& bi, const JPH::Vec3& desiredPos, const JPH::Quat& desiredRot, const JPH::EActivation& activation) {

        JPH::BodyID rootID = ragdoll->GetBodyID(0);
        JPH::Vec3  currentRoot = bi.GetPosition(rootID);
        JPH::RVec3  delta = desiredPos - currentRoot;
        for (JPH::BodyID id : ragdoll->GetBodyIDs()) {
            JPH::RVec3 p = bi.GetPosition(id);
            JPH::Quat  q = bi.GetRotation(id);
            bi.SetPositionAndRotation(id, p + delta,
                desiredRot * q,
                activation);
        }
    }

    export JPH::AABox getRagdollBoundingBox(JPH::Ragdoll* ragdoll, JPH::BodyInterface& bi) {
        JPH::AABox boundingBox; // starts invalid: mMin = FLT_MAX, mMax = -FLT_MAX

        for (JPH::BodyID id : ragdoll->GetBodyIDs()) {
            JPH::AABox wb = bi.GetTransformedShape(id).GetWorldSpaceBounds();
            boundingBox.Encapsulate(wb);
        }

        return boundingBox;
    }

 

}
