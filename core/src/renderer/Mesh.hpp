#pragma once

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../components.hpp"
#include "Shader.hpp"
#include "Grid.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using std::vector, std::string;


class Mesh {
public:
	vector<VertexData> vertices;
	vector <unsigned int> indices;
	//vector<Texture2D> m_textures;

	vector <GLuint64> textureHandlers;

	GLuint diffuseTextureID;
	GLuint SpecularTextureID;

	GLuint ssbo = 0;

	// create vertex buffer , vertex array,  Element Buffer objects
	GLuint VBO, VAO, EBO;

	Mesh(aiMesh* importedMesh, glm::mat4 localtransform = glm::mat4(1.0f)) {

		vertices.reserve(importedMesh->mNumVertices);

		for (int i = 0; i < importedMesh->mNumVertices; i++) {

			vertices.emplace_back();
			VertexData& currentVertex = vertices.back();

			currentVertex.vertices.x = importedMesh->mVertices[i].x;
			currentVertex.vertices.y = importedMesh->mVertices[i].y;
			currentVertex.vertices.z = importedMesh->mVertices[i].z;

			if (importedMesh->HasNormals()) {
				currentVertex.normal.x = importedMesh->mNormals[i].x;
				currentVertex.normal.y = importedMesh->mNormals[i].y;
				currentVertex.normal.z = importedMesh->mNormals[i].z;
			}

			//Mesh can have multiple texture coordinates we're just using the first one for now.
			importedMesh->mTextureCoords;
			if (importedMesh->mTextureCoords[0]) 
			{
				currentVertex.texCoords.x = importedMesh->mTextureCoords[0][i].x;
				currentVertex.texCoords.y = importedMesh->mTextureCoords[0][i].y;
			}

		}

		indices.reserve(importedMesh->mNumFaces * 3);

		for (int i = 0; i < importedMesh->mNumFaces; i++) {


			for (int j = 0; j < importedMesh->mFaces[i].mNumIndices; j++) {
				indices.emplace_back(importedMesh->mFaces[i].mIndices[j]);

			}


		}

	}

	
	// for generated meshes
	Mesh(int rows,int cols)
		
	{
		GridGenerator::generateGrid(rows, cols, vertices, indices);

		//cout << "vertices memory (used): " << vertices.size() * sizeof(VertexData) / 1'000'000 << " Megabytes\n";
		//cout << "vertices memory (reserved): " << vertices.capacity() * sizeof(VertexData) / 1'000'000 << " Megabytes\n";

		//cout << "indices memory (used): " << indices.size() * sizeof(unsigned int) / 1'000'000 << " Megabytes\n";
		//cout << "indices memory (reserved): " << indices.capacity() * sizeof(unsigned int) / 1'000'000 << " Megabytes\n";

	}

	Mesh(const Mesh& other) {
		printf("Mesh copy constructor called!\n");
	}

	void processMesh() {

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		// BIND tje VertexData Array object
		glBindVertexArray(VAO);

		// bind the vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//configure the currently bound buffer

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexData), &vertices[0], GL_STATIC_DRAW);

		//BiND ELEMENT buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//tell open gl how to interprate the data
		//position arrtibute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);


		//VERTEX NARMAL !!!
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));


		//texture attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texCoords));
		glEnableVertexAttribArray(2);

		
		// deprecating bindless textures because renderdoc does not support them
		/*
		if (!textureHandlers.empty()) {
			glGenBuffers(1, &ssbo);
			//cout << "SSBO : " << ssbo << '\n';
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * textureHandlers.size(),
				textureHandlers.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
		*/

	}
	
	

	// TODO deprecate
	void draw(GLuint & shaderID,glm::mat4 & meshMatrix) {

		glUseProgram(shaderID);

		Shader::setMat4("model", meshMatrix, shaderID);

		// Bind texture to unit 0 (or whatever unit you prefer)
		glBindTextureUnit(0, diffuseTextureID);
		// Set uniform
		glUniform1i(glGetUniformLocation(shaderID, "textureDiffuse"), 0);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void drawDSA(GLuint& shaderID, glm::mat4& meshMatrix) {

		glUseProgram(shaderID);


		Shader::setMat4("model", meshMatrix, shaderID);

		// Bind texture to unit 0 (or whatever unit you prefer)
		glBindTextureUnit(0, diffuseTextureID);
		// Set uniform
		glUniform1i(glGetUniformLocation(shaderID, "textureDiffuse"), 0);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

	}


	// TODO deprecate
	void drawBindless(GLuint& shaderID, glm::mat4& meshMatrix) {
		glUseProgram(shaderID);

		// Bind SSBO to binding point (match this in your shader)
		if (ssbo) {

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);
		}

		Shader::setMat4("model", meshMatrix, shaderID);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	/*
	const char* getFileExtension(const char* imagePath) {
		const char* dot = strrchr(imagePath, '.'); // Find last '.'
		if (!dot || dot == imagePath) {
			return NULL; // No extension found
		}
		return dot + 1; // Skip the dot and return the extension
	}
	*/


};