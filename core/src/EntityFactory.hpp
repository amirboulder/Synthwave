#pragma once
#include "core/src/pch.h"

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
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

		float meshX;
		float meshY;
		float meshZ;

		// Assuming modelSource has only 1 mesh
		calculateMeshSize(modelSource->meshes[0], meshX, meshY, meshZ);

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
			.set<EntityType>({ EntityType::Capsule })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
			.set<JPH::BodyID>(physicsID)
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			.child_of(parent)
			;

		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name))  return false;

		return true;

	}

	//creates jolts Human.tof 
	static bool createHumanRagdollEntity(flecs::world& ecs, const flecs::entity parent, const std::string name,
		const std::string ModelSrcName, const Transform transform, entUpdateFn updateFunction, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		Ref<RagdollSettings> ragdollSettings = RagdollLoader::load("assets/Human.tof", EMotionType::Dynamic);

		if (!ragdollSettings) {
			
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, ERROR "ragdollSettings is null" RESET);
			return false;
		}


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityType>({ EntityType::Ragdoll })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
			.set<AnimationTime>({})
			.add<RenderPipeline>(ecs.lookup(pipelineName.c_str()))
			//.emplace<ActorBehavior>(updateFunction)
			.child_of(parent);

		if (!validateEntityCreation(entity, name)) return false;

		JPH::SkeletalAnimation *  mAnimation;
		JPH::SkeletonPose *		  mPose =  new JPH::SkeletonPose;

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
		ModelSource* modelSource = assetLibRef.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;
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
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, ERROR "Failed to open file for reading : %s" RESET, ragdollFilePath);
		}


		StreamInWrapper stream_in(dataIn);
		RagdollSettings::RagdollResult result = RagdollSettings::sRestoreFromBinaryState(stream_in);
		if (result.HasError()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, ERROR "Failed to load binary file: %s" RESET, result.GetError().c_str());
			return false;
		}


		JPH::Ragdoll* ragdoll = result.Get()->CreateRagdoll(0, 0, &physicsSystem);
		ragdoll->AddToPhysicsSystem(EActivation::Activate);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityType>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
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
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;


		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		Vec3 pos = RVec3(1.0f, 7.0f, 0.0f);

		Ref<RagdollSettings> mRagdollSettings = RagdollLoader::createArm(pos,1.0f);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityType>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
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
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;


		JPH::PhysicsSystem& physicsSystem = ecs.get<PhysicsSystemRef>().physicsSystem;

		RVec3 pos = RVec3(transform.position.x, transform.position.y, transform.position.z);

		Ref<RagdollSettings> mRagdollSettings = RagdollLoader::createSnake(pos, 1.0f);

		const flecs::entity entity = ecs.entity(name.c_str())
			.set<EntityType>({ EntityType::Humanoid })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			//.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
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

		if (!EntityFactory::validateName(name)) return false;
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
			.set<EntityType>({ EntityType::Sensor })
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
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);

		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;
		
		float meshX;
		float meshY;
		float meshZ;

		// Assuming modelSource has only 1 mesh
		calculateMeshSize(modelSource->meshes[0], meshX, meshY, meshZ);

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
			.set<EntityType>({ EntityType::Capsule })
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource->createInstance())
			.set<ModelSourceRef>({ ModelSrcName })
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
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

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
			.set<EntityType>({ EntityType::Actor})
			.add<DynamicEnt>()
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource->createInstance())
			.set<JoltCharacter>({ joltCharacter })
			.set<JPH::BodyID>(joltCharacter->GetBodyID())
			.set<ModelSourceRef>({ ModelSrcName })
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
	static bool createRenderableEntity(flecs::world& ecs, flecs::entity parent, std::string name, ModelSource& modelSource, Transform transform, const char* pipelineName = NULL) {

		if (!EntityFactory::validateName(name)) return false;
		if (!EntityFactory::validateTransform(transform, name.c_str())) return false;


		const flecs::entity entity = ecs.entity(name.c_str())
			.set<Transform>(transform)
			.set<ModelInstance>(modelSource.createInstance());
		if (pipelineName) {
			entity.add<RenderPipeline>(ecs.lookup(pipelineName));

		}

		if (!validateEntityCreation(entity, name)) return false;

		return true;

	}

	static bool createStaticMeshEntity(flecs::world& ecs, const flecs::entity parent, const std::string name, const std::string ModelSrcName, Transform transform, const std::string pipelineName) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		float scaleFactor = 1.0f;

		//Get the modelSource from Asset Library
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

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
			.set<EntityType>({ EntityType::StaticMesh })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelSourceRef>({ ModelSrcName })
			.set<ModelInstance>(modelSource->createInstance())
			.set<JPH::BodyID>(physicsID)
			.add<RenderPipeline>( ecs.lookup(pipelineName.c_str()))
			.child_of(parent);


		// Store the entity ID in the physics body which gives us a two way mapping between entity and bodyId
		bodyInterface.SetUserData(physicsID, entity.id());

		if (!validateEntityCreation(entity, name)) return false;

		return true;
	}

	
	static bool createGridEntity(flecs::world& ecs, const flecs::entity parent, const std::string name, const std::string ModelSrcName, Transform transform, const std::string pipelineName, int rows, int cols) {

		if (!validateName(ecs, parent, name)) return false;
		if (!validateTransform(transform, name.c_str())) return false;
		if (!validatePipelineExistence(ecs, pipelineName)) return false;

		//Get the modelSource from Asset Library
		AssetLibRef ref = ecs.get<AssetLibRef>();
		ModelSource* modelSource = ref.assetLib->get(ModelSrcName);
		if (!validateModelSrcExistence(modelSource, ModelSrcName)) return false;

		// any thickness less than 0.01 will break jolt!
		float boxThickness = 1;
		Vec3 boxHalfExtents(rows * 0.5, boxThickness * 0.5, cols * 0.5);

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
			.set<EntityType>({ EntityType::Grid })
			.add<StaticEnt>()
			.set<Transform>(transform)
			.set<ModelSourceRef>({ ModelSrcName })
			.set<ModelInstance>(modelSource->createInstance())
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
			.set<EntityType>({ EntityType::Player })
			.child_of(parent);
		playerEntity.emplace<Player>(ecs, JPH::Vec3(1.0f, 15.0f, 0.0f), JPH::Quat(0.0f, 0.0f, 0.0f, 1.0f), 2.0f, 1.0f, playerEntity.id());

		ecs.set<PlayerRef>({ playerEntity });

		if (!validateEntityCreation(playerEntity, playerName)) return false;


		flecs::entity playerCam = ecs.entity(playerCamName.c_str())
			.set<EntityType>({ EntityType::Camera })
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


	static bool validateName(std::string name) {
		if (name.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error name cannot be empty");
			return false;
		}
		if (name.length() > 256) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Entity name %s  ' exceeds 256 characters ", name.c_str());
			return false;
		}

		//TODO validate Entity Name uniqueness
		// Check if an entity with the same name already exists in the global scope
		//flecs::entity existing;
		//existing = ecs.lookup(name.c_str());

		return true;
	}

	static bool validateName(flecs::world& ecs, flecs::entity parent, std::string name) {
		if (name.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error name cannot be empty");
			return false;
		}
		if (name.length() > 256) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Entity name '%s' exceeds 256 characters", name.c_str());
			return false;
		}

		// Check if an entity with the same name already exists in this scope
		flecs::entity existing;

		if (parent.is_valid()) {
			// Look up the entity within the parent's scope
			existing = parent.lookup(name.c_str());
		}
		
		if (existing.is_valid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "EntityFactory Error: Entity with name '%s' already exists under parent '%s'",
				name.c_str(),
				parent.name().c_str());
			return false;
		}
		return true;
	}

	static bool validateTransform(Transform transform, std::string name) {
		if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) || !std::isfinite(transform.position.z)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid position (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.rotation.x) || !std::isfinite(transform.rotation.y) ||
			!std::isfinite(transform.rotation.z) || !std::isfinite(transform.rotation.w)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid rotation (contains NaN or Inf) found in transform for : %s", name.c_str());
			return false;
		}
		if (!std::isfinite(transform.scale.x) || !std::isfinite(transform.scale.y) ||
			!std::isfinite(transform.scale.z)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error Invalid scale (contains NaN or Inf) found in transform for : %s", name.c_str());
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
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error: Entity %s size contains NaN or Inf", name.c_str());
			return false;
		}

		// Minimum constraint check
		if (size.GetX() < minSizeConstraint.GetX() || size.GetY() < minSizeConstraint.GetY() || size.GetZ() < minSizeConstraint.GetZ()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR,
				"Error: Size components for entity %s must be >= 0.1 m (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		// Maximum constraint check
		if (size.GetX() > maxSizeConstraint.GetX() || size.GetY() > maxSizeConstraint.GetY() || size.GetZ() > maxSizeConstraint.GetZ()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"Warning: Entity %s size exceeds recommended bounds for %s objects (x: %.3f, y: %.3f, z: %.3f)",
				name.c_str(), dynamicObject ? "dynamic" : "static", size.GetX(), size.GetY(), size.GetZ());
			return false;
		}

		return true;
	}

	static bool validatePhysicsBodyCreation(JPH::BodyID id, std::string name) {

		if (id.IsInvalid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating physics body for Entity : %s", name.c_str());
			return false;
		}
		return true;
	}

	static bool validatePipelineExistence(flecs::world& ecs,std::string pipelineName) {

		flecs::entity pipelineEnt = ecs.lookup(pipelineName.c_str());

		if (!pipelineEnt.is_valid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Pipeline entity does not exist : %s", pipelineName.c_str());
			return false;
		}
	}
	static bool validateEntityCreation(flecs::entity entity, std::string name) {
		if (!entity.is_valid()) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error creating Entity : %s", name.c_str());
			return false;
		}
		return true;
	}

	static bool validateModelSrcExistence(ModelSource* model,const std::string modelName) {

		if (!model) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, "EntityFactory Error ModelSource does not exist! : %s", modelName.c_str());
			return false;
		}
		return true;

	}

	static bool validateRagdollExistence(const std::string key, const std::map<std::string, std::string>& ragdolls) {

		if (!ragdolls.contains(key)) {
			SDL_LogError(SDL_LOG_CATEGORY_ERROR, ERROR "Ragdoll list does not have the following key : %s" RESET, key.c_str());
			return false;
		}

		return true;

	}

	//TODO move this function
	static void calculateMeshSize(const MeshSource& mesh, float& x, float& y, float& z) {
		if (mesh.vertices.empty()) {
			//width = 0.0f;
			//height = 0.0f;
			cout << "MESH DIMENSIONS ARE ZERO !!!\n";
			return;
		}

		float minX = FLT_MAX, maxX = -FLT_MAX;
		float minY = FLT_MAX, maxY = -FLT_MAX;
		float minZ = FLT_MAX, maxZ = -FLT_MAX;

		for (const auto& current : mesh.vertices) {

			minX = std::min(minX, current.position.x);
			maxX = std::max(maxX, current.position.x);

			minY = std::min(minY, current.position.y);
			maxY = std::max(maxY, current.position.y);

			minZ = std::min(minZ, current.position.z);
			maxZ = std::max(maxZ, current.position.z);
		}

		x = maxX - minX;
		y = maxY - minY;
		z = maxZ - minZ;
	}


};





