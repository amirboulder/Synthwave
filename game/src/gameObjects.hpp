
#include "core/src/Entity.hpp"

#include "core/src/EntityFactory.hpp"

#include "core/src/physics/physicsUtil.hpp"

using std::vector;

class Scene {

public:
	
	vector<Entity> entities;

	vector <Shader>  shaders;

	vector<TransformData> transforms;
	vector<PhysicsData> physicsCompoments;

	vector<Model> models;



	vector<TransformData2> StaticEntTransfroms;
	vector<PhysicsData> staticEntPhysics;

	Player player;
	
	Scene(int levelNum) {

		//TODO add switch statment for more levels

	}

	void constructLVL1(Fisiks & physik) {

		int reserveSize = 50;

		entities.reserve(reserveSize);
		transforms.reserve(reserveSize);
		models.reserve(reserveSize);
		physicsCompoments.reserve(reserveSize);

		EntityFactory factory(entities,physicsCompoments,models, transforms);

		shaders.emplace_back("shaders/shadersDSA/simpleShader.vs", "shaders/shadersDSA/simpleShader.fs");

		//Player
		glm::mat4 playerMatrix = glm::mat4(1.0f);
		playerMatrix = glm::translate(playerMatrix, { 2,7,2 });
		factory.createPlayerEntity(player, "assets/capsule4.glb", shaders.back().m_shaderID, physik, playerMatrix);

		//Capsule
		glm::mat4 capsuleMatrix = glm::mat4(1.0f);
		capsuleMatrix = glm::translate(capsuleMatrix, { 18,6,13 });
		capsuleMatrix = glm::scale(capsuleMatrix, { 1.0f,1.0f,1.0f });
		factory.createCapsuleEntity("assets/capsule4.glb", shaders.back().m_shaderID, physik, capsuleMatrix);
		factory.createCapsuleEntity("assets/capsule4.glb", shaders.back().m_shaderID, physik, capsuleMatrix);

		
		//Box
		glm::mat4 boxMatrix = glm::mat4(1.0f);
		boxMatrix = glm::translate(boxMatrix, { 10,6,10 });
		boxMatrix = glm::scale(boxMatrix, { 1.0f,1.0f,1.0f });
		factory.createBoxEntity("assets/cube.obj", shaders.back().m_shaderID, physik, boxMatrix);

		// Mountains
		shaders.emplace_back("shaders/wireframeShader.vs", "shaders/wireframeShader.fs",
			"shaders/wireframeShader.gs");
		glm::mat4 mtnMatrix = glm::mat4(1.0f);
		mtnMatrix = glm::scale(mtnMatrix, { 1.0,1.0,1.0 });
		mtnMatrix = glm::translate(mtnMatrix, { 50,-50.0,50 });
		stbi_set_flip_vertically_on_load(true);
		factory.createStaticMeshEntity("assets/mtn2.obj", shaders.back().m_shaderID, physik, mtnMatrix);
		

		//GRID
		shaders.emplace_back("shaders/grid2Shader.vs", "shaders/grid2Shader.fs");
		glm::mat4 gridMatrix = glm::mat4(1.0f);
		Entity grid = factory.createGridEntity(shaders.back().m_shaderID, physik, gridMatrix,256,256);
		
	}

	void LVL1Script(PhysicsSystem & physicsSystem, JPH::Vec3Arg playerPos) {

		float moveSpeed = 3;

		BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

		for (Entity& ent : entities) {

			JPH::Vec3 entityPos = bodyInterface.GetPosition(ent.physicsID);

			JPH::Quat entityRot = bodyInterface.GetRotation(ent.physicsID);
			JPH::Quat UprightRot = JPH::Quat(0,0,0,1);

			//JPH::TransformedShape entityShape = bodyInterface.GetTransformedShape(ent.physicsID);
			JPH::AABox entityAABOX = bodyInterface.GetTransformedShape(ent.physicsID).GetWorldSpaceBounds();
			JPH::Vec3 entityExtent =  entityAABOX.GetExtent();

			FUtil::GroundInfo groundInfo = FUtil::CheckGround(physicsSystem,entityPos,entityAABOX,entityExtent, ent.physicsID);

			if (groundInfo.isGrounded ) {

				JPH::Vec3 direction(playerPos.GetX() - entityPos.GetX(), 0, playerPos.GetZ() - entityPos.GetZ());

				if (direction.LengthSq() > 0.0f) {
					direction = direction.Normalized();
				}

				JPH::Vec3 desiredVelocity = direction * moveSpeed;

				bodyInterface.SetLinearVelocity(ent.physicsID, desiredVelocity);

				bodyInterface.SetRotation(ent.physicsID,entityRot, JPH::EActivation::Activate);
			}
		}
	}

};