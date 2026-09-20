//This file is AI generated.

module;


//=============================================================================
// Global module fragment.
//
// Jolt is built as a separate static library (target "Jolt"), so its headers
// MUST be included here, above "export module", not in the module purview.
// That keeps every Jolt entity attached to the global module and linking
// against the prebuilt Jolt.lib. Including them below "export module Jolt;"
// would attach them to this module and produce unresolved externals.
//
// Consequence: nothing here is visible to importers by itself. Everything the
// project needs is re-exported by name in the "export namespace JPH" block at
// the bottom. If you start using a new Jolt type, add its header here AND a
// using-declaration below.
//
// MACROS DO NOT CROSS A MODULE BOUNDARY.
// "import Jolt;" gives you types, not macros. JPH_ASSERT, JPH_NAMESPACE_BEGIN/
// END, JPH_OVERRIDE_NEW_DELETE and JPH_SUPPRESS_WARNINGS all live in
// Jolt/Core/Core.h and are NOT available to importers. Worse, JPH_ENABLE_ASSERTS
// is also defined by that header, so an "#ifdef JPH_ENABLE_ASSERTS" block in an
// importing file silently compiles to nothing instead of erroring.
//
// A file that needs those macros must keep a plain include alongside the import:
//
//     #include <Jolt/Jolt.h>   // macros only; types still come from the module
//     import Jolt;
//
// That combination is fine - the header's entities and this module's exported
// using-declarations name the same global-module entities, so there is no
// redefinition. (Verified: compiles with zero warnings.)
//
// Only these Jolt macros arrive on the compiler command line (target_compile_
// definitions on the Jolt target) and therefore work everywhere without the
// include: JPH_DEBUG_RENDERER, JPH_OBJECT_STREAM, JPH_PROFILE_ENABLED,
// JPH_CROSS_PLATFORM_DETERMINISTIC, JPH_OBJECT_LAYER_BITS.
//=============================================================================




#include <Jolt/Jolt.h>

// --- Core -------------------------------------------------------------------
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Array.h>
#include <Jolt/Core/Atomics.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/RTTI.h>
#include <Jolt/Core/NonCopyable.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/StreamWrapper.h>

// --- Math / Geometry --------------------------------------------------------
#include <Jolt/Math/Math.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Float4.h>
#include <Jolt/Math/UVec4.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Vec4.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Mat44.h>
#include <Jolt/Geometry/AABox.h>
#include <Jolt/Geometry/Plane.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Geometry/IndexedTriangle.h>

// --- Physics system ---------------------------------------------------------
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/BodyManager.h>

// --- Collision --------------------------------------------------------------
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Collision/EstimateCollisionResponse.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>

// --- Shapes -----------------------------------------------------------------
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

// --- Constraints ------------------------------------------------------------
#include <Jolt/Physics/Constraints/MotorSettings.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/PathConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/ConeConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

// --- Character --------------------------------------------------------------
#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Character/CharacterID.h>

// --- Ragdoll / Skeleton -----------------------------------------------------
#include <Jolt/Physics/Ragdoll/Ragdoll.h>
#include <Jolt/Skeleton/Skeleton.h>
#include <Jolt/Skeleton/SkeletalAnimation.h>
#include <Jolt/Skeleton/SkeletonPose.h>

// --- ObjectStream (serialization of .tof ragdoll / animation assets) --------
#ifdef JPH_OBJECT_STREAM
	#include <Jolt/ObjectStream/ObjectStreamIn.h>
	#include <Jolt/ObjectStream/ObjectStreamOut.h>
#endif

// --- Debug renderer ---------------------------------------------------------
#ifdef JPH_DEBUG_RENDERER
	#include <Jolt/Renderer/DebugRenderer.h>
#endif


export module Jolt;


//=============================================================================
// Re-export.
//
// Re-opening namespace JPH and naming each entity with a using-declaration is
// the only way to make global-module entities visible to importers. The names
// below are exactly the ones the project uses today (see physics.hpp,
// ragdoll.hpp, debugRenderer.hpp, physicsUtil.hpp, Biped.hpp, PhysicsComponents.hpp,
// EntityFactory.ixx, player.hpp) plus the settings/base types that come with them.
//
// Not listed and not needed:
//  - member operators (Vec3::operator*, Quat::operator*, ...) come with the class.
//  - free operators such as operator*(float, Vec3Arg) are declared as hidden
//    friends inside the class, so ADL finds them once the class is exported.
//  - nested names (BodyManager::DrawSettings, DebugRenderer::Triangle/Vertex/LOD/
//    GeometryRef/Batch/ECastShadow/ECullMode/EDrawMode, CharacterBase::EGroundState,
//    RagdollSettings::Part, SixDOFConstraintSettings::EAxis) come with their
//    enclosing class.
//=============================================================================

export namespace JPH {

	//--- Scalar aliases ------------------------------------------------------
	using JPH::uint;
	using JPH::uint8;
	using JPH::uint16;
	using JPH::uint32;
	using JPH::uint64;
	using JPH::Real;

	// Jolt re-exports these two from <atomic> / <thread> inside namespace JPH.
	// debugRenderer.hpp uses atomic<uint32>, physics.hpp uses thread::hardware_concurrency().
	// (Preferring std::atomic / std::thread at those call sites would be cleaner.)
	using JPH::atomic;
	using JPH::thread;

	//--- Free operators that ADL CANNOT reach --------------------------------
	// Most of Jolt's free operators (e.g. operator*(float, Vec3Arg)) are declared
	// as hidden friends inside the class, so ADL finds them once the class is
	// exported. The bitflag operators for EAllowedDOFs (Physics/Body/AllowedDOFs.h)
	// and EStateRecorderState (Physics/StateRecorder.h) are NOT - they are plain
	// namespace-scope functions, invisible to importers unless named here.
	//
	// Without these, CharacterSettings fails to construct: its default member
	// initializer is
	//     mAllowedDOFs = EAllowedDOFs::TranslationX | ... (Character.h:39)
	// which yields "C2678: binary '|': no operator found" at every use site,
	// reported inside Jolt's headers rather than in your own code.
	//
	// Naming the operator exports every overload of it in namespace JPH, so this
	// also covers any bitflag enum Jolt adds later.
	using JPH::operator|;
	using JPH::operator&;
	using JPH::operator^;
	using JPH::operator~;
	using JPH::operator|=;
	using JPH::operator&=;
	using JPH::operator^=;

	//--- Intermediate base classes -------------------------------------------
	// A class is only complete if its bases are. These are never named by the
	// project directly, but every derived type exported below needs them, so
	// they must appear here or the derived type arrives incomplete.
	using JPH::NonCopyable;

	//--- Core containers / smart pointers ------------------------------------
	using JPH::Array;
	using JPH::Ref;
	using JPH::RefConst;
	using JPH::RefTarget;
	using JPH::RefTargetVirtual;
	using JPH::Result;
	using JPH::String;

	//--- Core services -------------------------------------------------------
	using JPH::Factory;
	using JPH::RTTI;
	using JPH::DynamicCast;
	using JPH::RegisterDefaultAllocator;
	using JPH::RegisterTypes;
	using JPH::UnregisterTypes;

	using JPH::TempAllocator;
	using JPH::TempAllocatorImpl;
	using JPH::TempAllocatorMalloc;
	using JPH::JobSystem;
	using JPH::JobSystemWithBarrier;   // base of JobSystemThreadPool
	using JPH::JobSystemThreadPool;

	using JPH::StreamIn;
	using JPH::StreamOut;
	using JPH::StreamInWrapper;
	using JPH::StreamOutWrapper;

	//--- Trace / assert plumbing ---------------------------------------------
	// physics.hpp assigns Trace = TraceImpl and AssertFailed = AssertFailedImpl.
	// AssertLastParam / AssertFailedParamHelper are what the JPH_ASSERT macro
	// expands to, so they must be visible wherever that macro is used.
	using JPH::TraceFunction;
	using JPH::Trace;
#ifdef JPH_ENABLE_ASSERTS
	using JPH::AssertFailedFunction;
	using JPH::AssertFailed;
	using JPH::AssertLastParam;
	using JPH::AssertFailedParamHelper;
#endif

	//--- Math ----------------------------------------------------------------
	using JPH::Vec3;
	using JPH::Vec3Arg;
	using JPH::Vec4;
	using JPH::Vec4Arg;
	using JPH::Quat;
	using JPH::QuatArg;
	using JPH::Mat44;
	using JPH::Mat44Arg;
	using JPH::RVec3;
	using JPH::RVec3Arg;
	using JPH::RMat44;
	using JPH::RMat44Arg;
	using JPH::Float2;
	using JPH::Float3;
	using JPH::Float4;
	using JPH::Color;
	using JPH::ColorArg;

	using JPH::DegreesToRadians;
	using JPH::RadiansToDegrees;

	// WARNING C5304 on the next three.
	// Jolt declares JPH_PI / cMaxPhysicsJobs / cMaxPhysicsBarriers as namespace-
	// scope "static constexpr" / "constexpr", which gives them INTERNAL linkage,
	// and an exported using-declaration may not name an internal-linkage entity.
	// MSVC accepts it anyway (the constant folds into the .ifc) and consumers
	// compile clean, but it is ill-formed per [module.interface] and clang will
	// reject it. If this project ever moves to clang, replace these with
	// project-owned constants or patch Jolt to declare them "inline constexpr".
	using JPH::JPH_PI;

	//--- Geometry ------------------------------------------------------------
	using JPH::AABox;
	using JPH::Plane;
	using JPH::Triangle;
	using JPH::IndexedTriangle;
	using JPH::IndexedTriangleList;
	using JPH::VertexList;

	//--- Physics system ------------------------------------------------------
	using JPH::PhysicsSystem;
	using JPH::PhysicsSettings;
	using JPH::cMaxPhysicsJobs;      // see C5304 note above
	using JPH::cMaxPhysicsBarriers;  // see C5304 note above

	using JPH::Body;
	using JPH::BodyID;
	using JPH::BodyInterface;
	using JPH::BodyCreationSettings;
	using JPH::BodyManager;
	using JPH::BodyActivationListener;
	using JPH::BodyLockInterface;
	using JPH::BodyLockRead;
	using JPH::BodyLockWrite;
	using JPH::MassProperties;

	using JPH::EActivation;
	using JPH::EMotionType;
	using JPH::EMotionQuality;
	using JPH::EAllowedDOFs;
	using JPH::EOverrideMassProperties;

	//--- Layers and filters --------------------------------------------------
	using JPH::ObjectLayer;
	using JPH::ObjectLayerFilter;
	using JPH::ObjectLayerPairFilter;
	using JPH::DefaultObjectLayerFilter;
	using JPH::BroadPhaseLayer;
	using JPH::BroadPhaseLayerFilter;
	using JPH::BroadPhaseLayerInterface;
	using JPH::DefaultBroadPhaseLayerFilter;
	using JPH::ObjectVsBroadPhaseLayerFilter;

	using JPH::BodyFilter;
	using JPH::ShapeFilter;
	using JPH::IgnoreSingleBodyFilter;
	using JPH::IgnoreMultipleBodiesFilter;
	using JPH::IgnoreSingleBodyFilterChained;

	using JPH::CollisionGroup;
	using JPH::GroupFilter;
	using JPH::GroupFilterTable;

	//--- Contacts ------------------------------------------------------------
	using JPH::ContactListener;
	using JPH::ContactManifold;
	using JPH::ContactSettings;
	using JPH::ContactPoints;
	using JPH::ValidateResult;
	using JPH::CollideShapeResult;
	using JPH::CollisionDispatch;
	using JPH::CollisionEstimationResult;
	using JPH::EstimateCollisionResponse;
	using JPH::SubShapeID;
	using JPH::SubShapeIDPair;

	//--- Queries -------------------------------------------------------------
	using JPH::NarrowPhaseQuery;
	using JPH::TransformedShape;
	using JPH::RayCast;
	using JPH::RRayCast;
	using JPH::RayCastResult;
	using JPH::RayCastSettings;

	//--- Shapes --------------------------------------------------------------
	using JPH::Shape;
	using JPH::ConvexShape;
	using JPH::DecoratedShape;
	using JPH::ShapeSettings;
	using JPH::ShapeRefC;
	using JPH::PhysicsMaterial;
	using JPH::PhysicsMaterialList;

	using JPH::BoxShape;
	using JPH::BoxShapeSettings;
	using JPH::SphereShape;
	using JPH::SphereShapeSettings;
	using JPH::CapsuleShape;
	using JPH::CapsuleShapeSettings;
	using JPH::CylinderShape;
	using JPH::CylinderShapeSettings;
	using JPH::MeshShape;
	using JPH::MeshShapeSettings;
	using JPH::EmptyShape;
	using JPH::EmptyShapeSettings;
	using JPH::ScaledShape;
	using JPH::ScaledShapeSettings;
	using JPH::StaticCompoundShape;
	using JPH::StaticCompoundShapeSettings;

	//--- Constraints ---------------------------------------------------------
	using JPH::Constraint;
	using JPH::ConstraintSettings;
	using JPH::TwoBodyConstraint;
	using JPH::TwoBodyConstraintSettings;
	using JPH::EConstraintSpace;
	using JPH::EConstraintSubType;
	using JPH::EMotorState;
	using JPH::MotorSettings;

	using JPH::PointConstraint;
	using JPH::PointConstraintSettings;
	using JPH::PathConstraint;
	using JPH::PathConstraintSettings;
	using JPH::DistanceConstraint;
	using JPH::DistanceConstraintSettings;
	using JPH::FixedConstraint;
	using JPH::FixedConstraintSettings;
	using JPH::HingeConstraint;
	using JPH::HingeConstraintSettings;
	using JPH::SliderConstraint;
	using JPH::SliderConstraintSettings;
	using JPH::ConeConstraint;
	using JPH::ConeConstraintSettings;
	using JPH::SwingTwistConstraint;
	using JPH::SwingTwistConstraintSettings;
	using JPH::SixDOFConstraint;
	using JPH::SixDOFConstraintSettings;

	//--- Character -----------------------------------------------------------
	using JPH::CharacterBase;
	using JPH::CharacterBaseSettings;
	using JPH::Character;
	using JPH::CharacterSettings;
	using JPH::CharacterVirtual;
	using JPH::CharacterVirtualSettings;
	using JPH::CharacterContactListener;
	using JPH::CharacterContactSettings;
	using JPH::CharacterID;
	using JPH::CharacterVsCharacterCollisionSimple;

	//--- Ragdoll / Skeleton --------------------------------------------------
	using JPH::Ragdoll;
	using JPH::RagdollSettings;
	using JPH::Skeleton;
	using JPH::SkeletalAnimation;
	using JPH::SkeletonPose;

	//--- ObjectStream --------------------------------------------------------
#ifdef JPH_OBJECT_STREAM
	using JPH::ObjectStreamIn;
	using JPH::ObjectStreamOut;
#endif

	//--- Debug renderer ------------------------------------------------------
#ifdef JPH_DEBUG_RENDERER
	using JPH::DebugRenderer;
#endif

	using JPH::EBackFaceMode;

	//--- 0.0_r literal (physics.hpp does "using namespace JPH::literals") ----
	namespace literals {
		using JPH::literals::operator ""_r;
	}
}
