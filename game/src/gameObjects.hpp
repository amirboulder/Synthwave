
#include "core/src/Entity.hpp"

#include "core/src/EntityFactory.hpp"



class Scene {

public:
	
	std::vector<std::unique_ptr<Entity>> entities;

	vector <Shader>  shaders;

	std::vector<TransformData> transforms;
	
	std::vector<Model> models;

	std::vector<PhysicsData> physicsCompoments;

	Player player;
	
	Scene(int levelNum) {

		//TODO add switch statment for more levels

	}

	
	void constructLVL1(Fisiks & physik) {

		transforms.reserve(1001);
		models.reserve(1001);
		physicsCompoments.reserve(1001);

		EntityFactory factory(entities,physicsCompoments,models, transforms);


		shaders.emplace_back("shaders/shadersDSA/simpleShader.vs", "shaders/shadersDSA/simpleShader.fs");
		//glm::mat4 capsuleMatrix = glm::mat4(1.0f);
		//capsuleMatrix = glm::scale(capsuleMatrix, { 1.0,1.0,1.0 });
		//capsuleMatrix = glm::translate(capsuleMatrix, { 10,10,10 });

		//Entity x = factory.createCapsuleEntity("assets/pill.obj", shaders.back().m_shaderID, physik, capsuleMatrix);

		//capsuleMatrix = glm::translate(capsuleMatrix, { 0,10,0 });

		//factory.createCapsuleEntity("assets/pill.obj", shaders.back().m_shaderID, physik, capsuleMatrix);

		//capsuleMatrix = glm::translate(capsuleMatrix, { 20,10,20 });
		//factory.createCapsuleEntity("assets/pill.obj", shaders.back().m_shaderID, physik, capsuleMatrix);


		//shaders.emplace_back("shaders/shadersDSA/simpleShader.vs", "shaders/shadersDSA/simpleShader.fs");
		glm::mat4 capsuleMatrix = glm::mat4(1.0f);
		capsuleMatrix = glm::translate(capsuleMatrix, { 2,7,2 });
		capsuleMatrix = glm::scale(capsuleMatrix, { 1.0f,1.0f,1.0f });
		
		for (int i = 0; i < 1; i++) {

			//factory.createCapsuleEntity("assets/pill.obj", shaders.back().m_shaderID, physik, capsuleMatrix);
			//factory.createCapsuleEntity("assets/capsule.glb", shaders.back().m_shaderID, physik, capsuleMatrix);
			//factory.createCapsuleEntity("assets/capsule2.glb", shaders.back().m_shaderID, physik, capsuleMatrix);
			factory.createCapsuleEntity("assets/capsule3.glb", shaders.back().m_shaderID, physik, capsuleMatrix);

			capsuleMatrix = glm::translate(capsuleMatrix, { 3,3,3 });


		}

		factory.createPlayerEntity(player,"assets/capsule3.glb", shaders.back().m_shaderID, physik, capsuleMatrix);
		
		
		// Mountains
		shaders.emplace_back("shaders/wireframeShader.vs", "shaders/wireframeShader.fs",
			"shaders/wireframeShader.gs");
		glm::mat4 mtnMatrix = glm::mat4(1.0f);
		mtnMatrix = glm::scale(mtnMatrix, { 1.0,1.0,1.0 });
		mtnMatrix = glm::translate(mtnMatrix, { 50,-50,50 });
		stbi_set_flip_vertically_on_load(true);
	
		factory.createStaticMeshEntity("assets/mtn2.obj", shaders.back().m_shaderID, physik, mtnMatrix);
		

		
		shaders.emplace_back("shaders/grid2Shader.vs", "shaders/grid2Shader.fs");
		glm::mat4 gridMatrix = glm::mat4(1.0f);
		Entity grid = factory.createGridEntity(shaders.back().m_shaderID, physik, gridMatrix,256,256);
		



	}

};