#pragma once


#include "Mesh.hpp"
#include <unordered_map>



class Model {

public:
	vector <Mesh> meshes;
	glm::mat4 modelMatrix;

	static std::unordered_map<std::string, GLuint64> textureCache;
	static std::unordered_map<std::string, GLuint64> textureIDCache;

	GLuint shaderID;

	//TODO SAVE THE LOADED MODEL SO THAT WE ARE NOT IMPORTING THEM EVERYTIME
	// Basically the model get loaded by assimp, we then copy all that data into our model/mesh class
	Model(const char* filePath, glm::mat4 modelMatrix, GLuint shaderID)
		: shaderID(shaderID)
	{

		this->modelMatrix = modelMatrix;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
			return;
		}

		//cout << "number of materials " << scene->mNumMaterials << '\n';

		meshes.reserve(scene->mNumMeshes);
		for (int i = 0; i < scene->mNumMeshes; i++) {
			meshes.emplace_back(scene->mMeshes[i], modelMatrix);

			aiMesh* importedMesh = scene->mMeshes[i];

			aiMaterial* material = scene->mMaterials[importedMesh->mMaterialIndex];

			aiString texturePath;

			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {

				string path = getFileFolder(filePath);
				path.append(texturePath.C_Str());
				
				
				meshes.back().textureHandlers.emplace_back(LoadBindlessTexture(path));
				meshes.back().processMesh();
			}
			//if no texture load in default checkerboard texture
			else {
				meshes.back().textureHandlers.emplace_back(LoadBindlessTexture("assets/checkerboard.png"));
				meshes.back().processMesh();
			}

		}

	}

	void draw() {


		for (int i = 0; i < meshes.size(); i++) {

			meshes[i].meshMatrix = modelMatrix;
			
				meshes[i].draw(shaderID);
			
		}
	}

	/*
	GLuint LoadTexture(const std::string& texturePath) {
		// Check if the texture is already loaded
		if (textureCache.find(texturePath) != textureCache.end()) {
			return textureCache[texturePath]; // Return existing texture ID
		}

		// Load the texture from file
		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		int width, height, nrChannels;
		unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else {
			std::cerr << "Failed to load texture: " << texturePath << std::endl;
			return 0;
		}
		stbi_image_free(data);

		// Store the texture in cache
		textureCache[texturePath] = textureID;
		return textureID;
	}
	*/

	
	GLuint64 LoadBindlessTexture(const std::string& texturePath) {
		// Check if texture is already loaded
		if (textureCache.find(texturePath) != textureCache.end()) {
			return textureCache[texturePath]; // Return existing handle
		}

		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



		// Load image
		int width, height, nrChannels;
		unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
		if (!data) {
			std::cerr << "Failed to load texture: " << texturePath << std::endl;
			glDeleteTextures(1, &textureID); // Cleanup
			return 0;
		}

		// Determine texture format
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free image memory
		stbi_image_free(data);

		// Get Bindless Texture Handle
		GLuint64 textureHandle = glGetTextureHandleARB(textureID);
		glMakeTextureHandleResidentARB(textureHandle);

		// Store texture handle in cache
		textureCache[texturePath] = textureHandle;
		textureIDCache[texturePath] = textureID;  // Store texture ID for cleanup

		std::cout << "Loaded Bindless Texture: " << texturePath
			<< " Handle: " << textureHandle << std::endl;


		return textureHandle;
	}




	const char* getFileExtension(const char* imagePath) {
		const char* dot = strrchr(imagePath, '.'); // Find last '.'
		if (!dot || dot == imagePath) {
			return NULL; // No extension found
		}
		return dot + 1; // Skip the dot and return the extension
	}

	const char* getFileFolder(const char* imagePath) {
		if (!imagePath) {
			return nullptr; // Handle null input
		}

		const char* lastSlash = strrchr(imagePath, '/'); // Find the last '/'

		if (!lastSlash) {
			return nullptr; // No folder found (no '/')
		}

		// Calculate the length of the folder path
		size_t folderLength = lastSlash + 1 - imagePath;

		// Allocate memory for the folder path (including null terminator)
		char* folderPath = new char[folderLength + 1];

		// Copy the folder path into the allocated memory
		strncpy(folderPath, imagePath, folderLength);
		folderPath[folderLength] = '\0'; // Null-terminate the string

		return folderPath; // Return the dynamically allocated folder path
	}

};

std::unordered_map<std::string, GLuint64> Model::textureCache;
std::unordered_map<std::string, GLuint64> Model::textureIDCache;