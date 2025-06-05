#pragma once

#include <unordered_map>

#include "Mesh.hpp"

class Model {

public:
	vector <Mesh> meshes;
	std::vector<aiMatrix4x4> meshTransforms;
	
	static std::unordered_map<std::string, GLuint> DSAtextureCache;

	GLuint shaderID;

	uint32_t transformIndex;


	Model() {}

	//TODO SAVE THE LOADED MODEL SO THAT WE ARE NOT IMPORTING THEM EVERYTIME
	// Basically the model get loaded by assimp, we then copy all that data into our model/mesh class
	
	Model(const char* filePath)
		: shaderID(shaderID)
	{

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
			return;
		}

		cout << "processsing model : " << filePath << '\n';
		//cout << "number of textures " << scene->mNumTextures << '\n';
		//cout << "number of materials " << scene->mNumMaterials << '\n';

		string fileExtention = getFileExtension(filePath);

		cout << "file extention  : " << fileExtention << '\n';

		if (fileExtention == "glb") {

			ProcessGLB(scene);
			return;
		}
		if (fileExtention == "gltf") {

			//TODO handle
			cout << "gltf handling not yet implemented\n";
		}
		else {
			cout << "we don't support " << fileExtention << "files!\n";
		}

	}
	


	// factory constructor
	Model(const char* filePath, GLuint shaderID, uint32_t transformIndex)
		:shaderID(shaderID),
		transformIndex(transformIndex)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << '\n';
			return;
		}
		cout << "number of meshes for " << filePath << " is " << scene->mNumMeshes << '\n';

		
		meshes.reserve(scene->mNumMeshes);
		meshTransforms.resize(scene->mNumMeshes); 

		// Extract all mesh transforms first
		ExtractMeshTransforms(scene->mRootNode, aiMatrix4x4(), scene);

		// Process meshes with their transforms
		for (int i = 0; i < scene->mNumMeshes; i++) {
			meshes.emplace_back(scene->mMeshes[i]);
			aiMesh* importedMesh = scene->mMeshes[i];

			
			meshes.back().localTransform = ConvertMatrix(meshTransforms[i]);

			// Process materials and textures
			aiMaterial* material = scene->mMaterials[importedMesh->mMaterialIndex];
			aiString texturePath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
				string path = getFileFolder(filePath);
				path.append(texturePath.C_Str());
				meshes.back().diffuseTextureID = loadTexturesDSA(path);
				meshes.back().processMesh();
			}
			else {
				meshes.back().diffuseTextureID = loadTexturesDSA("assets/checkerboard.png");
				meshes.back().processMesh();
			}
		}
	}

	// Generated model constructor
	Model(GLuint shaderID, uint32_t transformIndex,int rows, int cols)
		:shaderID(shaderID),
		transformIndex(transformIndex)
	{
		meshes.emplace_back(rows, cols);
		meshes.back().processMesh();
	}


	// Copy constructor
	Model(const Model& other)
		:shaderID(other.shaderID),
		transformIndex(other.transformIndex),
		meshes(other.meshes)
	{
		printf("\033[33mModel copy constructor called!\033[0m\n");
	}

	void processObj(const aiScene* scene, const char* filePath) {

		meshes.reserve(scene->mNumMeshes);
		for (int i = 0; i < scene->mNumMeshes; i++) {
			meshes.emplace_back(scene->mMeshes[i]);

			aiMesh* importedMesh = scene->mMeshes[i];

			aiMaterial* material = scene->mMaterials[importedMesh->mMaterialIndex];

			aiString texturePath;

			
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {

				string path = getFileFolder(filePath);
				path.append(texturePath.C_Str());
				cout << " diffuse texture Path : " << path << '\n';

				meshes.back().diffuseTextureID = loadTexturesDSA(path);

			}

			if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {

				string path = getFileFolder(filePath);
				path.append(texturePath.C_Str());

				cout << "Specular texture Path : " << path << '\n';

				//meshes.back().SpecularTextureID = loadTexturesDSA(path);

			}

			//if no texture load in default checkerboard texture
			if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0) {

				meshes.back().diffuseTextureID = loadTexturesDSA("assets/checkerboard.png");

			}

			meshes.back().processMesh();



		}

	}

	void ProcessGLB(const aiScene* scene) {

		meshes.reserve(scene->mNumMeshes);

		for (int i = 0; i < scene->mNumMeshes; i++) {
			meshes.emplace_back(scene->mMeshes[i]);

			aiMesh* importedMesh = scene->mMeshes[i];

			aiMaterial* material = scene->mMaterials[importedMesh->mMaterialIndex];


				aiString texture_path;
				if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS) {
					if (texture_path.C_Str()[0] == '*') { // Embedded texture reference
						unsigned int texture_index = std::atoi(texture_path.C_Str() + 1);

						meshes.back().diffuseTextureID = loadEmbeddedTextureDSA(scene->mTextures[texture_index]);
					}
					else {
						std::cout << "Material " << i << " uses external texture: " << texture_path.C_Str() << std::endl;
					}
				}
			

			//if no texture load in default checkerboard texture
			// TODO check if this is correct by exporting a capsule as glb from blender
			if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0) {

				cout << "no texture found for " << scene->GetShortFilename << "mesh index : " << i <<
					"loading default checkerboard " << '\n';

				meshes.back().diffuseTextureID = loadTexturesDSA("assets/checkerboard.png");

			}


			meshes.back().processMesh();

		}

	}


	void draw(std::vector<TransformData> & transforms) {

		for (int i = 0; i < meshes.size(); i++) {

		meshes[i].draw(shaderID, transforms[transformIndex].currentMatrix);

		}
	}

	

	GLuint loadTexturesDSA(const std::string& texturePath) {

		// Check if texture is already loaded
		if (DSAtextureCache.find(texturePath) != DSAtextureCache.end()) {
			return DSAtextureCache[texturePath]; // Return existing texture ID
		}

		// Create and load the texture using DSA
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		// Set texture parameters using DSA
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load imageData
		int width, height, nrChannels;
		unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
		if (!data) {
			std::cerr << "loadTexturesDSA Failed to load texture: " << texturePath << std::endl;
			glDeleteTextures(1, &textureID); // Cleanup
			return 0;
		}

		// Determine texture format
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		GLenum internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;

		// Allocate and upload texture data using DSA
		glTextureStorage2D(textureID, 1, internalFormat, width, height);

		glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

		// Generate mipmaps using DSA
		glGenerateTextureMipmap(textureID);

		// Free imageData memory
		stbi_image_free(data);

		// Store texture ID in cache
		DSAtextureCache[texturePath] = textureID;

		//cout << "texture ID : " << DSAtextureID << '\n';
		return textureID;

	}

	GLuint loadEmbeddedTextureDSA(aiTexture* texture) {

		string textureName = std::to_string(reinterpret_cast<uintptr_t>(texture));

		// Check if texture is already loaded
		if (DSAtextureCache.find(textureName) != DSAtextureCache.end()) {
			return DSAtextureCache[textureName]; // Return existing texture ID
		}

		// Create and load the texture using DSA
		GLuint textureID;
		glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

		// Set texture parameters using DSA
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int width, height, nrChannels;
		unsigned char* imageData = stbi_load_from_memory((unsigned char*)texture->pcData, texture->mWidth, &width, &height, &nrChannels, 0);
		if (imageData) {
			std::cout << "loadEmbeddedTextureDSA Loaded texture: " << width << "x" << height << ", channels: " << nrChannels << std::endl;
			
		}
		else {
			std::cerr << "loadEmbeddedTextureDSA Failed to load texture data" << std::endl;
		}

		// Determine texture format
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		GLenum internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;

		// Allocate and upload texture data using DSA
		glTextureStorage2D(textureID, 1, internalFormat, width, height);

		glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, imageData);

		// Generate mipmaps using DSA
		glGenerateTextureMipmap(textureID);

		// Free imageData memory
		stbi_image_free(imageData);

		// Store texture ID in cache
		DSAtextureCache[textureName] = textureID;

		//cout << "texture ID : " << DSAtextureID << '\n';
		return textureID;

	}

	const char* getFileExtension(const char* filePath) {
		const char* dot = strrchr(filePath, '.'); // Find last '.'
		if (!dot || dot == filePath) {
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

	void ExtractMeshTransforms(const aiNode* node, const aiMatrix4x4& parentTransform, const aiScene* scene) {
		// Calculate current node's world transform
		aiMatrix4x4 currentTransform = parentTransform * node->mTransformation;

		// Process all meshes in this node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			unsigned int meshIndex = node->mMeshes[i];
			meshTransforms[meshIndex] = currentTransform;
		}

		// Recursively process child nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ExtractMeshTransforms(node->mChildren[i], currentTransform, scene);
		}
	}

	// Convert aiMatrix4x4 to your matrix type (adjust based on your math library)
	glm::mat4 ConvertMatrix(const aiMatrix4x4& aiMat) {
		return glm::mat4(
			aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
			aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
			aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
			aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
		);
	}


};

std::unordered_map<std::string, GLuint> Model::DSAtextureCache;