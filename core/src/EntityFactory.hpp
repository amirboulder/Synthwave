#pragma once

#include "../../core/src/AssetSystems/AssetLibrary.hpp"

//Maybe Use this everywhere
using entUpdateFn = std::function<void(flecs::world&, flecs::entity)>;


/// <summary>
/// All member functions are static so other systems don't need to instantiate the class in order to use them.
/// Used for creating various entity types that the engine supports,
/// does a lot of error handling since we getting input from the user and users can't be trusted!
/// </summary>
class EntityFactory {

private:

	// Private constructor to prevent instantiation
	EntityFactory() = delete;
	~EntityFactory() = delete;

public:

	//Creates a capsule shaped entity
	static bool createCapsuleEntity(flecs::world& ecs, const flecs::entity parent,
		std::string_view name, const Transform transform) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;

		MeshComponent meshComp = assetManager->requestMeshComponent(assetManager->defaultAssetsMap.at(DefaultAssets::CAPSULE));
		glm::vec3 scaledSize = meshComp.aabb.extents * transform.scale;

		float physicsRadius = scaledSize.x;
		float physicsHalfHeight = scaledSize.y;

		//the half-height parameter in Jolt is the half-height of the cylindrical center section only
		float physicsCylHalf = scaledSize.y - physicsRadius; 

		if (physicsCylHalf <= 0) {
			LogError(LOG_APP, "physicsCylHalf is zero");
			return false;
		}

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> capsuleShape = new JPH::CapsuleShape(physicsCylHalf, physicsRadius);

		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::BodyCreationSettings pillSettings(
			capsuleShape,
			joltPosition,
			joltRotation,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		// bounciness
		pillSettings.mRestitution = 0.5f;

		pillSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		pillSettings.mMassPropertiesOverride.mMass = 50.1f;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		// Create and add body
		const BodyID physicsID = bodyInterface.CreateAndAddBody(pillSettings, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;

		flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Capsule })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent)
			;


		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data()))  return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManager, entity, meshComp, name)) return false;

		return true;
	}

	static bool createCubeEntity(flecs::world& ecs, const flecs::entity parent, std::string_view name,
		const Transform transform) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;
		
		//Assuming default asset exists
		MeshComponent meshComp = assetManager->requestMeshComponent(assetManager->defaultAssetsMap.at(DefaultAssets::CUBE));

		glm::vec3 scaledSize = meshComp.aabb.extents * transform.scale;

		Vec3 boxHalfExtents(scaledSize.x , scaledSize.y, scaledSize.z );

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// Convert GLM to Jolt types
		JPH::Vec3 joltPos(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRot.IsNormalized()) {
			joltRot = joltRot.Normalized();
		}

		JPH::BodyCreationSettings bodySettings(boxShape, joltPos, joltRot,JPH::EMotionType::Dynamic,Layers::MOVING);

		// bounciness
		bodySettings.mRestitution = 0.5f;

		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = 50.1f;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		// Create and add body
		const BodyID physicsID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;


		const flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Cube })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent)
			;


		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data()))  return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManager, entity, meshComp, name)) return false;

		return true;

	}

	static bool createSphereEntity(flecs::world& ecs, const flecs::entity parent, std::string_view name,
		const Transform transform) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;

		//Assuming default asset exists
		MeshComponent meshComp = assetManager->requestMeshComponent(assetManager->defaultAssetsMap.at(DefaultAssets::SPHERE));

		glm::vec3 scaledSize = meshComp.aabb.extents * transform.scale;

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> shape = new SphereShape(scaledSize.x); //Assuming uniform scaling

		// Convert GLM to Jolt types
		JPH::Vec3 joltPos(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRot.IsNormalized()) {
			joltRot = joltRot.Normalized();
		}

		JPH::BodyCreationSettings bodySettings(shape, joltPos, joltRot, JPH::EMotionType::Dynamic, Layers::MOVING);

		// bounciness
		bodySettings.mRestitution = 0.5f;

		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = 50.1f;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		// Create and add body
		const BodyID physicsID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;


		const flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Cube })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent)
			;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data()))  return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManager, entity, meshComp, name)) return false;

		return true;
	}

	static bool createCylinderEntity(flecs::world& ecs, const flecs::entity parent, std::string_view name,
		const Transform transform) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;
		MeshComponent meshComp = assetManager->requestMeshComponent(assetManager->defaultAssetsMap.at(DefaultAssets::CYLINDER));

		glm::vec3 scaledSize = meshComp.aabb.extents * transform.scale;

		// Compute Cylinder dimensions
		float physicsRadius = scaledSize.x;
		float physicsHalfHeight = scaledSize.y;

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> shape = new CylinderShape(physicsHalfHeight, physicsRadius); 

		// Convert GLM to Jolt types
		JPH::Vec3 joltPos(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRot.IsNormalized()) {
			joltRot = joltRot.Normalized();
		}

		JPH::BodyCreationSettings bodySettings(shape, joltPos, joltRot, JPH::EMotionType::Dynamic, Layers::MOVING);

		// bounciness
		bodySettings.mRestitution = 0.5f;

		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = 100.0f;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		// Create and add body
		const BodyID physicsID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;

		const flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Cylinder })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent)
			;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data()))  return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManager, entity, meshComp, name)) return false;

		return true;
	}

	static bool createBoxCarEntity(flecs::world& ecs, const flecs::entity parent, std::string_view name,
		const Transform transform) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;
	
		uint64_t modelID = assetManager->defaultAssetsMap.at(DefaultAssets::BOXCAR);

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		flecs::entity  rootEntity= ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::BoxCar })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.child_of(parent)
			;


		if (!validateEntityCreation(rootEntity, name.data()))  return false;

		ModelData model = assetManager->requestModel(modelID);
		MeshNode rootNode = assetManager->requestMeshNode(model.rootNodeID);

		if (!createMeshHierarchy(ecs, rootEntity, rootNode, name, assetManager)) return false;

		/*
		 StaticCompoundShapeSettings settings;

		
		for (const Mesh& mesh : modelSource->meshes) {

			Mesh::calculateMeshSize(mesh.vertices);

			Vec3 boxHalfExtents(meshX * 0.5, meshY * 0.5, meshZ * 0.5);
			
			//For now just use a box for everything
			Ref<BoxShapeSettings> boxShapeSettings = new BoxShapeSettings(boxHalfExtents);

			JPH::Vec3 joltPos(mesh.transform.position.x, mesh.transform.position.y, mesh.transform.position.z);
			JPH::Quat joltRot(mesh.transform.rotation.x, mesh.transform.rotation.y, mesh.transform.rotation.z, mesh.transform.rotation.w);
			if (!joltRot.IsNormalized()) {
				joltRot = joltRot.Normalized();
			}

			settings.AddShape(joltPos, joltRot, boxShapeSettings, (uint32_t)entity.id());

		}
		


		Result<Ref<Shape>> shapeResult = settings.Create();

		// Convert GLM to Jolt types
		JPH::Vec3 joltPos(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRot(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

		BodyCreationSettings bodySettings(
			shapeResult.Get(),
			joltPos,
			joltRot,
			EMotionType::Dynamic,
			Layers::MOVING
		);

		const BodyID physicsID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

		rootEntity.set<JPH::BodyID>(physicsID);

		*/

		return true;
	}

	static bool createHumanRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
	const Transform transform, entUpdateFn updateFunction) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;

		////Get the modelSource from Asset Library
		//AssetLibRef ref = ecs.get<AssetLibRef>();
		//ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		//if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		Ref<RagdollSettings> ragdollSettings = RagdollLoader::create(2.0f);

		if (!ragdollSettings) {
			LogError(LOG_PHYSICS, "ragdollSettings is null for entity %s", name.c_str());
			return false;
		}

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Ragdoll })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<AnimationTime>({})
			//.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::SkeletalAnimation *  mAnimation;
		JPH::SkeletonPose *		  mPose =  new JPH::SkeletonPose;

		JPH::Ragdoll* ragdoll = ragdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);

		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {
			if (!validatePhysicsBodyCreation(id, name)) return false;
		}

		entity.set<JoltRagdoll>({ ragdoll });

		return true;
	}

	//creates jolts Human.tof 
	static bool createHumanTOFRagdollEntity(
		flecs::world& ecs,
		const flecs::entity parent, 
		const std::string name,
		const Transform transform, 
		entUpdateFn updateFunction) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

	
		constexpr float ragdollScale = 3.0f;

		Ref<RagdollSettings> ragdollSettings =
			RagdollLoader::load("assets/ragdolls/Human.tof", EMotionType::Dynamic, ragdollScale);

		if (!ragdollSettings) {
			LogError(LOG_PHYSICS, "ragdollSettings is null for entity %s", name.c_str());
			return false;
		}

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Ragdoll })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<EnemyState>({ EnemyState::SLEEP})
			.set<AnimationTime>({})
			.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::SkeletonPose mPose;
		JPH::Ragdoll* ragdoll = ragdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
		ragdoll->SetGroupID(static_cast<uint32>(entity.id()));

		JPH::AABox ragdollAABox = Utils::Phys::getRagdollBoundingBox(ragdoll, bi);

		// Load animation (same scale as ragdoll so pose bone offsets match body positions)
		JPH::SkeletalAnimation* mAnimation =
			AnimationLoader::load("assets/ragdolls/neutral.tof", ragdollScale);
		if (!mAnimation) {
			LogError(LOG_PHYSICS, "failed loading animation for entity %s", name.c_str());
			return false;
		}

		// Initialize pose
		mPose.SetSkeleton(ragdollSettings->GetSkeleton());
		//mAnimation->Sample(0.0f, mPose); //Setting the pose to this makes it a valid pose

		RVec3 desiredPos(transform.position.x, transform.position.y, transform.position.z);
		Quat   desiredRot = Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

		Utils::Phys::MoveAndRotateRagdoll(ragdoll, bi, desiredPos, desiredRot, JPH::EActivation::Activate);
		float HipsFromSolesDist = Utils::Phys::getHipsFromSolesDist(ragdoll, ragdollSettings->GetSkeleton(), bi);


		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {
			if (!validatePhysicsBodyCreation(id, name)) return false;
		}

		float xExtent = ragdollAABox.mMax.GetX() - ragdollAABox.mMin.GetX();
		float zExtent = ragdollAABox.mMax.GetZ() - ragdollAABox.mMin.GetZ();
		float totalHeight = ragdollAABox.mMax.GetY() - ragdollAABox.mMin.GetY();

		//Taking the average of x and z here because the ragdoll is in T-Pose
		// but the capsule could have a tighter shape if we build a bounding box without the arms.
		float characterRadius = ((xExtent + zExtent) * 0.5f) * 0.5f;
		float characterCylinderHalfHeight = std::max(0.01f, (totalHeight - characterRadius * 2.0f) * 0.5f);

		float cylHalfHeight = HipsFromSolesDist - characterRadius;

		Quat rootRot = bi.GetRotation(ragdoll->GetBodyID(0));
		LogInfo(LOG_APP, "Hip Rotation COM : %f %f %f %f", rootRot.GetX(), rootRot.GetY(), rootRot.GetZ(), rootRot.GetW());

		// Character settings
		JPH::CharacterSettings characterSettings;
		characterSettings.mShape = new CapsuleShape(cylHalfHeight, characterRadius);
		characterSettings.mMass = 20.0f;
		characterSettings.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		characterSettings.mLayer = Layers::CHARACTER_ANCHOR;
		characterSettings.mGravityFactor = 1;

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::Character* joltCharacter = new JPH::Character(
			&characterSettings, joltPosition, joltRotation, 0, &physicsSystem
		);

		if (!validatePhysicsBodyCreation(joltCharacter->GetBodyID(), name.data())) {
			delete joltCharacter;
			return false;
		}

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);
		physicsSystem.GetBodyInterface().SetUserData(joltCharacter->GetBodyID(), entity.id());

		////////////////////////////////////////////////////////
		//Create a constraint between hip and character

		JPH::BodyID characterBodyID = joltCharacter->GetBodyID();
		JPH::BodyID hipBodyID = ragdoll->GetBodyID(0);

		// Hip anchor: spring hip COM to character COM. Leave rotation free so pose
		// motors own hip orientation (hip body frame != upright capsule frame).
		SixDOFConstraintSettings settings;
		settings.mSpace = EConstraintSpace::LocalToBodyCOM;
		settings.mPosition1 = Vec3::sZero(); // character COM
		settings.mPosition2 = Vec3::sZero(); // hip COM

		float maxHoldForce = 1000000.0f;

		float limXZ = 0.1f;

		settings.SetLimitedAxis(SixDOFConstraintSettings::EAxis::TranslationX, -limXZ, limXZ);
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationX] = MotorSettings(2.0f, 1.0f);
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationX].mMinForceLimit = -maxHoldForce;
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationX].mMaxForceLimit = maxHoldForce;

		settings.SetLimitedAxis(SixDOFConstraintSettings::EAxis::TranslationY, -HipsFromSolesDist, HipsFromSolesDist );
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationY] = MotorSettings(2.0f, 1.0f);
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationY].mMinForceLimit = -maxHoldForce;
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationY].mMaxForceLimit = maxHoldForce;

		settings.SetLimitedAxis(SixDOFConstraintSettings::EAxis::TranslationZ, -limXZ, limXZ);
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationZ] = MotorSettings(2.0f, 1.0f);
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationZ].mMinForceLimit = -maxHoldForce;
		settings.mMotorSettings[SixDOFConstraintSettings::EAxis::TranslationZ].mMaxForceLimit = maxHoldForce;

		//setupTranslationAxis(SixDOFConstraintSettings::EAxis::TranslationX);
		//setupTranslationAxis(SixDOFConstraintSettings::EAxis::TranslationY);
		//setupTranslationAxis(SixDOFConstraintSettings::EAxis::TranslationZ);

		settings.MakeFixedAxis(SixDOFConstraintSettings::EAxis::RotationX);
		//settings.MakeFixedAxis(SixDOFConstraintSettings::EAxis::RotationY);
		settings.MakeFixedAxis(SixDOFConstraintSettings::EAxis::RotationZ);

		//settings.MakeFreeAxis(SixDOFConstraintSettings::EAxis::RotationX);
		//settings.MakeFreeAxis(SixDOFConstraintSettings::EAxis::RotationY);
		//settings.MakeFreeAxis(SixDOFConstraintSettings::EAxis::RotationZ);

		/*
		auto setupRotationAxis = [&](SixDOFConstraintSettings::EAxis axis) {
			settings.SetLimitedAxis(axis, 0.0f, 0.0f);
			settings.mMotorSettings[axis] = MotorSettings(2.0f, 1.0f);
			settings.mMotorSettings[axis].mMinTorqueLimit = -maxHoldForce;
			settings.mMotorSettings[axis].mMaxTorqueLimit = maxHoldForce;
		};

		setupRotationAxis(SixDOFConstraintSettings::EAxis::RotationX);
		setupRotationAxis(SixDOFConstraintSettings::EAxis::RotationY);
		setupRotationAxis(SixDOFConstraintSettings::EAxis::RotationZ);
		*/


		Ref<SixDOFConstraint> hipConstraint = dynamic_cast<SixDOFConstraint*>(bi.CreateConstraint(&settings, characterBodyID, hipBodyID));

		physicsSystem.AddConstraint(hipConstraint);

		entity.set<PhysicsConstraint>({ hipConstraint });
		entity.set<JoltRagdoll>({ ragdoll });
		entity.set<JoltPose>({ mPose });
		entity.set<JoltAnimation>({ mAnimation });
		entity.set<JoltCharacter>({ joltCharacter });

		return true;
	}


	//creates jolts Human.tof 
	static bool createRagdollEntityForce(
		flecs::world& ecs,
		const flecs::entity parent,
		const std::string name,
		const Transform transform,
		entUpdateFn updateFunction) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();

		
		constexpr float ragdollScale = 3.0f;

		Ref<RagdollSettings> ragdollSettings =
			RagdollLoader::load("assets/ragdolls/Human.tof", EMotionType::Dynamic, ragdollScale);

		if (!ragdollSettings) {

			LogError(LOG_PHYSICS, "ragdollSettings is null for entity %s", name.c_str());
			return false;
		}

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Ragdoll })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<AnimationTime>({})
			.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;



		JPH::SkeletonPose mPose;

		JPH::Ragdoll* ragdoll = ragdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
		ragdoll->SetGroupID(static_cast<uint32>(entity.id()));

		cout << "Ragdoll body count : " << ragdoll->GetBodyCount() << std::endl;
		cout << "Ragdoll GetConstraintCount : " << ragdoll->GetConstraintCount() << std::endl;
		cout << "GetSkeleton GetJointCount : " << ragdollSettings->GetSkeleton()->GetJointCount() << std::endl;

		// Load animation (same scale as ragdoll so pose bone offsets match body positions)
		JPH::SkeletalAnimation* mAnimation =
			AnimationLoader::load("assets/ragdolls/neutral.tof", ragdollScale);
		if (!mAnimation) {
			LogError(LOG_PHYSICS, "failed loading animation for entity %s", name.c_str());
			return false;
		}

		// Initialize pose
		mPose.SetSkeleton(ragdollSettings->GetSkeleton());
		mAnimation->Sample(0.0f, mPose); //Setting the pose to this makes it a valid pose
		JPH::Vec3 rootOffset = mPose.GetRootOffset();


		
		RVec3 desiredPos(transform.position.x, transform.position.y, transform.position.z);
		Quat   desiredRot = Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

		BodyID rootID = ragdoll->GetBodyID(0);
		RVec3  currentRoot = bi.GetPosition(rootID);
		RVec3  delta = desiredPos - currentRoot;
		for (BodyID id : ragdoll->GetBodyIDs()) {
			RVec3 p = bi.GetPosition(id);
			Quat  q = bi.GetRotation(id);
			bi.SetPositionAndRotation(id, p + delta,
				desiredRot * q,
				EActivation::Activate);
		}
		
		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		/////////////////////////////////////
		// 	//TOOD extract this pattern
		//Find the distance from hipToSoles
		JPH::Skeleton* skel = ragdollSettings->GetSkeleton();
		auto findJoint = [&](const char* name) -> int {
			for (int i = 0, n = (int)skel->GetJointCount(); i < n; ++i)
				if (std::strcmp(skel->GetJoint(i).mName.c_str(), name) == 0) return i;
			return -1;
		};

		int footL = findJoint("L_Foot_sjnt_0");
		int footR = findJoint("R_Foot_sjnt_0");

		float minSoleY = FLT_MAX;
		for (int fi : { footL, footR }) {
			if (fi < 0) continue;
			JPH::BodyID footID = ragdoll->GetBodyID(fi);
			JPH::AABox wb = bi.GetTransformedShape(footID).GetWorldSpaceBounds();
			minSoleY = std::min(minSoleY, wb.mMin.GetY());
		}

		float hipComY = desiredPos.GetY();                 // hip COM after the move
		float hipsFromSoles = hipComY - minSoleY;          // > 0
		
	

		IgnoreMultipleBodiesFilter* filter = new IgnoreMultipleBodiesFilter;

		entity.set<JoltRagdollFilter>({ filter });
		Utils::Phys::buildRagdollFilter(ragdoll, *filter);

		JPH::BodyID hipID = ragdoll->GetBodyID(0);
		
		////Create an Anchor Body and constrain it to the hip
		//JPH::BodyCreationSettings anchorSettings;
		//anchorSettings.SetShape(new JPH::SphereShape(0.1f));
		//anchorSettings.mMotionType = JPH::EMotionType::Dynamic;
		//anchorSettings.mObjectLayer = Layers::CHARACTER_ANCHOR;
		//anchorSettings.mIsSensor = true;
		//anchorSettings.mPosition = bi.GetPosition(hipID);

		//JPH::BodyID anchorBodyID = bi.CreateAndAddBody(anchorSettings, JPH::EActivation::Activate);

		//JPH::FixedConstraintSettings hipConstraintSettings;
		//hipConstraintSettings.mSpace = EConstraintSpace::LocalToBodyCOM;
		//hipConstraintSettings.mPoint1 = JPH::Vec3::sZero();
		//hipConstraintSettings.mPoint2 = JPH::Vec3::sZero();

		//Ref<FixedConstraint> hipConstraint = dynamic_cast<FixedConstraint*>(bi.CreateConstraint(&hipConstraintSettings, anchorBodyID, hipID));
		//physicsSystem.AddConstraint(hipConstraint);

		entity.set<JoltRagdoll>({ ragdoll});
		
		entity.set<JoltPose2>({ mPose, hipsFromSoles });
		entity.set<JoltAnimation>({ mAnimation });
		//entity.set<JoltAnchorBody>({ anchorBodyID, hipConstraint });

		return true;
	}

	//creates jolts Human.tof 
	static bool createRagdollEntityPID(
		flecs::world& ecs,
		const flecs::entity parent,
		const std::string name,
		const Transform transform,
		entUpdateFn updateFunction) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;
		BodyInterface& bi = physicsSystem.GetBodyInterface();


		constexpr float ragdollScale = 3.0f;

		Ref<RagdollSettings> ragdollSettings =
			RagdollLoader::load("assets/ragdolls/Human.tof", EMotionType::Dynamic, ragdollScale);

		if (!ragdollSettings) {

			LogError(LOG_PHYSICS, "ragdollSettings is null for entity %s", name.c_str());
			return false;
		}

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Ragdoll })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<AnimationTime>({})
			.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::SkeletonPose mPose;

		JPH::Ragdoll* ragdoll = ragdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);
		ragdoll->SetGroupID(static_cast<uint32>(entity.id()));

		cout << "Ragdoll body count : " << ragdoll->GetBodyCount() << std::endl;
		cout << "Ragdoll GetConstraintCount : " << ragdoll->GetConstraintCount() << std::endl;
		cout << "GetSkeleton GetJointCount : " << ragdollSettings->GetSkeleton()->GetJointCount() << std::endl;

		// Load animation (same scale as ragdoll so pose bone offsets match body positions)
		JPH::SkeletalAnimation* mAnimation =
			AnimationLoader::load("assets/ragdolls/neutral.tof", ragdollScale);
		if (!mAnimation) {
			LogError(LOG_PHYSICS, "failed loading animation for entity %s", name.c_str());
			return false;
		}

		// Initialize pose
		mPose.SetSkeleton(ragdollSettings->GetSkeleton());
		mAnimation->Sample(0.0f, mPose); //Setting the pose to this makes it a valid pose
		JPH::Vec3 rootOffset = mPose.GetRootOffset();



		RVec3 desiredPos(transform.position.x, transform.position.y, transform.position.z);
		Quat   desiredRot = Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);

		BodyID rootID = ragdoll->GetBodyID(0);
		RVec3  currentRoot = bi.GetPosition(rootID);
		RVec3  delta = desiredPos - currentRoot;
		for (BodyID id : ragdoll->GetBodyIDs()) {
			RVec3 p = bi.GetPosition(id);
			Quat  q = bi.GetRotation(id);
			bi.SetPositionAndRotation(id, p + delta,
				desiredRot * q,
				EActivation::Activate);
		}

		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		/////////////////////////////////////
		//Find the distance from hipToSoles
		JPH::Skeleton* skel = ragdollSettings->GetSkeleton();

		int footL = Utils::Phys::findJoint(ragdoll, skel, "L_Foot_sjnt_0");
		int footR = Utils::Phys::findJoint(ragdoll, skel, "R_Foot_sjnt_0");


		float minSoleY = FLT_MAX;
		for (int fi : { footL, footR }) {
			if (fi < 0) continue;
			JPH::BodyID footID = ragdoll->GetBodyID(fi);
			JPH::AABox wb = bi.GetTransformedShape(footID).GetWorldSpaceBounds();
			minSoleY = std::min(minSoleY, wb.mMin.GetY());
		}

		float hipComY = desiredPos.GetY();                 // hip COM after the move
		float hipsFromSoles = hipComY - minSoleY;          // > 0

		

		IgnoreMultipleBodiesFilter* filter = new IgnoreMultipleBodiesFilter;

		entity.set<JoltRagdollFilter>({ filter });
		Utils::Phys::buildRagdollFilter(ragdoll, *filter);

		JPH::BodyID hipID = ragdoll->GetBodyID(0);

		entity.set<JoltRagdoll>({ ragdoll });

		entity.set<JoltPose2>({ mPose, hipsFromSoles });
		entity.set<JoltAnimation>({ mAnimation });

		return true;
	}



	static bool createRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		 const Transform transform, const std::string ragdollFilename, entUpdateFn updateFunction) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		//Get the modelSource and Ragdoll filepath from Asset Library
		AssetLibRef assetLibRef = ecs.get<AssetLibRef>();
		std::map<std::string, std::string>& ragdollList = assetLibRef.assetLib->ragdolls;

	/*	ModelSource* modelSource = assetLibRef.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;*/

		if (!validateRagdollExistence(ragdollFilename, ragdollList)) return false;

		const char * ragdollFilePath =  ragdollList.at(ragdollFilename).c_str();

		//Load from file
		std::stringstream dataIn;
		std::ifstream inFile(ragdollFilePath, std::ios::binary);
		if (inFile.is_open()) {
			dataIn << inFile.rdbuf();  // Read entire file into stringstream
			inFile.close();

		}
		else {
			LogError(LOG_SYS, "Failed to open file for reading : %s", ragdollFilePath);
		}


		StreamInWrapper stream_in(dataIn);
		RagdollSettings::RagdollResult result = RagdollSettings::sRestoreFromBinaryState(stream_in);
		if (result.HasError()) {
			LogError(LOG_SYS, "Failed to load binary file: %s", result.GetError().c_str());
			return false;
		}


		JPH::Ragdoll* ragdoll = result.Get()->CreateRagdoll(0, 0, &physicsSystem);
		ragdoll->AddToPhysicsSystem(EActivation::Activate);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<AnimationTime>({})
			//emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		//validate all physics bodies
		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		entity.set<JoltRagdoll>({ ragdoll });
	
		return true;
	}

	static bool createRobotArmEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		//AssetLibRef ref = ecs.get<AssetLibRef>();
		//ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		//if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;


		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		Vec3 pos = RVec3(1.0f, 7.0f, 0.0f);

		Ref<RagdollSettings> mRagdollSettings = RagdollLoader::createArm(pos,1.0f);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			//.set<AnimationTime>({})
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::Ragdoll* ragdoll = mRagdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);


		cout << "Ragdoll body count : " << ragdoll->GetBodyCount() << std::endl;
		cout << "Ragdoll GetConstraintCount : " << ragdoll->GetConstraintCount() << std::endl;
		cout << "GetSkeleton GetJointCount : " << mRagdollSettings->GetSkeleton()->GetJointCount() << std::endl;


		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		TwoBodyConstraint* constraint1 = ragdoll->GetConstraint(1);
		HingeConstraint* hinge = static_cast<HingeConstraint*>(constraint1);
		hinge->SetMotorState(EMotorState::Position);

		MotorSettings& motorSettings = hinge->GetMotorSettings();
		motorSettings.mSpringSettings.mDamping = 1.0f;

		entity.set<JoltRagdoll>({ ragdoll });
		//entity.set<JoltPose>({ mPose });
		//entity.set<JoltAnimation>({ mAnimation });
		
		return true;

	}

	static bool createSnakeEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibRef ref = ecs.get<AssetLibRef>();
		//ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		//if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;


		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		RVec3 pos = RVec3(transform.position.x, transform.position.y, transform.position.z);

		Ref<RagdollSettings> mRagdollSettings = RagdollLoader::createSnake(pos, 1.0f);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			//.set<AnimationTime>({})
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::Ragdoll* ragdoll = mRagdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);



		cout << "Ragdoll body count : " << ragdoll->GetBodyCount() << std::endl;
		cout << "Ragdoll GetConstraintCount : " << ragdoll->GetConstraintCount() << std::endl;
		cout << "GetSkeleton GetJointCount : " << mRagdollSettings->GetSkeleton()->GetJointCount() << std::endl;


		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		entity.set<JoltRagdoll>({ ragdoll });
		//entity.set<JoltPose>({ mPose });
		//entity.set<JoltAnimation>({ mAnimation });

		return true;

	}

	// create a static box shaped sensor
	static bool createBoxSensorEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		Transform transform, JPH::Vec3Arg size,
		ContactFunction onContactAdded) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name)) return false;
		if (!EntityFactory::validateSize(size, name,/*isDynamic=*/false)) return false;


		Vec3 boxHalfExtents(size.GetX() * 0.5, size.GetY() * 0.5, size.GetZ() * 0.5);

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::BodyCreationSettings sensorSetting(
			boxShape,
			joltPosition,
			joltRotation,
			JPH::EMotionType::Static,
			Layers::Sensors
		);

		//Make it a sensor!
		sensorSetting.mIsSensor = true;

		JPH::BodyInterface & bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		BodyID physicsID = bodyInterface.CreateAndAddBody(sensorSetting, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Sensor })
			.add<StaticEnt>()
			.add<Sensor>()
			.set<Transform>(transform)
			.set<JPH::BodyID>(physicsID)
			.add<ContactDataList>()
			.set<HasContactScript, ContactFunction>({ onContactAdded })
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		return true;
	}



	static bool createActorEntity(flecs::world& ecs, flecs::entity parent, std::string_view name,
		Transform transform, JPH::CharacterSettings settings,
		entUpdateFn actorUpdate) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		//Get MeshComponent from AssetManager
		AssetManager* assetManager = ecs.get<AssetManagerRef>().assetManager;

		ModelData model = assetManager->requestModel(assetManager->defaultAssetsMap.at(DefaultAssets::ROBOT));
		MeshNode rootNode = assetManager->requestMeshNode(model.rootNodeID);

		MeshComponent meshComp = assetManager->requestMeshComponent(rootNode.meshID);
		glm::vec3 scaledSize = meshComp.aabb.extents * transform.scale;

		float physicsRadius = scaledSize.x;
		float physicsHalfHeight = scaledSize.y;

		//the half-height parameter in Jolt is the half-height of the cylindrical center section only
		float physicsCylHalf = scaledSize.y - physicsRadius;

		if (physicsCylHalf <= 0) {
			LogError(LOG_APP, "physicsCylHalf is zero");
			return false;
		}


		// Character settings
		JPH::CharacterSettings settings2;
		settings2.mShape = new CapsuleShape(physicsCylHalf, physicsRadius);
		settings2.mMass = 2000.0f;
		settings2.mMaxSlopeAngle = DegreesToRadians(20.0f); // Max walkable slope
		settings2.mLayer = Layers::MOVING;
		settings2.mGravityFactor = 1;

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}



		JPH::Character* joltCharacter = new JPH::Character(
			&settings2, joltPosition, joltRotation, 0, &physicsSystem
		);

		if (!validatePhysicsBodyCreation(joltCharacter->GetBodyID(), name.data())) {
			delete joltCharacter;
			return false;
		}

		flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Actor})
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<ActorDebugInfo>({})
			.set<JoltCharacter>({ joltCharacter })
			.set<JPH::BodyID>(joltCharacter->GetBodyID())
			.emplace<ActorBehavior>(actorUpdate)
			.child_of(parent);

		if (!validateEntityCreation(entity, name.data())) {
			delete joltCharacter;
			return false;
		}

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);
		physicsSystem.GetBodyInterface().SetUserData(joltCharacter->GetBodyID(), entity.id());

		if (!createMeshHierarchy(ecs, entity, rootNode, name, assetManager)) return false;

		return true;
	}

	// A Renderable is just a model and a transform no physics body
	static bool createRenderableEntity(flecs::world& ecs, flecs::entity parent,
		std::string name, Transform transform, const uint64_t& meshID) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		//Get MeshComponent from AssetManager
		AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;

		MeshComponent meshComp = assetManger->requestMeshComponent(meshID);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			;

		if (!validateEntityCreation(entity, name)) return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManger, entity, meshComp, name)) return false;

		return true;

	}

	/// <summary>
	/// Infinitely far away, parallel rays ? sun, moon .Has no position, only direction.
	/// </summary>
	static bool createDirectionalLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const DirectionalLight& directionalLight) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Light })
			.add<Light>()
			.set<DirectionalLight>({ directionalLight })
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	/// <summary>
	/// A point light radiates in all directions from a point, fades with distance
	/// </summary>
	static bool createPointLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const PointLight& pointLight) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Light })
			.add<Light>()
			.set<PointLight>({ pointLight })
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}


	static bool createAreaLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const AreaLight& areaLight, Transform& transform) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.add<Light>()
			.set<AreaLight>({ areaLight });

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	static bool createMTNEntity(flecs::world& ecs, const flecs::entity parent,
		const std::string name, Transform transform) {

		AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;

		uint64_t pipelineID = getPipelineEntityId(ecs, "pipelineSolid-Wireframe");

		if (pipelineID == 0) {

			LogWarn(LOG_APP, "Mountain will be created with default pipeline because pipelineID is zero");
		}

		if (!createStaticMeshEntity(ecs, parent, name, transform, assetManger->defaultAssetsMap.at(DefaultAssets::MOUNTAIN), pipelineID)) {
			
			return false;
		}

		return true;
	}

	static bool createStaticMeshEntity(flecs::world& ecs, const flecs::entity parent, 
		std::string_view name, Transform transform, uint64_t meshID, uint64_t pipelineID = 0) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;
	
		//Get MeshComponent from AssetManager
		AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;

		MeshComponent meshComp = assetManger->requestMeshComponent(meshID);
		Mesh meshSrc = assetManger->requestMesh(meshID);

		if (meshSrc.vertices.size() == 0) {

			LogError(LOG_ERR, "Mesh is empty");
			return false;
		}

		//Create physics body from mesh data
		// Scale vertices
		VertexList scaledVertexList;
		for (const Vertex& vertexData : meshSrc.vertices) {
			glm::vec3 scaledVertex = vertexData.position * transform.scale; // Apply scale
			scaledVertexList.push_back(Float3(scaledVertex.x, scaledVertex.y, scaledVertex.z));
		}

		// Create triangle list
		IndexedTriangleList triangleList;
		for (size_t i = 0; i < meshSrc.indices.size(); i += 3) {
			triangleList.push_back(IndexedTriangle(
				meshSrc.indices[i],
				meshSrc.indices[i + 1],
				meshSrc.indices[i + 2]
			));
		}

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create MeshShapeSettings
		MeshShapeSettings meshSettings(scaledVertexList, triangleList);

		// Create MeshShape
		Ref<Shape> meshShape = meshSettings.Create().Get();

		// Create BodyCreationSettings
		BodyCreationSettings meshBodySettings(
			meshShape,
			joltPosition,
			joltRotation,
			EMotionType::Static,
			Layers::NON_MOVING
		);

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();


		// Create and add body
		BodyID physicsID = bodyInterface.CreateAndAddBody(
			meshBodySettings,
			EActivation::DontActivate
		);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;

		const flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::StaticMesh })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			//for secondary pipelines create a tag which will be used in the query for that renderQuery
			//.add<RenderPipeline>( ecs.lookup(pipelineName.c_str())) 
			.child_of(parent);


		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data())) return false;

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManger, entity, meshComp, name, transform, pipelineID)) return false;

		return true;
	}

	static bool createGridEntity(flecs::world& ecs, const flecs::entity parent, std::string_view name, Transform transform, uint32_t size) {

		if (!validateName(ecs, parent, name.data())) return false;
		if (!validateTransform(transform, name.data())) return false;

		//Get the modelSource from Asset Library
		AssetManager* assetManger = ecs.get<AssetManagerRef>().assetManager;

		std::string assetName = std::format("Grid{}", size);

		MeshComponent meshComp;
		uint64_t id =  util::generateAssetID(assetName);
		//If a grid chunk of this size has been generated before then use it,
		//If not then generate and send it to assetManager
		if (assetManger->isMeshCompLoaded(id)) {

			meshComp = assetManger->requestMeshComponent(id);
		}
		else {

			Mesh gridMesh = createGridMesh(size);
			assetManger->fillMeshComp(gridMesh, meshComp, id);
		}

		// any thickness less than 0.01 will break jolt!
		float boxThickness = 1;
		Vec3 boxHalfExtents(size * 0.5, boxThickness * 0.5, size * 0.5);

		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// - 0.5 is needed to visually align the grid render with the physics body #MAGICNUMBER
		//TODO find out why the grid render slightly below its physics body by default
		Vec3 joltPosition(transform.position.x, transform.position.y - boxThickness * 0.5, transform.position.z);
		Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		// Create BodyCreationSettings
		BodyCreationSettings boxBodySettings(
			boxShape,
			joltPosition,
			joltRotation,
			EMotionType::Static,
			Layers::NON_MOVING
		);

		boxBodySettings.mRestitution = 0.1f; // High restitution for bounciness
		boxBodySettings.mFriction = 1.0f;    // Low friction for sliding

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		BodyID physicsID = bodyInterface.CreateAndAddBody(boxBodySettings, EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name.data())) return false;

		const flecs::entity entity = ecs.entity(name.data())
			.set<EntityTypeComponent>({ EntityType::Grid })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<WorldMatrix>({})
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent);

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name.data())) return false;

		//For now look up the desired pipeline and add it to the SubMesh Entity

		uint64_t pipelineID = getPipelineEntityId(ecs,"pipelineGrid-Wireframe");

		if (pipelineID == 0) {

			LogWarn(LOG_APP, "Grid will be created with default pipeline because pipelineID is zero");
		}

		//Create a child entity for mesh and a grandchild entity for each submesh.
		if (!createMeshEntity(ecs, assetManger, entity, meshComp, name, transform, pipelineID)) return false;

		return true;
	}

	//TODO use transform
	static bool createPlayerEntity(flecs::world& ecs, const flecs::entity parent, Transform transform, const std::string pipelineName, bool sCreateInnerBody = true) {

		const RenderConfig& config = ecs.get<RenderConfig>();

		string playerName = "player";
		string playerCamName = "PlayerCam";

		if (!validateName(ecs, parent, playerName)) return false;
		if (!validateName(ecs, parent, playerCamName)) return false;

		flecs::entity playerEntity = ecs.entity(playerName.c_str())
			.set<EntityTypeComponent>({ EntityType::Player })
			.child_of(parent);
		playerEntity.emplace<Player>(ecs, JPH::Vec3(1.0f, 15.0f, 0.0f), JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f), 3.0f, 1.0f, playerEntity.id(), sCreateInnerBody);

		ecs.set<PlayerRef>({ playerEntity });

		if (!validateEntityCreation(playerEntity, playerName)) return false;


		flecs::entity playerCam = ecs.entity(playerCamName.c_str())
			.set<EntityTypeComponent>({ EntityType::Camera })
			.emplace<Camera>(config)
			.child_of(parent);

		ecs.set<PlayerCamRef>({ playerCam });

		if (!validateEntityCreation(playerCam, playerCamName)) return false;

		
		return true;
	}

	static bool createMeshHierarchy(flecs::world& ecs, const flecs::entity parent, const MeshNode& rootNode, std::string_view name, AssetManager* assetManager) {

		// Request the mesh component for the root itself
		MeshComponent meshComp = assetManager->requestMeshComponent(rootNode.meshID);

		std::string rootEntName = std::format("{}-Mesh", name.data());
		flecs::entity rootEntity = ecs.entity(flecs::Parent{ parent }, rootEntName.c_str())
			.set<WorldMatrix>({})
			.set<Transform>(rootNode.transform)
			.set<MeshComponent>({ meshComp });

		if (!validateEntityCreation(rootEntity, rootEntName.c_str())) return false;

		if (!createSubMeshEntities(ecs, rootEntity, meshComp, rootEntName, assetManager)) {
			return false;
		}


		// recursively create all children
		return createMeshEntityHelperRecursive(ecs, rootEntity, rootNode, name, assetManager);
	}

	static bool createMeshEntityHelperRecursive(flecs::world& ecs, const flecs::entity parent, const MeshNode& meshNode, std::string_view name, AssetManager* assetManager) {

		for (size_t i = 0; i < meshNode.children.size(); i++) {
			MeshNode childNode = assetManager->requestMeshNode( meshNode.children[i]);

			// Process this specific child
			MeshComponent meshComp = assetManager->requestMeshComponent(childNode.meshID);

			std::string childEntName = std::format("{}-Mesh-{}", name.data(), childNode.name);
			flecs::entity meshEntity = ecs.entity(flecs::Parent{ parent }, childEntName.c_str())
				.set<WorldMatrix>({})
				.set<Transform>(childNode.transform)
				.set<MeshComponent>({ meshComp });

			if (!validateEntityCreation(meshEntity, childEntName.c_str())) return false;

			// Create subMeshes for this child
			if (!createSubMeshEntities(ecs, meshEntity, meshComp, childEntName, assetManager)) {
				return false;
			}

			// Recurse deeper into this child's own children
			if (!createMeshEntityHelperRecursive(ecs, meshEntity, childNode, name, assetManager)) {
				return false;
			}
		}

		return true;
	}


	static bool createMeshEntity(flecs::world& ecs, AssetManager* assetManager, const flecs::entity parent, const MeshComponent& meshComp, std::string_view name, Transform transform = {}, uint64_t pipelineID = 0) {

		std::string rootEntName = std::format("{}-Mesh", name.data());
		flecs::entity rootEntity = ecs.entity(flecs::Parent{ parent }, rootEntName.c_str())
			.set<WorldMatrix>({})
			.set<Transform>(transform)
			.set<Transform>(transform)
			.set<MeshComponent>({ meshComp });

		if (!validateEntityCreation(rootEntity, rootEntName.c_str())) return false;

		if (!createSubMeshEntities(ecs, rootEntity, meshComp, rootEntName, assetManager, pipelineID)) {
			return false;
		}

		return true;
	}

	//static bool createMeshEntity(flecs::world& ecs, const flecs::entity parent, uint64_t meshID, std::string_view name, AssetManager* assetManager) {

	//	MeshComponent meshComp = assetManager->requestMeshComponent(meshID);

	//	std::string rootEntName = std::format("{}-Mesh", parent.name());
	//	flecs::entity rootEntity = ecs.entity(flecs::Parent{ parent }, rootEntName.c_str())
	//		.set<WorldMatrix>({})
	//		.set<MeshComponent>({ meshComp });

	//	if (!validateEntityCreation(rootEntity, rootEntName.c_str())) return false;

	//	if (!createSubMeshEntities(ecs, rootEntity, meshComp, rootEntName, assetManager)) {
	//		return false;
	//	}

	//	return true;
	//}

	static bool createSubMeshEntities(flecs::world& ecs, const flecs::entity parent,  const MeshComponent & meshComp, std::string_view name, AssetManager* assetManager, uint64_t pipelineID = 0) {


		for (int i = 0; i < meshComp.subMeshCount; i++) {

			SubMeshComponent subMeshComp = assetManager->requestSubMeshComponent(meshComp.firstSubMeshIndex + i);

			subMeshComp.pipelineID = pipelineID;

			std::string childEntName = std::format("{}-subMesh {}", name, i);
			flecs::entity childEntity = ecs.entity(flecs::Parent{ parent }, childEntName.c_str())
				.set<SubMeshComponent>({ std::move(subMeshComp)});

			if (!validateEntityCreation(childEntity, childEntName.c_str()))  return false;
		}

		return true;

	}

	static bool createHUDElementEntity(flecs::world& ecs, std::string name, std::function<void(flecs::world& ecs)> drawFunction) {

		flecs::entity entity = ecs.entity(name.c_str())
			.emplace<HudRender>(drawFunction);

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	static flecs::entity createEditorItemEntity(flecs::world& ecs, std::string name, flecs::entity editorToggle,
		std::function<void(flecs::world& ecs)> drawFunction) {

		flecs::entity entity = ecs.entity(name.c_str())
			.emplace<Render>(drawFunction)
			.add<EditorUIComponent>();

		if (!validateEntityCreation(entity, name)) return flecs::entity::null();

		//Adding entity to editorToggle enables us to disable all Editor Components by disabling it
		editorToggle.add(entity);

		return entity;
	}

	static flecs::entity createMenuItemEntity(flecs::world& ecs, std::string name,
		std::function<void(flecs::world& ecs)> drawFunction) {

		flecs::entity entity = ecs.entity(name.c_str())
			.emplace<Render>(drawFunction)
			.add<MenuComponent>();

		if (!validateEntityCreation(entity, name)) return flecs::entity::null();

		return entity;
	}

	static uint64_t getPipelineEntityId(flecs::world& ecs, std::string_view name) {

		flecs::entity pipelineEnt = ecs.lookup(name.data());
		if (!pipelineEnt.is_valid()) {
			LogError(LOG_APP, "Pipeline Entity with name : %s does not exist!", name.data());
			return 0;
		}

		const Pipeline* pipeline = pipelineEnt.try_get<Pipeline>();

		if (!pipelineEnt.try_get<Pipeline>()) {
			LogError(LOG_APP, "Pipeline Entity with name : % s does not  have pipeline component", name.data());
			return 0;

		}

		return pipelineEnt.id();
	}

	static bool validateName(flecs::world& ecs, flecs::entity parent, std::string_view name) {
		if (name.empty()) {
			LogError(LOG_APP, "Entity name cannot be empty, unable to create child entity for %s", parent.name());
			return false;
		}
		if (name.length() > 256) {
			LogError(LOG_APP, "Error Entity name '%s' exceeds 256 characters", name.data());
			return false;
		}

		// Check if an entity with the same name already exists in this scope
		flecs::entity existing;

		if (parent.is_valid()) {
			// Look up the entity within the parent's scope
			existing = parent.lookup(name.data());
		}
		
		if (existing.is_valid()) {
			LogError(LOG_ECS, "EntityFactory Error: Entity with name '%s' already exists under parent '%s'",
				name.data(),
				parent.name().c_str());
			return false;
		}
		return true;
	}

	static bool validateTransform(Transform transform, std::string_view name) {
		if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) || !std::isfinite(transform.position.z)) {
			LogError(LOG_APP, "Error Invalid position (contains NaN or Inf) found in transform for : %s", name.data());
			return false;
		}
		if (!std::isfinite(transform.rotation.x) || !std::isfinite(transform.rotation.y) ||
			!std::isfinite(transform.rotation.z) || !std::isfinite(transform.rotation.w)) {
			LogError(LOG_APP, "Error Invalid rotation (contains NaN or Inf) found in transform for : %s", name.data());
			return false;
		}
		if (!std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
			!std::isfinite(transform.scale.z)) {
			LogError(LOG_APP, "Error Invalid scale (contains NaN or Inf) found in transform for : %s", name.data());
			return false;
		}
		return true;
	}


	// Jolt documentation says dynamic objects should be in the order [0.1, 10]
	// Static objects should be in the order [0.1, 2000] meters long
	static bool validateSize(JPH::Vec3Arg size, std::string_view name, bool dynamicObject)
	{
		const JPH::Vec3 minSizeConstraint(0.1f, 0.1f, 0.1f);
		const JPH::Vec3 maxSizeConstraint = dynamicObject
			? JPH::Vec3(10.0f, 10.0f, 10.0f)
			: JPH::Vec3(2000.0f, 2000.0f, 2000.0f);

		// Check for NaN / Inf first
		if (!std::isfinite(size.GetX()) || !std::isfinite(size.GetY()) || !std::isfinite(size.GetZ())) {
			LogError(LOG_APP, "Error: Entity %s size contains NaN or Inf", name.data());
			return false;
		}

		// Minimum constraint check
		if (size.GetX() < minSizeConstraint.GetX() || size.GetY() < minSizeConstraint.GetY() || size.GetZ() < minSizeConstraint.GetZ()) {
			LogError(LOG_APP,
				"Error: Size components for entity %s must be >= 0.1 m (x: %.3f, y: %.3f, z: %.3f)",
				name.data(), size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		// Maximum constraint check
		if (size.GetX() > maxSizeConstraint.GetX() || size.GetY() > maxSizeConstraint.GetY() || size.GetZ() > maxSizeConstraint.GetZ()) {
			LogError(LOG_APP,
				"Warning: Entity %s size exceeds recommended bounds for %s objects (x: %.3f, y: %.3f, z: %.3f)",
				name.data(), dynamicObject ? "dynamic" : "static", size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		return true;
	}

	static bool validatePhysicsBodyCreation(JPH::BodyID id, std::string_view name) {

		if (id.IsInvalid()) {
			LogError(LOG_PHYSICS, "Error creating physics body for Entity : %s", name.data());
			return false;
		}
		return true;
	}

	static bool validatePipelineExistence(flecs::world& ecs,std::string pipelineName) {

		flecs::entity pipelineEnt = ecs.lookup(pipelineName.c_str());

		if (!pipelineEnt.is_valid()) {
			LogError(LOG_ECS, "Pipeline entity does not exist : %s", pipelineName.c_str());
			return false;
		}
	}
	static bool validateEntityCreation(flecs::entity entity, std::string_view name) {
		if (!entity.is_valid()) {
			LogError(LOG_ECS, "Error creating Entity : %s", name.data());
			return false;
		}
		return true;
	}

	static bool validateRagdollExistence(const std::string key, const std::map<std::string, std::string>& ragdolls) {

		if (!ragdolls.contains(key)) {
			LogError(LOG_ECS, "Ragdoll list does not have the following key : %s", key.c_str());
			LogError(LOG_ECS, "Ragdoll list does not have the following key : %s", key.c_str());
			return false;
		}

		return true;
	}
};





