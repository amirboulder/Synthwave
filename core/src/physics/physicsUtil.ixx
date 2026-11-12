module;

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <optional>

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

    export float calculateDistance(const JPH::Vec3& a, const JPH::Vec3& b) {

        return std::sqrt((b.GetX() - a.GetX()) * (b.GetX() - a.GetX())
            + (b.GetY() - a.GetY()) * (b.GetY() - a.GetY())
            + (b.GetZ() - a.GetZ()) * (b.GetZ() - a.GetZ()));
    }

    //2D distance along the xz plane
    export float calculateDistanceXZ(const JPH::Vec3& a, const JPH::Vec3& b) {

        return std::sqrt((b.GetX() - a.GetX()) * (b.GetX() - a.GetX())
            + (b.GetZ() - a.GetZ()) * (b.GetZ() - a.GetZ()));
    }


    float cross(const JPH::Vec3& O , const JPH::Vec3& A, const JPH::Vec3& B) {

        return (A.GetX() - O.GetX()) * (B.GetZ() - O.GetZ())
            -  (A.GetZ() - O.GetZ()) * (B.GetX() - O.GetX());
    }   

    export struct ConvexHull {

        std::vector<JPH::Vec3> points;

    };

    //From https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
    //Note: it will sort the callers vector!
    export std::vector<JPH::Vec3> buildConvexHull(std::vector<JPH::Vec3>& points) {

        size_t numPoints = points.size();
        size_t k = 0;

        //There must be at least 3 points for a convex hull with area 
        // but we want to know if even a line segment or a point is present
        if (numPoints < 3) {
            return points;
        }

        std::vector<JPH::Vec3> hull(2 * numPoints); //Jolt does not have a Vec2 type so we use Vec3.

        // Sort points lexicographically
        std::sort(points.begin(), points.end(), [](const JPH::Vec3& a, const JPH::Vec3& b) {
            return a.GetX() < b.GetX() || (a.GetX() == b.GetX() && a.GetZ() < b.GetZ());
        });


        //Build lower hull
        for (size_t i = 0; i < numPoints; i++) {
            while (k >= 2 && cross(hull[k - 2], hull[k - 1], points[i]) <= 0) k--;
            hull[k++] = points[i];
        }
        
        //Build upper hull
        for (size_t i = numPoints - 1, t = k + 1; i > 0; --i) {
            while (k >= t && cross(hull[k - 2], hull[k - 1], points[i - 1]) <= 0) k--;
            hull[k++] = points[i - 1];
        }
      
        hull.resize(k - 1);
        return hull;
    }

    //Calculates the StabilityMargin assuming Y is up!
    export float calculateStabilityMargin(const std::vector<JPH::Vec3>& hull,const JPH::Vec3& point) {

        constexpr float eps = 0.003f;

        float stabilityMargin = FLT_MAX;
         
        size_t hullSize = hull.size();

        if (hullSize == 0) {
            return -FLT_MAX;
        }
        if (hullSize == 1) {
            return -calculateDistanceXZ(hull[0], point);
        }

        if (hullSize == 2) {

            JPH::Vec3 ab = hull[1] - hull[0];
            JPH::Vec3 ap = point - hull[0];

            ab.SetY(0.0f);
            ap.SetY(0.0f);

            float abLenSquared = ab.LengthSq();

            //If the two points are the same fall back to single point case.
            if (abLenSquared < eps * eps) {
                return -calculateDistanceXZ(hull[0], point);
            }

            float proj = ap.Dot(ab);

            float d = proj / abLenSquared;
            
            d = std::clamp(d, 0.0f, 1.0f);

            JPH::Vec3 cp = hull[0] + d * ab;

            return -calculateDistanceXZ(point, cp);
            
        }

        for (size_t i = 0; i < hull.size(); i++) {

            const JPH::Vec3& a = hull[i];
            const JPH::Vec3& b = hull[(i + 1) % hull.size()];

            float distanceXZ = calculateDistanceXZ(b, a);

            if (distanceXZ < eps) {
                continue;
            }

            float currentMargin =   cross(a, b, point) / distanceXZ;

            stabilityMargin = std::min(currentMargin, stabilityMargin);
        }

        //If all points are almost the same, fall back to single point case.
        if (stabilityMargin == FLT_MAX) {
            return -calculateDistanceXZ(hull[0], point);
        }

        return stabilityMargin;
    }

    export std::optional<JPH::Vec3> calculateCentroid(const std::vector<JPH::Vec3>& hull) {

        if (hull.size() == 0) {
            return std::nullopt;
        }

        JPH::Vec3 centroid{};

        for (const JPH::Vec3& point : hull) {
            centroid += point;
        }

        centroid /= hull.size();

        return centroid;
    }
}
