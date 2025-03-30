#pragma once

#include "renderer/Model.hpp"

class Entity {

public:

	Model model;

	glm::vec3 position;

	glm::vec3 veclocity;
	float acceleration;

	Entity(const char* filePath, glm::mat4 modelMatrix,GLuint ShaderID)
		: model(filePath,modelMatrix,ShaderID)

	{
		this->position = { 0.01f,0,0 };
		this->veclocity = { 0.01f,0.0f,0.01f };
		this->acceleration = 1.0f;


	}

	void update() {

		this->acceleration = glm::clamp(this->acceleration + 0.0001f, 0.0f, 1.0f);
		this->veclocity *= acceleration;
		this->position += veclocity;

		// Maybe this faster ??
		// TODO TEST to see if it makes a difference
		// cause I think we're making a new matrix each time
		this->model.modelMatrix[3][0] = position.x;
		this->model.modelMatrix[3][1] = position.y;
		this->model.modelMatrix[3][2] = position.z;

		//this->model.modelMatrix = glm::translate(glm::mat4(1.0f), this->position);
	}

	void draw() {
		this->model.draw();
	}

};