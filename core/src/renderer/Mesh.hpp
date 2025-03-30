#pragma once

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Shader.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using std::vector, std::string;


struct Texture2D {
	GLuint textureRef;
	int width, height, nrChannels;


};



struct Vertex {
	glm::vec3 vertices;
	glm::uvec3 normal;
	glm::vec2 texCoords;
};


class Mesh {
public:
	vector<Vertex> vertices;
	vector <unsigned int> indices;
	vector<Texture2D> m_textures;

	vector <GLuint64> textureHandlers;

	GLuint ssbo = 0;

	// create vertex buffer , vertex array,  Element Buffer objects
	GLuint VBO, VAO, EBO;

	size_t m_indSize;

	glm::mat4 meshMatrix = glm::mat4(1.0f);

	Mesh(aiMesh* importedMesh, glm::mat4 localtransform) {

		meshMatrix = localtransform;

		vertices.reserve(importedMesh->mNumVertices);

		for (int i = 0; i < importedMesh->mNumVertices; i++) {

			vertices.emplace_back();
			Vertex& currentVertex = vertices.back();

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

		//TODO REMOVE
		m_indSize = indices.size();

		if (textureHandlers.size() == 0) {
			processMesh();
		}

		
	}

	void processMesh() {

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		// BIND tje Vertex Array object
		glBindVertexArray(VAO);

		// bind the vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		//configure the currently bound buffer

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		//BiND ELEMENT buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//tell open gl how to interprate the data
		//position arrtibute
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);


		//VERTEX NARMAL !!!
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));


		//texture attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
		glEnableVertexAttribArray(2);

		cout << "texture handlers size :" << textureHandlers.size() << '\n';
		
		if (!textureHandlers.empty()) {
			glGenBuffers(1, &ssbo);
			cout << "SSBO : " << ssbo << '\n';
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * textureHandlers.size(),
				textureHandlers.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}

	}
	
	/*

	void draw(GLuint shaderID) {
		// Ensure we're using the correct shader
		glUseProgram(shaderID);

		// Loop through textures and pass their bindless handles
		for (int i = 0; i < m_textures.size(); i++) {

			glActiveTexture(GL_TEXTURE0 + i);

			string name = "texture";
			string number = std::to_string(i + 1);

			Shader::setInt("texturediffuse", m_textures[i].textureRef, shaderID);

			glBindTexture(GL_TEXTURE_2D, m_textures[i].textureRef);
			//GLuint64 textureHandle = m_textures[i].textureRef;  // Get bindless handle
			//glMakeTextureHandleResidentARB(textureHandle);  // Make texture resident
			//std::string uniformName = "textures[" + std::to_string(i) + "]";
			//Shader::setUint64("textures", textureHandle, shaderID);
		//	Shader::setUint64(uniformName.c_str(), textureHandle, shaderID);  // Pass handle to shader
			//glMakeTextureHandleNonResidentARB(textureHandle);
		}

		Shader::setMat4("model", meshMatrix, shaderID);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, m_indSize, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glActiveTexture(GL_TEXTURE0);
	}
	*/


	void draw(GLuint shaderID) {
		glUseProgram(shaderID);

		// Bind SSBO to binding point (match this in your shader)
		if (ssbo) {
			
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo); 
		}

		Shader::setMat4("model", meshMatrix, shaderID); // Assuming this is a static helper

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}


	const char* getFileExtension(const char* imagePath) {
		const char* dot = strrchr(imagePath, '.'); // Find last '.'
		if (!dot || dot == imagePath) {
			return NULL; // No extension found
		}
		return dot + 1; // Skip the dot and return the extension
	}


};