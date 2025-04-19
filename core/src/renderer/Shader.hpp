#pragma once

#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"

#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>

using std::cout;
using std::string;



class Shader {

	bool hasGeometryShader = false;
	bool hasComputeShader = false;


public:
	GLuint m_shaderID;




	Shader(const char * vertexPath, const char * fragmentPath, const char * geometryPath = nullptr) {

		string vertexShaderCode = readFile(vertexPath);
		string fragmentShaderCode = readFile(fragmentPath);

		

		

		GLuint vertexShader =  createVertexShader(vertexShaderCode.c_str(), vertexPath);
		GLuint fragmentShader = createFragmentShader(fragmentShaderCode.c_str(),fragmentPath);

		if (geometryPath != nullptr) {
			string geometryShaderCode = readFile(geometryPath);
			GLuint geometryShader = createGeometryShader(geometryShaderCode.c_str(), geometryPath);

			hasGeometryShader = true;

			createShaderProgram(vertexShader, fragmentShader, geometryShader);
		}
		else {
			createShaderProgram(vertexShader, fragmentShader);
		}

		
	}

	

	GLuint createVertexShader(const char * vertexShaderSource, const char * name) {


		//GLuinit is unsigned int
		GLuint vertexShader;
		vertexShader = glCreateShader(GL_VERTEX_SHADER);


		//attach shader source to shader & compile
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);

		//checkSuccess(vertexShader, GL_COMPILE_STATUS);
		checkCompileErrors(vertexShader, "VERTEX", name);

		return vertexShader;
	}

	GLuint createGeometryShader(const char * geometryShaderSource, const char * name) {


		//Fragment shader
		GLuint GeometryShader;
		GeometryShader = glCreateShader(GL_GEOMETRY_SHADER);

		glShaderSource(GeometryShader, 1, &geometryShaderSource, NULL);
		glCompileShader(GeometryShader);

		//checkSuccess(fragmentShader, GL_COMPILE_STATUS);
		checkCompileErrors(GeometryShader, "Geometry Shader", name);

		return GeometryShader;
	}

	GLuint createFragmentShader(const char* fragmentShaderSource,const char * name) {


		//Fragment shader
		GLuint fragmentShader;
		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);

		//checkSuccess(fragmentShader, GL_COMPILE_STATUS);
		checkCompileErrors(fragmentShader, "Fragment Shader", name);

		return fragmentShader;
	}


	void createShaderProgram(GLuint vertexShader, GLuint fragmentShader, GLuint GeometryShader = 0) {

		m_shaderID = glCreateProgram();
	
		//attach and link the shaders
		glAttachShader(m_shaderID, vertexShader);
		glAttachShader(m_shaderID, fragmentShader);

		if (hasGeometryShader)
		{
			glAttachShader(m_shaderID, GeometryShader);
		}

		glLinkProgram(m_shaderID);

		checkCompileErrors(m_shaderID, "PROGRAM");


		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteShader(GeometryShader);

	}

	static void use(GLuint shaderID) {
		glUseProgram(shaderID);
	}

	void use() {
		glUseProgram(m_shaderID);
	}

	void remove() {
		glDeleteProgram(m_shaderID);
	}

	void setBool(const std::string & name , bool value) {
		glUniform1i(glGetUniformLocation(m_shaderID, name.c_str()),(int)value);
	}

	void setInt(const std::string& name, int value) {
		glUniform1i(glGetUniformLocation(m_shaderID, name.c_str()), value);
	}

	static void setInt(const char* name, int value,GLuint shaderID) {
		glUniform1i(glGetUniformLocation(shaderID, name), value);
	}

	static void setUint64(const char* name, GLuint64 value, GLuint shaderID) {
		GLint location = glGetUniformLocation(shaderID, name);
		if (location != -1) {
			glUniformHandleui64ARB(location, value); 
		}
		else {
			std::cerr << "Warning: Uniform '" << name << "' not found in shader!" << std::endl;
		}
	}

	void setFloat(const std::string& name, float value) {
		glUniform1f(glGetUniformLocation(m_shaderID, name.c_str()), value);
	}

	void setVec3(const char* name, const glm::vec3 vec) const {
		unsigned int location = glGetUniformLocation(m_shaderID, name);
		glUniform3f(location, vec.x, vec.y, vec.z);
	}

	void setMat4(const char * name, const glm::mat4& mat) 
	{

		glUniformMatrix4fv(glGetUniformLocation(m_shaderID, name), 1, GL_FALSE, &mat[0][0]);
	}

	static void setMat4(const char* name, const glm::mat4& mat, GLuint shaderID)
	{

		glUniformMatrix4fv(glGetUniformLocation(shaderID, name), 1, GL_FALSE, &mat[0][0]);
	}


	string readFile(const char* path) {

		string shaderCode;
		std::ifstream shaderFile;

		shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			//open file 
			shaderFile.open(path);
			std::stringstream shaderStream;

			//read files contents into buffer
			shaderStream << shaderFile.rdbuf();

			//close file
			shaderFile.close();

			//convert to string
			shaderCode = shaderStream.str();

		}
		catch (std::ifstream::failure& e) {

			cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << '\n';
		}

		return shaderCode;
	}


	void checkCompileErrors(unsigned int shader, std::string type, const char * name = NULL)
	{
		int success;
		char infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				std::cout << name << '\n';
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- \n" ;
				
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}

	void checkSuccess(GLuint shader, int status) {

		int success;
		char infoLog[1024];
		
		if (status == GL_COMPILE_STATUS) {
			glGetShaderiv(shader, status, &success);

			if (!success) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: "
					<< "\n" << infoLog << "\n -- " << std::endl;
			}
			return;
		}
		else {
			glGetProgramiv(shader, status, &success);

			if (!success) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << "\n" << infoLog << "\n -- " << std::endl;

			}

		}

	}

};