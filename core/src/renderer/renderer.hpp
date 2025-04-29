#include <vector>
#include "../Entity.hpp"
#include "Grid.hpp"
#include "Camera.hpp"
#include "window.hpp"
#include "text/freeType.hpp"

struct Renderer {

	TextRenderer textRenderer;
	Camera camera;
	GLuint uboMatrices;
	glm::mat4 view;
	glm::mat4 projection;

	// TODO maybe don't store this data as it may become stale
	int winWidth, winHeight;



	Renderer(int winWidth,int winHeight)
		: camera(glm::vec3(0.0f, 10.0f, 0.0f)),
		textRenderer(winWidth, winHeight, "assets/fonts/Supermolot Light.otf"),
		winWidth(winWidth),winHeight(winHeight)

	{

		setup();

	}

	void setup() {

		//VSync
		glfwSwapInterval(1);
		
		//OPENGL SETTINGS
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
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
		projection = camera.getProjectionMatrix(static_cast<float>(winWidth) / static_cast<float>(winHeight));

	}

	void drawFps(int fps) {
		
		textRenderer.renderText(std::to_string(fps), 0.0, winHeight - 60, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	}

	void drawText(std::string text, glm::vec2 postion, float scale, glm::vec3 color) {
		textRenderer.renderText(text, postion.x, postion.y, scale, color);
	}

	/*
	void draw( std::vector<std::unique_ptr<Entity>>& entities) {


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

		for ( auto& entity : entities) {
			entity->draw(); 
		}

	}
	*/


	void draw(std::vector<Model>& models, std::vector<TransformData> & transforms) {


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

		for (auto& model : models) {
			model.draw(transforms);
		}

	}

	void handleWindowResize(int newWidth , int newHeight) {

		winWidth = newWidth;
		winHeight = newHeight;
		projection = camera.getProjectionMatrix(static_cast<float>(newWidth) / static_cast<float>(newHeight));
		textRenderer.updateProjection(newWidth, newHeight);

	}

};