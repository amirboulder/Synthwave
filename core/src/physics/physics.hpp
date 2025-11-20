#pragma once

#include "core/src/pch.h"

#include "debugRenderer.hpp"

#include "../ecs/components.hpp"


// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is XX or not.
using namespace JPH::literals;


// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	std::cout << buffer << '\n';
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
{
	// Print to the TTY
	std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << '\n';

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer					mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool				ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// An example contact listener
class MyContactListener : public ContactListener
{
public:

	flecs::world& ecs;


	MyContactListener(flecs::world& ecs)
		:ecs(ecs)
	{

	}

	// See: ContactListener
	virtual ValidateResult	OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override
	{
		//cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{

		// If either body is sensor then call their SensorBehavior
		flecs::entity e1(ecs, (flecs::entity_t)inBody1.GetUserData());
		flecs::entity e2(ecs, (flecs::entity_t)inBody2.GetUserData());

		if (e1.has<SensorBehavior>())
		{
			e1.get<SensorBehavior>().onContactAdded(ecs, e2, e1);
		}

		if (e2.has<SensorBehavior>())
		{
			e2.get<SensorBehavior>().onContactAdded(ecs, e2, e1);
		}


	}

	virtual void			OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override
	{
		//cout << "A contact was persisted" << endl;
	}

	virtual void			OnContactRemoved(const SubShapeIDPair& inSubShapePair) override
	{
		//cout << "A contact was removed" << std::endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void		OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		//cout << "A body got activated" << '\n';
	}

	virtual void		OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override
	{
		//cout << "A body went to sleep" << '\n';
	}
};


class Fisiks {

public:

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
	BPLayerInterfaceImpl broad_phase_layer_interface;

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
	ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	// Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
	ObjectLayerPairFilterImpl object_vs_object_layer_filter;

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	MyBodyActivationListener body_activation_listener;

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	MyContactListener contact_listener;

	PhysicsSystem physicsSystem;

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	BodyInterface& bodyInterface;

	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	const int cCollisionSteps = 1;

	const float timeStep = 1.0f / 60.0f;

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update.
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	TempAllocatorImpl* temp_allocator;

	JobSystemThreadPool* job_system;

	flecs::world& ecs;

	flecs::query<Transform, JPH::BodyID, DynamicEnt>q1;

	flecs::system updateSys;
	flecs::system syncSys;

	flecs::entity physicsPhase;
	flecs::entity physicsRenderPhase;

	Fisiks(flecs::world& ecs)
		: broad_phase_layer_interface(),
		object_vs_broadphase_layer_filter(),
		object_vs_object_layer_filter(),
		body_activation_listener(),
		contact_listener(ecs),
		physicsSystem(),
		bodyInterface(physicsSystem.GetBodyInterface()),
		ecs(ecs)



	{
		// Register allocation hook. In this example we'll just let Jolt use 
		// malloc / free but you can override these if you want (see Memory.h).
		// This needs to be done before any other Jolt function is called.
		RegisterDefaultAllocator();

		// Install trace and assert callbacks
		Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

			// Create a factory, this class is responsible for creating instances 
			// of classes based on their name or hash and is mainly used for deserialization of saved data.
			// It is not directly used in this example but still required.
			Factory::sInstance = new Factory();

		// Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
		// If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
		// If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
		RegisterTypes();

		temp_allocator = new TempAllocatorImpl(10 * 1024 * 1024);

		job_system = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);


		// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const uint cMaxBodies = 1024;

		// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
		const uint cNumBodyMutexes = 0;

		// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
		// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
		// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
		const uint cMaxBodyPairs = 1024;

		// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
		// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
		const uint cMaxContactConstraints = 1024;


		// Now we can initialize the actual physics system.
		physicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);


		// A body activation listener gets notified when bodies activate and go to sleep
		physicsSystem.SetBodyActivationListener(&body_activation_listener);

		// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
		physicsSystem.SetContactListener(&contact_listener);


		// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance
		// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
		// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
		physicsSystem.OptimizeBroadPhase();

		physicsSystem.SetGravity(JPH::Vec3(0, -20.0f, 0));

		init();

	}

	void init() {

		registerComponents();
		registerPhase();
		registerSystems();

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "FISIKS Initialized");
	}


	void registerSystems() {

		//Creation order determines the order in which these systems run within a phase
		updateSystem();
		syncSystem();


	}

	void registerComponents() {

		ecs.component<PhysicsSystemRef>("PhysicsSystemRef");
		ecs.emplace<PhysicsSystemRef>(physicsSystem);

	}

	void registerPhase() {

		// Each phase has its own dependency, it ensures that
		// 1.phases can be disabled without affecting other phases (disabling is transitive in flecs)
		// 2.Phases can run in the order we want regardless of creation order 
		//PhaseDependencies depend on each other, that's handled in StateManager.RegisterPhaseDependencies()
		// that way phases created earlier in initialization can depend on phases created after them
		flecs::entity physicsPhaseDependency = ecs.entity("PhysicsPhaseDependency");

		physicsPhase = ecs.entity("PhysicsPhase")
			.add(flecs::Phase)
			.depends_on(physicsPhaseDependency);

		// disabled by default so that we don't start simulating physics until a level is loaded
		physicsPhase.disable();


	}

	// Update system is part of the physics phase
	void updateSystem() {

		updateSys = ecs.system("PhysicsUpdateSys")
			.kind(physicsPhase)
			.run([&](flecs::iter& it) {

			physicsSystem.Update(timeStep, cCollisionSteps, temp_allocator, job_system);

		});

	}


	//Sync system is part of the physicsPhase
	void syncSystem() {

		syncSys = ecs.system<Transform, JPH::BodyID, DynamicEnt>("PhysicsSyncSys")
			.kind(physicsPhase)
			.each([&](flecs::entity e, Transform& transform, JPH::BodyID& physicsBody, DynamicEnt) {

			JPH::Vec3 pos;
			JPH::Quat rotation;
			bodyInterface.GetPositionAndRotation(physicsBody, pos, rotation);

			//transform.position = *reinterpret_cast<glm::vec3*>(&pos);
			transform.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());

			//transform.rotation = *reinterpret_cast<glm::quat*>(&rotation);
			transform.rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

		});
	}


	//TODO move these to physicsUtil
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


	~Fisiks() {

		// Unregisters all types with the factory and cleans up the default material
		UnregisterTypes();

		// Destroy the factory
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}
};


