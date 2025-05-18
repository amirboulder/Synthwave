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
	

	GLuint diffuseTextureID;
	//GLuint SpecularTextureID;

	GLuint VAO;

	Mesh(aiMesh* importedMesh, glm::mat4 localtransform = glm::mat4(1.0f)) {

		vertices.reserve(importedMesh->mNumVertices);

		for (int i = 0; i < importedMesh->mNumVertices; i++) {

			vertices.emplace_back();
			VertexData& currentVertex = vertices.back();

			currentVertex.vertex.x = importedMesh->mVertices[i].x;
			currentVertex.vertex.y = importedMesh->mVertices[i].y;
			currentVertex.vertex.z = importedMesh->mVertices[i].z;

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

	}

	Mesh(const Mesh& other)
	:	vertices(other.vertices),
		indices(other.indices),
		diffuseTextureID(other.diffuseTextureID),
		//SpecularTextureID(SpecularTextureID),
		//VBO(other.VAO),
		
		//EBO(other.EBO)
		VAO(other.VAO)

	{
		printf("Mesh copy constructor called!\n");
	}

	void processMesh() {
		GLuint VBO, EBO;

		// Create VAO, VBO, and EBO
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);
		glCreateBuffers(1, &EBO);

		// Upload vertex data to VBO
		glNamedBufferData(VBO, vertices.size() * sizeof(VertexData), vertices.data(), GL_STATIC_DRAW);
		// Upload index data to EBO
		glNamedBufferData(EBO, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		// Bind VBO to VAO
		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(VertexData));

		// Bind EBO to VAO
		glVertexArrayElementBuffer(VAO, EBO);

		// Position attribute (index 0)
		glEnableVertexArrayAttrib(VAO, 0);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(VertexData, vertex));

		// Normal attribute (index 1)
		glEnableVertexArrayAttrib(VAO, 1);
		glVertexArrayAttribBinding(VAO, 1, 0);
		glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(VertexData, normal));

		// Texture coordinates attribute (index 2)
		glEnableVertexArrayAttrib(VAO, 2);
		glVertexArrayAttribBinding(VAO, 2, 0);
		glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(VertexData, texCoords));
	}
	
	void transfromMesh(glm::mat4 transfrom) {
	
		for (int i = 0; i < vertices.size(); i++) {
			
			glm::vec4 v4 = glm::vec4(vertices[i].vertex, 1.0f);

			glm::vec4 transformedV4 = transfrom * v4;

			vertices[i].vertex = glm::vec3(transformedV4);

		}

	}
	
	void draw(GLuint & shaderID,glm::mat4 & meshMatrix) {

		glUseProgram(shaderID);

		Shader::setMat4("model", meshMatrix, shaderID);

		glBindTextureUnit(0, diffuseTextureID);

		glUniform1i(glGetUniformLocation(shaderID, "textureDiffuse"), 0);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		//glBindVertexArray(0)
	}

};