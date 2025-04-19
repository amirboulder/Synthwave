
#include "core/src/Entity.hpp"

class Scene {

public:
	
	vector <Entity>  entities;
	vector <Shader>  shaders;

	Scene(int levelNum) {

		//TODO add switch statment for more levels

	}


	void constructLevel1() {

		
		shaders.emplace_back("shaders/bindlessTextureShader.vs", "shaders/bindlessTextureShader.fs");
		glm::mat4 pillMatrix = { { 1, 0, 0, 0 }, {0,1,0,0}, {0,0,1,0}, {5,5,5,1} };
		entities.emplace_back("assets/pill.obj", pillMatrix, shaders.back().m_shaderID);

		//shaders.emplace_back("shaders/bindlessTextureShader.vs", "shaders/bindlessTextureShader.fs");
		//glm::mat4 sponzaMatrix = { { 0.01, 0, 0, 0 }, {0,0.01,0,0}, {0,0,0.01,0}, {5,5,5,1} };
		//entities.emplace_back("assets/sponza/sponza.obj", sponzaMatrix, shaders.back().m_shaderID);


		shaders.emplace_back("shaders/wireframeShader.vs", "shaders/wireframeShader.fs",
			"shaders/wireframeShader.gs");
		glm::mat4 testModelMatix = glm::mat4(1.0f);
		testModelMatix = glm::scale(testModelMatix, { 1.0,1.0,1.0 });
		testModelMatix = glm::translate(testModelMatix, { 50,-50,50 });
		stbi_set_flip_vertically_on_load(true);
		entities.emplace_back("assets/mtn2.obj", testModelMatix, shaders.back().m_shaderID);


	}

};