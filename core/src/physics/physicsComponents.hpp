#pragma once

//PLace All physics Components here

enum class ContactPhase : uint8 { Added, Persisted, Sleeping };

struct ContactData {
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

struct HasContactScript {};

using ContactFunction = std::function<void(const ContactData& contactData)> ;

struct ContactDataList {
    std::vector<ContactData> contacts;
};