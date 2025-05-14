#pragma once

#include <Jolt/Jolt.h>

#ifndef JPH_DEBUG_RENDERER
#error This file should only be included when JPH_DEBUG_RENDERER is defined
#endif // !JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRendererSimple.h>

#include "../renderer/Shader.hpp"

struct cv3 {
	JPH::Vec3 vec;
	JPH::ColorArg color;
};

class MyDebugRenderer : public JPH::DebugRendererSimple
{
	GLuint lineShaderID;
	GLuint triangleShaderID;
	GLuint textShaderID;

	

	GLuint LVBO, LVAO;

	vector <JPH::Vec3> lineVector;


public:
	MyDebugRenderer(GLuint lineShaderID, GLuint triangleShaderID, GLuint textShaderID)
		: lineShaderID(lineShaderID), triangleShaderID(triangleShaderID), textShaderID(textShaderID)
	{

		lineVector.reserve(10000);
	}

	virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
	{
		/*
		float lineVertices[] = {
			inFrom.GetX(), inFrom.GetY(), inFrom.GetZ(),
			inTo.GetX(),   inTo.GetY(),   inTo.GetZ()
		};
		*/

		


		lineVector.emplace_back(inFrom);
		lineVector.emplace_back(inTo);

		


		
	}

	virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override
	{
		/*
		float vertices[] = {
			inV1.GetX(), inV1.GetY(), inV1.GetZ(), inColor.r, inColor.g, inColor.b,
			inV2.GetX(), inV2.GetY(), inV2.GetZ(), inColor.r, inColor.g, inColor.b,
			inV3.GetX(), inV3.GetY(), inV3.GetZ(), inColor.r, inColor.g, inColor.b
		};
		*/



		GLuint VBO, VAO;
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);

		vector <cv3> vertices;

		vertices.emplace_back(inV1, inColor);
		vertices.emplace_back(inV2, inColor);
		vertices.emplace_back(inV3, inColor);

		// Upload vertex data to VBO
		glNamedBufferData(VBO, vertices.size() * sizeof(cv3), vertices.data(), GL_STATIC_DRAW);

		glEnableVertexArrayAttrib(VAO, 0);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

		// Bind VBO to VAO for vertex data
		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(cv3));

		glUseProgram(triangleShaderID);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);
	}

	virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override
	{
		glUseProgram(textShaderID);
	}

	void drawAllLines() {

		// Create buffers
		glCreateVertexArrays(1, &LVAO);
		glCreateBuffers(1, &LVBO);


		glNamedBufferData(LVBO, lineVector.size() * sizeof(JPH::Vec3), lineVector.data(), GL_STATIC_DRAW);

		glEnableVertexArrayAttrib(LVAO, 0);
		glVertexArrayAttribBinding(LVAO, 0, 0);
		glVertexArrayAttribFormat(LVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

		// Bind VBO to VAO for vertex data
		glVertexArrayVertexBuffer(LVAO, 0, LVBO, 0, sizeof(JPH::Vec3));



		// Draw
		glUseProgram(lineShaderID);

		glBindVertexArray(LVAO);
		glDrawArrays(GL_LINES, 0, lineVector.size());

		glBindVertexArray(0);
		glDeleteBuffers(1, &LVBO);
		glDeleteVertexArrays(1, &LVAO);

		lineVector.clear();


	}
};