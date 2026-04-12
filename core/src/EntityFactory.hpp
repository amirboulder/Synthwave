#pragma once

#include "../../core/src/AssetLibrary/AssetLibrary.hpp"

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

	static bool createCubeEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibrary * assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName)) return false;

		float meshX, meshY, meshZ;

		// Assuming modelSource has only 1 mesh
		MeshSource::calculateMeshSize(modelSource->meshes[0], meshX, meshY, meshZ);

		Vec3 boxHalfExtents(meshX * 0.5, meshY * 0.5, meshZ * 0.5);

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);


		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::BodyCreationSettings cubeSetting(
			boxShape,
			joltPosition,
			joltRotation,
			JPH::EMotionType::Dynamic,
			Layers::MOVING
		);

		// bounciness
		cubeSetting.mRestitution = 0.5f;

		cubeSetting.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		cubeSetting.mMassPropertiesOverride.mMass = 50.1f;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		// Create and add body
		const BodyID physicsID = bodyInterface.CreateAndAddBody(cubeSetting, JPH::EActivation::Activate);

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Cube })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(ModelSrcName.c_str()) })
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.child_of(parent)
			;



		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name))  return false;

		return true;

	}

	static bool createCarEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName)) return false;

		JPH::BodyInterface& bodyInterface = ecs.get<PhysicsSystemRef>().physicsSystem.GetBodyInterface();

		MeshComponent meshComp = assetLib->requestMeshComponent(ModelSrcName.c_str());

		flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Car })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ std::move(meshComp) })
			.add<Renderable>()
			//.set<PhysicsBody>(std::move(physicsBodyIDs))
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.child_of(parent)
			;

		if (!validateEntityCreation(entity, name))  return false;


		StaticCompoundShapeSettings settings;

		for (const MeshSource& mesh : modelSource->meshes) {

			float meshX, meshY, meshZ;

			// Assuming modelSource has only 1 mesh
			MeshSource::calculateMeshSize(mesh, meshX, meshY, meshZ);

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


		entity.set<JPH::BodyID>(physicsID);


		return true;

	}

	static bool createHumanRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

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
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
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
	static bool createHumanTOFRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		/*AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;*/

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		Ref<RagdollSettings> ragdollSettings = RagdollLoader::load("assets/Human.tof", EMotionType::Dynamic);

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
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			//.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::SkeletalAnimation* mAnimation;
		JPH::SkeletonPose* mPose = new JPH::SkeletonPose;

		JPH::Ragdoll* ragdoll = ragdollSettings->CreateRagdoll(0, entity.id(), &physicsSystem);
		ragdoll->AddToPhysicsSystem(JPH::EActivation::Activate);


		cout << "Ragdoll body count : " << ragdoll->GetBodyCount() << std::endl;
		cout << "Ragdoll GetConstraintCount : " << ragdoll->GetConstraintCount() << std::endl;
		cout << "GetSkeleton GetJointCount : " << ragdollSettings->GetSkeleton()->GetJointCount() << std::endl;

		// Load animation

		AssetStream stream(String("assets/sprint.tof"), std::ios::in);
		if (!ObjectStreamIn::sReadObject(stream.Get(), mAnimation))
			cout << "ERROR Loading animation \n";

		cout << "mAnimation GetAnimatedJoints : " << mAnimation->GetAnimatedJoints().size() << std::endl;

		// Initialize pose
		mPose->SetSkeleton(ragdollSettings->GetSkeleton());

		// Place the root joint on the first body so that we draw the pose in the right place
		//RVec3 root_offset = RVec3(10.0f, 10.0f, 10.0f);

		//SkeletonPose::JointState& joint = mPose->GetJoint(0);
		//joint.mTranslation = Vec3::sZero(); // All the translation goes into the root offset
		//ragdoll->GetRootTransform(root_offset, joint.mRotation);

		for (JPH::BodyID id : ragdoll->GetBodyIDs()) {

			if (!validatePhysicsBodyCreation(id, name)) return false;

		}

		entity.set<JoltRagdoll>({ ragdoll });
		entity.set<JoltPose>({ mPose });
		entity.set<JoltAnimation>({ mAnimation });

		return true;

	}


	static bool createRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, const std::string ragdollFilename, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

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
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
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
		std::function<void(flecs::world& ecs, flecs::entity self, flecs::entity other)> onContactAdded) {

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
			Layers::NON_MOVING
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
			.emplace<SensorBehavior>(onContactAdded)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		return true;
	}


	//Creates a capsule shaped entity
	static bool createCapsuleEntity(flecs::world& ecs,const flecs::entity parent ,const std::string name,const std::string ModelSrcName, const Transform transform, const std::string pipelineName) {

		if (!validateName(ecs,parent,name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if(!validatePipelineExistence(ecs,pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName)) return false;

		float meshX;
		float meshY;
		float meshZ;

		// Assuming modelSource has only 1 mesh
		MeshSource::calculateMeshSize(modelSource->meshes[0], meshX, meshY, meshZ);

		// Compute capsule dimensions
		float modelRadius = meshX / 2.0f; // Unscaled model radius
		float modelHeight = meshY; // Unscaled model total height
		float physicsRadius = modelRadius * transform.scale.x; // Scale radius (x-axis)
		float physicsHalfHeight = (modelHeight / 2.0f - modelRadius) * transform.scale.y; // Scale height (y-axis)

		// Ref<> manages reference counting - no manual cleanup needed
		Ref<Shape> capsuleShape = new JPH::CapsuleShape(physicsHalfHeight, physicsRadius);


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

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Capsule })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(ModelSrcName.c_str()) })
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.child_of(parent)
			;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name))  return false;

		return true;
	}

	
	static bool createActorEntity(flecs::world& ecs, flecs::entity parent, const std::string name, const std::string ModelSrcName, Transform transform, JPH::CharacterSettings settings,
		std::function<void(flecs::world& ecs, flecs::entity self)> actorUpdate, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName));

		// Convert GLM to Jolt types
		JPH::Vec3 joltPosition(transform.position.x, transform.position.y, transform.position.z);
		JPH::Quat joltRotation(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
		if (!joltRotation.IsNormalized()) {
			joltRotation = joltRotation.Normalized();
		}

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;


		JPH::Character* joltCharacter = new JPH::Character(
			&settings, joltPosition, joltRotation, 0, &physicsSystem
		);

		if (!validatePhysicsBodyCreation(joltCharacter->GetBodyID(), name)) {
			delete joltCharacter;
			return false;
		}

		flecs::entity actorEnt = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Actor})
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(ModelSrcName.c_str()) })
			.add<Renderable>()
			.set<JoltCharacter>({ joltCharacter })
			.set<JPH::BodyID>(joltCharacter->GetBodyID())
			.emplace<ActorBehavior>(actorUpdate)
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.child_of(parent);

		if (!validateEntityCreation(actorEnt, name)) {
			delete joltCharacter;
			return false;
		}

		joltCharacter->AddToPhysicsSystem(JPH::EActivation::Activate);
		physicsSystem.GetBodyInterface().SetUserData(joltCharacter->GetBodyID(), actorEnt.id());

		return true;
	}

	// A Renderable is just a model and a transform no physics body
	//TODO Update
	static bool createRenderableEntity(flecs::world& ecs, flecs::entity parent, std::string name, const std::string ModelSrcName, Transform transform, const char* pipelineName = NULL) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName));

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(ModelSrcName.c_str()) })
			.add<Renderable>()
			;

		if (pipelineName) {
			entity.add<RenderPipeline>(ecs.lookup(pipelineName));

		}

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	/// <summary>
	/// Infinitely far away, parallel rays — sun, moon .Has no position, only direction.
	/// </summary>
	static bool createDirectionalLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const Light& light, const DirectionalLight& directionalLight, Transform& transform) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<Light>({ light })
			.add<DirectionalLight>()
			;

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	/// <summary>
	/// A point light radiates in all directions from a point, fades with distance
	/// </summary>
	static bool createPointLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const Light& light, const PointLight& pointLight, Transform& transform) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<Light>({ light })
			.set<PointLight>({ pointLight })
			;

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}


	/// <summary>
	/// Global constant light — no position, no direction, hits everything equally
	/// </summary>
	static bool createAmbientLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const Light& light, const AmbientLight& ambientLight) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Light>({ light })
			.set<AmbientLight>({ ambientLight })
			;

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}


	static bool createAreaLightEntity(flecs::world& ecs, flecs::entity parent, std::string name, const Light& light, const PointLight& pointLight, Transform& transform) {

		if (!EntityFactory::validateName(ecs, parent, name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;

		//Get the modelSource from Asset Library
		//AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		//const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		//if (!validateModelSource(modelSource, ModelSrcName));

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)

			.set<Light>({ light })
			.set<PointLight>({ pointLight })
			;

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	static bool createStaticMeshEntity(flecs::world& ecs, const flecs::entity parent, const std::string name, const std::string ModelSrcName, Transform transform, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		float scaleFactor = 1.0f;

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName));

		// Scale vertices
		//ASSUMING the model only has one mesh
		VertexList scaledVertexList;
		for (const Vertex& vertexData : modelSource->meshes[0].vertices) {
			glm::vec3 scaledVertex = vertexData.position * scaleFactor; // Apply scale
			scaledVertexList.push_back(Float3(scaledVertex.x, scaledVertex.y, scaledVertex.z));
		}


		// Create triangle list
		//ASSUMING the model only has one mesh
		IndexedTriangleList triangleList;
		for (size_t i = 0; i < modelSource->meshes[0].indices.size(); i += 3) {
			triangleList.push_back(IndexedTriangle(
				modelSource->meshes[0].indices[i],
				modelSource->meshes[0].indices[i + 1],
				modelSource->meshes[0].indices[i + 2]
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

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::StaticMesh })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ ModelSrcName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(ModelSrcName.c_str()) })
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.add<RenderPipeline>( ecs.lookup(pipelineName.c_str()))
			.child_of(parent);


		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	
	static bool createGridEntity(flecs::world& ecs, const flecs::entity parent, const std::string name, Transform transform, const std::string pipelineName, uint32_t size) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;


		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;

		//Grid is generated so generate then get the model
		std::string modelName = assetLib->generateGridModel(size);

		//const ModelSource* modelSource = assetLib->getModel(modelName);
		//if (!validateModelSource(modelSource, modelName));

		// any thickness less than 0.01 will break jolt!
		float boxThickness = 1;
		Vec3 boxHalfExtents(size * 0.5, boxThickness * 0.5, size * 0.5);

		Ref<Shape> boxShape = new BoxShape(boxHalfExtents);

		// - 0.5 is needed to visually align the grid render with the physics body #MAGICNUMBER
		//TODO find out why the grid render slightly below its physics body by default
		Vec3 joltPosition(transform.position.x, transform.position.y - boxThickness - 0.5, transform.position.z);
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

		

		if (!validatePhysicsBodyCreation(physicsID, name)) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityTypeComponent>({ EntityType::Grid })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelSourceName>({ modelName.c_str() })
			.set<MeshComponent>({ assetLib->requestMeshComponent(modelName.c_str()) })
			.add<Renderable>()
			.set<JPH::BodyID>(physicsID)
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.child_of(parent);

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	//TODO use transform
	static bool createPlayerEntity(flecs::world& ecs, const flecs::entity parent, Transform transform, const std::string pipelineName,  const std::string ModelSrcName = " ") {

		const RenderConfig& config = ecs.get<RenderConfig>();

		string playerName = "player";
		string playerCamName = "PlayerCam";

		if (!validateName(ecs, parent, playerName)) return false;
		if (!validateName(ecs, parent, playerCamName)) return false;

		flecs::entity playerEntity = ecs.entity(playerName.c_str())
			.set<EntityTypeComponent>({ EntityType::Player })
			.child_of(parent);
		playerEntity.emplace<Player>(ecs, JPH::Vec3(1.0f, 15.0f, 0.0f), JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, 1.0f, playerEntity.id());

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

	static const ModelSource* getModelSource(flecs::world& ecs, std::string ModelSrcName) {

		//Get the modelSource from Asset Library
		AssetLibrary* assetLib = ecs.get<AssetLibRef>().assetLib;
		const ModelSource* modelSource = assetLib->getModel(ModelSrcName);
		if (!validateModelSource(modelSource, ModelSrcName)) return nullptr;

		return modelSource;
	}


	static bool validateName(flecs::world& ecs, flecs::entity parent, std::string name) {
		if (name.empty()) {
			LogError(LOG_APP, "Entity name cannot be empty, unable to create child entity for %s", parent.name());
			return false;
		}
		if (name.length() > 256) {
			LogError(LOG_APP, "Error Entity name '%s' exceeds 256 characters", name.c_str());
			return false;
		}

		// Check if an entity with the same name already exists in this scope
		flecs::entity existing;

		if (parent.is_valid()) {
			// Look up the entity within the parent's scope
			existing = parent.lookup(name.c_str());
		}
		
		if (existing.is_valid()) {
			LogError(LOG_ECS, "EntityFactory Error: Entity with name '%s' already exists under parent '%s'",
				name.c_str(),
				parent.name().c_str());
			return false;
		}
		return true;
	}

	static bool validateTransform(Transform transform, std::string name) {
		if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) || !std::isfinite(transform.position.z)) {
			LogError(LOG_APP, "Error Invalid position (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.rotation.x) || !std::isfinite(transform.rotation.y) ||
			!std::isfinite(transform.rotation.z) || !std::isfinite(transform.rotation.w)) {
			LogError(LOG_APP, "Error Invalid rotation (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
			!std::isfinite(transform.scale.z)) {
			LogError(LOG_APP, "Error Invalid scale (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		return true;
	}


	// Jolt documentation says dynamic objects should be in the order [0.1, 10]
	// Static objects should be in the order [0.1, 2000] meters long
	static bool validateSize(JPH::Vec3Arg size, const std::string& name, bool dynamicObject)
	{
		const JPH::Vec3 minSizeConstraint(0.1f, 0.1f, 0.1f);
		const JPH::Vec3 maxSizeConstraint = dynamicObject
			? JPH::Vec3(10.0f, 10.0f, 10.0f)
			: JPH::Vec3(2000.0f, 2000.0f, 2000.0f);

		// Check for NaN / Inf first
		if (!std::isfinite(size.GetX()) || !std::isfinite(size.GetY()) || !std::isfinite(size.GetZ())) {
			LogError(LOG_APP, "Error: Entity %s size contains NaN or Inf", name.c_str());
			return false;
		}

		// Minimum constraint check
		if (size.GetX() < minSizeConstraint.GetX() || size.GetY() < minSizeConstraint.GetY() || size.GetZ() < minSizeConstraint.GetZ()) {
			LogError(LOG_APP,
				"Error: Size components for entity %s must be >= 0.1 m (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		// Maximum constraint check
		if (size.GetX() > maxSizeConstraint.GetX() || size.GetY() > maxSizeConstraint.GetY() || size.GetZ() > maxSizeConstraint.GetZ()) {
			LogError(LOG_APP,
				"Warning: Entity %s size exceeds recommended bounds for %s objects (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), dynamicObject ? "dynamic" : "static", size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		return true;
	}

	static bool validatePhysicsBodyCreation(JPH::BodyID id, std::string name) {

		if (id.IsInvalid()) {
			LogError(LOG_PHYSICS, "Error creating physics body for Entity : %s", name.c_str());
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
	static bool validateEntityCreation(flecs::entity entity, std::string name) {
		if (!entity.is_valid()) {
			LogError(LOG_ECS, "Error creating Entity : %s", name.c_str());
			return false;
		}
		return true;
	}

	static bool validateModelSrcExistence(ModelSource* model,const std::string modelName) {

		if (!model) {
			LogError(LOG_ECS, "EntityFactory Error ModelSource does not exist! : %s", modelName.c_str());
			return false;
		}
		return true;

	}

	static bool validateModelSource(const ModelSource * src, const std::string modelName) {

		if (!src) {
			LogError(LOG_ECS, "EntityFactory ModelSource %s does not exist", modelName.c_str());
			return false;
		}
		return true;

	}

	static bool validateRagdollExistence(const std::string key, const std::map<std::string, std::string>& ragdolls) {

		if (!ragdolls.contains(key)) {
			LogError(LOG_ECS, ERROR "Ragdoll list does not have the following key : %s" RESET, key.c_str());
			return false;
		}

		return true;

	}

	//TODO move this function
	


};





