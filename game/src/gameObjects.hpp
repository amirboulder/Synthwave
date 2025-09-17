#include "core/src/Entity.hpp"
#include "core/src/EntityFactory.hpp"
#include "core/src/physics/physicsUtil.hpp"

#include "core/src/state/stateManager.hpp"

using std::vector;

class Scene {

public:
	
	Entities dynamicEnts;
	Entities staticEnts;
	Entities mtnEnts;

	Player & player;

	StateManager & stateManager;

	Fisiks& fisiks;

	Renderer& renderer;

	Scene(Fisiks& fisiks, Renderer& renderer, StateManager& stateManager, Player& player)
		:fisiks(fisiks), renderer(renderer), stateManager(stateManager), player(player)
	{

		//=========== creating shaders
		// 
		//reserve multiple to avoid reallocation
		renderer.pipelines.reserve(4);
		//create BP pipeline 
		renderer.pipelines.emplace_back(dynamicEnts.models, dynamicEnts.transforms);
		Pipeline& pipelineBP = renderer.pipelines[0];

		//shader::generateSpirvShaders("shaders/slang/shaders.slang", "shaders/compiled/VertexShader.spv", "shaders/compiled/FragmentShader.spv");
		PL::loadVertexShader(renderer.device, pipelineBP.vertexShader,"shaders/compiled/VertexShader.spv", 0, 2, 0, 0);
		PL::loadFragmentShader(renderer.device, pipelineBP.fragmentShader,"shaders/compiled/FragmentShader.spv", 1, 0, 0, 0);
		renderer.createPipeline(pipelineBP.vertexShader, pipelineBP.fragmentShader, pipelineBP.pipeline);

		renderer.pipelines.emplace_back(staticEnts.models, staticEnts.transforms);
		Pipeline& pipelineGrid = renderer.pipelines[1];

		//shader::generateSpirvShaders("shaders/slang/gridshader.slang", "shaders/compiled/grid.vert.spv", "shaders/compiled/grid.frag.spv");
		PL::loadVertexShader(renderer.device, pipelineGrid.vertexShader,"shaders/compiled/grid.vert.spv", 0, 2, 0, 0);
		PL::loadFragmentShader(renderer.device, pipelineGrid.fragmentShader,"shaders/compiled/grid.frag.spv", 0, 0, 0, 0);
		renderer.createPipeline(pipelineGrid.vertexShader, pipelineGrid.fragmentShader, pipelineGrid.pipeline);

		renderer.pipelines.emplace_back(mtnEnts.models, mtnEnts.transforms);
		Pipeline& pipelineMtn = renderer.pipelines[2];

		//shader::generateSpirvShaders("shaders/slang/wireframe.slang", "shaders/compiled/wireframe.vert.spv", "shaders/compiled/wireframe.frag.spv");
		PL::loadVertexShader(renderer.device, pipelineMtn.vertexShader,"shaders/compiled/wireframe.vert.spv", 0, 2, 0, 0);
		PL::loadFragmentShader(renderer.device, pipelineMtn.fragmentShader,"shaders/compiled/wireframe.frag.spv", 0, 0, 0, 0);
		pipelineMtn.createPipeline(renderer.window,renderer.device,renderer.config.sampleCountMSAA);
		pipelineMtn.drawType = 1;



		constructLVL2();



	}

	bool constructLVL1(Fisiks& fisiks, Renderer& renderer) {

		int reserveSize = 50;

		//create Model Sources
		//robot
		ModelSource robotSource("assets/robot4Wheels.glb", renderer.device);
		//capsule
		ModelSource capsuleSource("assets/capsule4.glb", renderer.device);
		//Mountain
		ModelSource mtnSource("assets/mtn2.obj", renderer.device,true);
		//Grid
		ModelSource gridSource(256, 256, renderer.device);


		
		//Create instances

		//Robot1
		dynamicEnts.models.emplace_back();
		dynamicEnts.transforms.emplace_back();
		Transform& robot1Transform = dynamicEnts.transforms.back();
		robot1Transform.position = glm::vec3(1.0f);
		robot1Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		robot1Transform.scale = glm::vec3(1.0f);
		robotSource.createInstance(dynamicEnts.models.back());

		//Capsule1
		dynamicEnts.models.emplace_back();
		dynamicEnts.transforms.emplace_back();
		Transform& capsule1Transfrom = dynamicEnts.transforms.back();
		capsule1Transfrom.position = glm::vec3(1.0f);
		capsule1Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule1Transfrom.scale = glm::vec3(1.0f);
		capsuleSource.createInstance(dynamicEnts.models.back());

		dynamicEnts.models.emplace_back();
		dynamicEnts.transforms.emplace_back();
		Transform& capsule2Transfrom = dynamicEnts.transforms.back();
		capsule2Transfrom.position = glm::vec3(5.0f, -2.0f, 5.0f);
		capsule2Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule2Transfrom.scale = glm::vec3(1.0f);
		capsuleSource.createInstance(dynamicEnts.models.back());




		//Grid
		staticEnts.models.emplace_back();
		staticEnts.transforms.emplace_back();
		Transform& gridTransfrom = staticEnts.transforms.back();
		gridTransfrom.rotation = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);
		gridSource.createInstance(staticEnts.models.back());


		//mtn
		mtnEnts.models.emplace_back();
		mtnEnts.transforms.emplace_back();
		Transform& mtnTransfrom = mtnEnts.transforms.back();
		mtnTransfrom.position = glm::vec3(0.0f, 50.0f, 0.0f);
		mtnSource.createInstance(mtnEnts.models.back());
		mtnEnts.models.back().meshes[0].size = mtnSource.meshes[0].vertices.size();


		return true;
	}

	bool constructLVL2() {

		EntityFactory factory;


		//create Model Sources
		//robot
		ModelSource robotSource("assets/robot4Wheels.glb", renderer.device);
		//capsule
		ModelSource capsuleSource("assets/capsule4.glb", renderer.device);
		//Grid
		ModelSource gridSource(256, 256, renderer.device);

		//buildings
		//ModelSource buildingSource("assets/realistic_chicago_buildings.glb", renderer.device);
		//buildings
		//ModelSource sponzaSource("assets/Sponza/sponza.obj", renderer.device);


		////Robot1
		//dynamicEnts.models.emplace_back();
		//dynamicEnts.transforms.emplace_back();
		//Transform& robot1Transform = dynamicEnts.transforms.back();
		//robot1Transform.position = glm::vec3(1.0f);
		//robot1Transform.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		//robot1Transform.scale = glm::vec3(1.0f);
		//robotSource.createInstance(dynamicEnts.models.back());


		//Player
		stateManager.setPlayer(player);
		stateManager.load();
		player.createPhysicsBody(fisiks.physics_system);


		//Capsule1
		Transform capsule1Transfrom;
		capsule1Transfrom.position = glm::vec3(1.0f,5.0f,0.0f);
		capsule1Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule1Transfrom.scale = glm::vec3(1.0f);
		factory.createCapsuleEntity(dynamicEnts, fisiks,capsuleSource, capsule1Transfrom);


		//Capsule2
		Transform capsule2Transfrom;
		capsule2Transfrom.position = glm::vec3(1.0f, 19.0f, 0.0f);
		capsule2Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule2Transfrom.scale = glm::vec3(1.0f);
		factory.createCapsuleEntity(dynamicEnts, fisiks, capsuleSource, capsule2Transfrom);

		//Capsule2
		Transform capsule3Transfrom;
		capsule3Transfrom.position = glm::vec3(1.0f, 29.0f, 0.0f);
		capsule3Transfrom.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
		capsule3Transfrom.scale = glm::vec3(1.0f);
		factory.createCapsuleEntity(dynamicEnts, fisiks, capsuleSource, capsule2Transfrom);




		//Grid
		Transform gridTransfrom;
		gridTransfrom.rotation = glm::quat(0.0f, 0.0f, 1.0f, 0.0f);
		factory.createGridEntity(staticEnts, fisiks, gridSource, gridTransfrom, 256, 256);

		//buildings
		/*Transform buildingTransform;
		factory.createRenderableEntity(dynamicEnts, fisiks, buildingSource, buildingTransform);*/

		/*Transform sponzaTransform;
		sponzaTransform.scale.x = 0.1;
		sponzaTransform.scale.y = 0.1;
		sponzaTransform.scale.z = 0.1;
		factory.createRenderableEntity(dynamicEnts, fisiks, sponzaSource, sponzaTransform);*/


		return true;
	}

	//void LVL1Script(PhysicsSystem & physicsSystem, JPH::Vec3Arg playerPos) {

	//	float moveSpeed = 3;

	//	BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

	//	for (Entity& ent : entities) {

	//		JPH::Vec3 entityPos = bodyInterface.GetPosition(ent.physicsID);

	//		JPH::Quat entityRot = bodyInterface.GetRotation(ent.physicsID);
	//		JPH::Quat UprightRot = JPH::Quat(0,0,0,1);

	//		//JPH::TransformedShape entityShape = bodyInterface.GetTransformedShape(ent.physicsID);
	//		JPH::AABox entityAABOX = bodyInterface.GetTransformedShape(ent.physicsID).GetWorldSpaceBounds();
	//		JPH::Vec3 entityExtent =  entityAABOX.GetExtent();

	//		FUtil::GroundInfo groundInfo = FUtil::CheckGround(physicsSystem,entityPos,entityAABOX,entityExtent, ent.physicsID);

	//		if (groundInfo.isGrounded ) {

	//			JPH::Vec3 direction(playerPos.GetX() - entityPos.GetX(), 0, playerPos.GetZ() - entityPos.GetZ());

	//			if (direction.LengthSq() > 0.0f) {
	//				direction = direction.Normalized();
	//			}

	//			JPH::Vec3 desiredVelocity = direction * moveSpeed;

	//			bodyInterface.SetLinearVelocity(ent.physicsID, desiredVelocity);

	//			bodyInterface.SetRotation(ent.physicsID,entityRot, JPH::EActivation::Activate);
	//		}
	//	}
	//}

};