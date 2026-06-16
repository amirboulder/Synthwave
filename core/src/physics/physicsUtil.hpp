#pragma once

namespace Utils::Phys {

	struct GroundInfo {
		JPH::Vec3 groundPoint;
		JPH::Vec3 groundNormal;
		float distanceToGround = 0.0f;
		JPH::BodyID groundBodyID;
        bool isGrounded = false;
	};

    GroundInfo CheckGround(PhysicsSystem& physicsSystem, JPH::Vec3 & entityPos, JPH::AABox & entityAABOX, JPH::Vec3 & entityExtent, BodyID physicsID) {
        GroundInfo info;

        BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

        //JPH::Vec3 entityPos = bodyInterface.GetPosition(ent.physicsID);
        //JPH::AABox entityAABOX = bodyInterface.GetTransformedShape(ent.physicsID).GetWorldSpaceBounds();
        //JPH::Vec3 entityExtent = entityAABOX.GetExtent();

        // Cast from entity bottom
        JPH::Vec3 rayStart = JPH::Vec3(entityPos.GetX(),
            entityPos.GetY() - entityExtent.GetY() + 0.05f,
            entityPos.GetZ());
        JPH::Vec3 rayDirection = JPH::Vec3(0, +1, 0);
        float maxGroundDistance = 0.3f; // Maximum distance to consider "grounded"


        JPH::RRayCast ray(rayStart, rayDirection * maxGroundDistance);
        JPH::RayCastResult result;

        const DefaultBroadPhaseLayerFilter default_broadphase_layer_filter = physicsSystem.GetDefaultBroadPhaseLayerFilter(1);
        const BroadPhaseLayerFilter& broadphase_layer_filter = default_broadphase_layer_filter;

        const DefaultObjectLayerFilter default_object_layer_filter = physicsSystem.GetDefaultLayerFilter(1);
        const ObjectLayerFilter& object_layer_filter = default_object_layer_filter;

        const IgnoreSingleBodyFilter default_body_filter(physicsID);
        const BodyFilter& body_filter = default_body_filter;

       // physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result, broadphase_layer_filter, object_layer_filter, body_filter);

        if (physicsSystem.GetNarrowPhaseQuery().CastRay(ray, result, broadphase_layer_filter, object_layer_filter, body_filter)) {
            info.groundPoint = ray.mOrigin + ray.mDirection * result.mFraction;
           // info.groundNormal = result.mSurfaceNormal;
            info.distanceToGround = result.mFraction * maxGroundDistance;
            info.groundBodyID = result.mBodyID;
            info.isGrounded = info.distanceToGround <= 0.1f;
        }

        return info;
    }


    bool isPlayerVisible(JPH::PhysicsSystem* physicsSystem,
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
            // Hit something — check if it's the player
            return hit.mBodyID == playerBodyID;
        }

        return true; // Nothing in the way
    }


    bool checkVisibilityRayCast(const JPH::PhysicsSystem& physicsSystem,
        JPH::Vec3 fromPos,
        JPH::Vec3 toPos,
        JPH::BodyID sourceBody,
        JPH::BodyID targetBody)
    {
        JPH::Vec3 dir = toPos - fromPos;

        // RRayCast takes origin + direction (not normalized, length = max distance)
        JPH::RRayCast ray{ fromPos, dir };
        JPH::RayCastResult hit;

        // IgnoreMultipleBodiesFilter lets us skip the actor's own body
        JPH::IgnoreSingleBodyFilter bodyFilter(sourceBody);

        if (physicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {}, bodyFilter))
        {
            // Hit something — check if it's the player
            return hit.mBodyID == targetBody;
        }

        return true; // Nothing in the way
    }


    void PrintJPHMat4(const JPH::Mat44& mat, unsigned int index) {
        std::cout << "JPH Matrix with index: " << index << "\n";
        for (int row = 0; row < 4; ++row) {
            std::cout << "| ";
            for (int col = 0; col < 4; ++col) {
                // Access matrix in column-major order, but print as row-major for readability
                std::cout << std::setw(10) << std::setprecision(4)
                    << std::fixed << mat.GetColumn4(col)[row] << " ";
            }
            std::cout << "|\n";
        }
        std::cout << "\n"; // Add newline for separation
    }

    void PrintGLMMat4(const glm::mat4& mat, const unsigned int index) {
        std::cout << "GLM Matrix with index : " << index << ":\n";
        for (int row = 0; row < 4; ++row) {
            std::cout << "| ";
            for (int col = 0; col < 4; ++col) {
                std::cout << std::setw(10) << std::setprecision(4) << mat[col][row] << " ";
            }
            std::cout << "|\n";
        }
    }

}
