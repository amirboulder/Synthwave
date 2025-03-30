#include <vector>
#include "../Entity.hpp"
#include "Grid.hpp"
#include "Camera.hpp"
#include "window.hpp"

struct Renderer {

	vector <Entity>& entities;
	GLuint uboMatrices;
	Camera camera;
	glm::mat4 view;
	glm::mat4 projection;



	Grid grid;

	//TODO EXTRACT GRID FROM RENDERER CONSTRUCTOR

	Renderer(vector <Entity>& entities)
		: camera(glm::vec3(0.0f, 10.0f, 0.0f)),
		entities(entities),
		grid("shaders/grid2Shader.vs", "shaders/grid2Shader.fs", 1600, 1600, false)
	{

		setup();

	}



	void setup() {

		

		//OPENGL SETTINGS
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glGenBuffers(1, &uboMatrices);
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// Bind it to a specific binding point 
		glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboMatrices);

		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));



		view = camera.getViewMatrix();
		projection = camera.getProjectionMatrix();


	}



	void draw() {


		view = camera.getViewMatrix();

		// Update View & Projection matrices
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));


		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		float r = 47.0f;
		float g = 34.0f;
		float b = 88.0f;

		glClearColor(r / 256.0f, g / 256.0f, b / 256.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		grid.renderGrid();

		//mtn1.renderMtn();


		for (Entity& e : entities) {

			e.draw();

		}

	}

};