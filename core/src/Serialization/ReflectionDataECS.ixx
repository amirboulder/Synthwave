module;

#include <string>

#include <flecs.h>

// IntelliSense only; cl.exe never sees this. Module units skip the PCH, so they
// get JPH types solely from the import, which IntelliSense reports as incomplete.
#ifdef __INTELLISENSE__
#include <Jolt/Jolt.h>
#endif

export module ReflectionDataECS;

import Jolt;
import Components;
import GraphicsComponents;
import PhysicsComponents;
import PlayerComponents;
import Mesh;
import Camera;
import Util;


export bool buildRigidBodyDescFromJolt(JPH::PhysicsSystem& physicsSystem, JPH::BodyID id, RigidBodyDesc& desc) {

    // Don't call BodyInterface while holding this lock, Jolt's body mutexes are not recursive
    JPH::BodyLockRead lock(physicsSystem.GetBodyLockInterface(), id);
    if (!lock.Succeeded()) return false;
    const JPH::Body& body = lock.GetBody();

    desc.motionType = body.GetMotionType();
    desc.friction = body.GetFriction();
    desc.restitution = body.GetRestitution();
    //desc.allowSleeping = body.GetAllowSleeping(); //Causes errors just allow everything to sleep for now.
    desc.activation = body.IsActive();
    desc.layer = body.GetObjectLayer();
    desc.linearVelocity = JPHVec3ToGLM(body.GetLinearVelocity());
    desc.angularVelocity = JPHVec3ToGLM(body.GetAngularVelocity());

    // Static bodies have no MotionProperties, keep the defaults
    if (const JPH::MotionProperties* mp = body.GetMotionPropertiesUnchecked(); mp && !body.IsStatic()) {
        desc.mass = mp->GetInverseMass() > 0.0f ? 1.0f / mp->GetInverseMass() : 0.0f;
        desc.linearDamping = mp->GetLinearDamping();
        desc.angularDamping = mp->GetAngularDamping();
        desc.gravityFactor = mp->GetGravityFactor();
        desc.allowedDOFs = mp->GetAllowedDOFs();
        desc.motionQuality = mp->GetMotionQuality();
    }

    return true;
}


export void registerReflectionData(flecs::world & ecs){

    
    ecs.component<glm::vec3>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    ecs.component<glm::quat>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z")
        .member<float>("w");

    ecs.component<JPH::Vec3>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    ecs.component<JPH::Quat>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z")
        .member<float>("w");

   ecs.component<Transform>()
        .member<glm::vec3>("position")
        .member<glm::quat>("rotation")
        .member<glm::vec3>("scale");

   // Register reflection for std::string
   ecs.component<std::string>()
       .opaque(flecs::String) // Opaque PipelineType that maps to string
       .serialize([](const flecs::serializer* s, const std::string* data) {
       const char* str = data->c_str();
       return s->value(flecs::String, &str); // Forward to serializer
   })
       .assign_string([](std::string* data, const char* value) {
       *data = value; // Assign new value to std::string
   });
   
   ecs.component<ModelSourceName>()
       .member<std::string>("name");

   ecs.component<ObjectType>()
       .member<std::string>("name");

   ecs.component<EntityType>()
       .constant("Empty", EntityType::Empty)
       .constant("Actor", EntityType::Actor)
       .constant("Capsule", EntityType::Capsule)
       .constant("Grid", EntityType::Grid)
       .constant("StaticMesh", EntityType::StaticMesh)
       .constant("Sphere", EntityType::Sphere)
       .constant("Cube", EntityType::Cube)
       .constant("Light", EntityType::Light)
       .constant("Camera", EntityType::Camera);

   // Pointer-to-member so offsets are correct, Player starts with non-reflected members (allocator, listener, refs)
   // Position and rotation are saved through the entity's Transform
   ecs.component<Player>()
       .member("moveSpeed",        &Player::moveSpeed)
       .member("jumpSpeed",        &Player::jumpSpeed)
       .member("terminalVelocity", &Player::terminalVelocity)
       .member("gravity",          &Player::gravity)
       .member("cameraOffset",     &Player::cameraOffset)
       ;

   ecs.component<Camera>()
       .member<float>("yaw")
       .member<float>("pitch")
       ;



   ecs.component<RigidBodyDesc>()
       .member("mass", &RigidBodyDesc::mass)
       .member("friction", &RigidBodyDesc::friction)
       .member("restitution", &RigidBodyDesc::restitution)
       .member("linearDamping", &RigidBodyDesc::linearDamping)
       .member("angularDamping", &RigidBodyDesc::angularDamping)
       .member("gravityFactor", &RigidBodyDesc::gravityFactor)
       .member("linearVelocity", &RigidBodyDesc::linearVelocity)
       .member("angularVelocity", &RigidBodyDesc::angularVelocity)
       .member("motionQuality", &RigidBodyDesc::motionQuality)
       // EAllowedDOFs is a uint8 bitmask, store the raw bits
       .member<uint8_t>("allowedDOFs", 1, offsetof(RigidBodyDesc, allowedDOFs))
       .member("motionType", &RigidBodyDesc::motionType)
       .member("layer", &RigidBodyDesc::layer)
       .member("activation", &RigidBodyDesc::activation)
       .member("allowSleeping", &RigidBodyDesc::allowSleeping)
       ;
   
   // BodyID serializes as a RigidBodyDesc read live from Jolt, so to_json() captures physics state.
  // Must be capture-less (flecs takes a function pointer), get the world from the serializer instead.
   ecs.component<JPH::BodyID>()
       .opaque(ecs.component<RigidBodyDesc>().id())
       .serialize([](const flecs::serializer* s, const JPH::BodyID* id) -> int {

       const flecs::world world(const_cast<flecs::world_t*>(s->world));
       JPH::PhysicsSystem& ps = world.get<PhysicsSystemRef>().physicsSystem;

       RigidBodyDesc desc;             // defaults if the body is gone
       buildRigidBodyDescFromJolt(ps, *id, desc);

       // as_type is a struct, so flecs writes the braces and expects member/value pairs
       const uint8_t motionQuality = static_cast<uint8_t>(desc.motionQuality);
       const uint8_t allowedDOFs = static_cast<uint8_t>(desc.allowedDOFs);
       const uint8_t motionType = static_cast<uint8_t>(desc.motionType);

       s->member("mass");            s->value(desc.mass);
       s->member("friction");        s->value(desc.friction);
       s->member("restitution");     s->value(desc.restitution);
       s->member("linearDamping");   s->value(desc.linearDamping);
       s->member("angularDamping");  s->value(desc.angularDamping);
       s->member("gravityFactor");   s->value(desc.gravityFactor);
       s->member("linearVelocity");  s->value(desc.linearVelocity);
       s->member("angularVelocity"); s->value(desc.angularVelocity);
       s->member("motionQuality");   s->value(motionQuality);
       s->member("allowedDOFs");     s->value(allowedDOFs);
       s->member("motionType");      s->value(motionType);
       s->member("layer");           s->value(desc.layer);
       s->member("activation");      s->value(desc.activation);
       s->member("allowSleeping");   s->value(desc.allowSleeping);
       return 0;
   });


   ecs.component<WorldMatrix>().add<DontSerialize>();       // recomputed by transform propagation
   ecs.component<ContactDataList>().add<DontSerialize>();   // per-frame contact buffer
   ecs.component<MeshComponent>().add<DontSerialize>();     // runtime geometry-buffer indices
   ecs.component<SubMeshComponent>().add<DontSerialize>();
   ecs.component<JoltCharacter>().add<DontSerialize>();     // owning pointer to a Jolt object
   ecs.component<ActorDebugInfo>().add<DontSerialize>();


}

