#pragma once

#include "../pipeline.hpp"
#include "../computePipeline.hpp"

//TODO Add HOT reloading for shaders!!!

class PipelineLibrary {

public:

	flecs::world& ecs;

	fs::path shaderSourceDir;
	fs::path shaderCompiledDir;

	std::vector<fs::path> shaderSrcPaths;
	std::vector<fs::path> needsRecompilePaths;
	std::vector<fs::path> shaderMetaDataPaths;


	PipelineLibrary(flecs::world& ecs,
		fs::path sourceDir = "shaders/slang/",
		fs::path compiledDir = "shaders/compiled")
		:ecs(ecs), shaderSourceDir(std::move(sourceDir)), shaderCompiledDir(std::move(compiledDir))
	{

	}

	void init() {

		syncShaders();
	}

	void syncShaders() {

		if (!fs::exists(shaderSourceDir)) {
			LogError(LOG_RENDER, "Shader source directory %s does not exist", shaderSourceDir.c_str());
			/*
			* TODO We can create the shaders folder in the build directory here but
			, in a shipped game we may not have access to the source files.
			Perhaps we can add code to check if source is available.
			*/
			std::exit(EXIT_FAILURE);
		}

		createSrcList();
		createMetadataList();
		buildNeedsCompileList();

		for (const fs::path& path : needsRecompilePaths) {
			cout << path << std::endl;
		}

		ShaderCompiler shaderCompiler;
		shaderCompiler.compileAllShaders(needsRecompilePaths);

		shaderMetaDataPaths.clear();
		createMetadataList();

		for (const fs::path& path : shaderMetaDataPaths) {

			//If unable to create shaders than we can't really run the program
			if (!createPipelineEnt(path)) {
				std::exit(EXIT_FAILURE);
			}
		}
	}

	//Put all the shader source paths found in shaderSourceDir into a vector
	void createSrcList() {

		for (const auto& entry : fs::recursive_directory_iterator(shaderSourceDir)) {
			if (!entry.is_regular_file()) continue;
			if (entry.path().extension() != ".slang") continue;

			shaderSrcPaths.push_back(entry);
		}
	}

	void createMetadataList() {

		if (!fs::exists(shaderCompiledDir)) {
			LogDebug(LOG_RENDER, "Compiled shader directory %s does not exist, creating it");
			fs::create_directory(shaderCompiledDir);
		}

		for (const auto& entry : fs::recursive_directory_iterator(shaderCompiledDir)) {
			if (!entry.is_regular_file()) continue;
			if (entry.path().extension() != ".json") continue;

			shaderMetaDataPaths.push_back(entry);
		}

	}

	/// <summary>
	/// For all the shader source files check if they have a matching metadata file in compiled folder.
	/// Metadata is a json file with the same name as the source and compiled files.
	/// Its created using information from slangs shader reflection api.
	/// If the metadata file is outdated then add the source file to needsRecompilePaths list.
	/// If there is no metadata then add the source to needsRecompilePaths.
	/// </summary>
	void buildNeedsCompileList() {

		for (const fs::path& src : shaderSrcPaths) {

			std::string srcFilename = src.stem().string();

			bool foundMetadata = false;
			bool alreadyAddedToList = false; //makes sure we only add file to list once
			for (const fs::path& metadata : shaderMetaDataPaths) {

				if (srcFilename == metadata.stem().string()) {

					auto sourceFileLastWrite = fs::last_write_time(src);
					auto compiledFileLastTime = fs::last_write_time(metadata);

					if (sourceFileLastWrite < compiledFileLastTime) {
						foundMetadata = true;
						break;
					}
					if (sourceFileLastWrite > compiledFileLastTime) {

						needsRecompilePaths.push_back(src);
						alreadyAddedToList = true;
						break;
					}
				}
			}
			if (!foundMetadata && !alreadyAddedToList) {
				needsRecompilePaths.push_back(src);
			}
		}
	}


	bool createPipelineEnt(const fs::path& metaDataFilePath) {

		std::string filename = metaDataFilePath.stem().string();

		string entityName = "pipeline";
		entityName += filename;

		std::ifstream file(metaDataFilePath);
		if (!file.is_open()) {
			LogError(LOG_RENDER, "Failed to open file %s", metaDataFilePath.string().c_str());
			return false;
		}

		rapidjson::IStreamWrapper isw(file);
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if (doc.HasParseError()) {
			LogError(LOG_RENDER, "JSON parse error: %s at offset %s in file %s",
				doc.GetParseError(), doc.GetErrorOffset(), metaDataFilePath.string().c_str());
			return false;
		}

		if (doc.IsObject()) {
			LogTrace(LOG_RENDER, "Successfully loaded JSON object from file %s", metaDataFilePath.string().c_str());
		}

		if (doc.HasMember("shaderType")) {

			std::string shaderType = doc["shaderType"].GetString();

			if (shaderType != "graphics" && shaderType != "compute") {

				LogError(LOG_RENDER, "shader type %s in file %s is not supported",
					shaderType.c_str(),	metaDataFilePath.string().c_str());
			}

			//TODO maybe put the code in this if statement into a separate function
			if (shaderType == "graphics") {

				flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<Pipeline>({});

				if (!pipelineEnt) {
					LogError(LOG_RENDER, "Failed to create entity: %s", entityName);
					return false;
				}

				Pipeline& pipelneRef = pipelineEnt.get_mut<Pipeline>();

				ShaderReflectionData reflectionVS;
				ShaderReflectionData reflectionFS;

				PipelineType pipelineType = PipelineType::NONE;


				processGraphicsShaderMetadata(filename, doc, reflectionVS, reflectionFS, pipelineType);

				if (!pipelneRef.createPipeline(ecs, filename, reflectionVS, reflectionFS, pipelineType)){
					return false;
				}

			}
			else if (shaderType == "compute") {

				ComputeShaderReflectionData reflectionCS;

				processComputeShaderMetadata(filename, doc, reflectionCS);


				flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<ComputePipeline>({});
				ComputePipeline& pipelineRef = pipelineEnt.get_mut<ComputePipeline>();


				if (!pipelineRef.createPipeline(ecs, entityName.c_str(), reflectionCS.outputFile.c_str(),
					reflectionCS.numSamplers,
					reflectionCS.numReadOnlyStorageTextures,
					reflectionCS.numReadOnlyStorageBuffers,
					reflectionCS.numReadwriteStorageTextures,
					reflectionCS.numReadwriteStorageBuffers,
					reflectionCS.numUniformBuffers,
					reflectionCS.threadCountX,
					reflectionCS.threadCountY,
					reflectionCS.threadCountZ
				)) return false;

			}


		}
		else {

			LogError(LOG_RENDER, "shader metadata file %s is missing shaderType",
				metaDataFilePath.string().c_str());
			return false;
		}

		return true;

	}

	bool processGraphicsShaderMetadata(const std::string filename, const rapidjson::Document & doc,
		ShaderReflectionData & reflectionVS,
		ShaderReflectionData & reflectionFS,
		PipelineType & pipelineType) {

		if (!doc.HasMember("vertex")) {

			LogError(LOG_RENDER, "Shader metadata file for %s is missing vertex section", filename.c_str());
			return false;
		}
		if (!doc.HasMember("fragment")) {

			LogError(LOG_RENDER, "Shader metadata file for %s is missing fragment section", filename.c_str());
			return false;
		}

		const rapidjson::Value& vertexStage = doc["vertex"];
		const rapidjson::Value& fragmentStage = doc["fragment"];


		//Process Vertex Section
		std::string errorMsgs;
		
		readIntField(vertexStage, "numSamplers", reflectionVS.numSamplers, errorMsgs);
		readIntField(vertexStage, "numStorageBuffers", reflectionVS.numStorageBuffers, errorMsgs);
		readIntField(vertexStage, "numStorageTextures", reflectionVS.numStorageTextures, errorMsgs);
		readIntField(vertexStage, "numUniformBuffers", reflectionVS.numUniformBuffers, errorMsgs);
		readStringField(vertexStage, "outputFile", reflectionVS.outputFile, errorMsgs);

		if (errorMsgs.size() > 0) {

			LogError(LOG_RENDER, "The following are missing from %s shader metadata file in vertex section : %s",
				filename.c_str(), errorMsgs.c_str());
			return false;
		}

		//Process fragment Section
		
		errorMsgs.clear();

		readIntField(fragmentStage, "numSamplers", reflectionFS.numSamplers, errorMsgs);
		readIntField(fragmentStage, "numStorageBuffers", reflectionFS.numStorageBuffers, errorMsgs);
		readIntField(fragmentStage, "numStorageTextures", reflectionFS.numStorageTextures, errorMsgs);
		readIntField(fragmentStage, "numUniformBuffers", reflectionFS.numUniformBuffers, errorMsgs);
		readStringField(fragmentStage, "outputFile", reflectionFS.outputFile, errorMsgs);

		if (errorMsgs.size() > 0) {

			LogError(LOG_RENDER, "The following are missing from %s shader metadata file in fragment section : %s", filename.c_str(), errorMsgs.c_str());
			return false;
		}


		//process common info
		if (doc.HasMember("pipelineType") && doc["pipelineType"].IsInt()) {
			int rawValue = doc["pipelineType"].GetInt();
			pipelineType = static_cast<PipelineType>(rawValue);
		}

		if (!fs::exists(reflectionVS.outputFile)) {
			LogError(LOG_RENDER, "compiled shader file %s does not exist",
				reflectionVS.outputFile.c_str());
			return false;
		}
		if (!fs::exists(reflectionFS.outputFile)) {
			LogError(LOG_RENDER, "compiled shader file %s does not exist",
				reflectionFS.outputFile.c_str());
			return false;
		}

		return true;

	}


	bool processComputeShaderMetadata(const std::string filename, const rapidjson::Document& doc, ComputeShaderReflectionData & reflectionCS) {

		if (!doc.HasMember("compute")) {
			LogError(LOG_RENDER, "Shader metadata file %s is missing compute section", filename.c_str());
			return false;
		}
		const rapidjson::Value& computeVal = doc["compute"];

		std::string errorMsgs;

		readIntField(computeVal, "numSamplers", reflectionCS.numSamplers, errorMsgs);
		readIntField(computeVal, "numUniformBuffers", reflectionCS.numUniformBuffers, errorMsgs);

		readIntField(computeVal, "numReadOnlyStorageBuffers", reflectionCS.numReadOnlyStorageBuffers, errorMsgs);
		readIntField(computeVal, "numReadOnlyStorageTextures", reflectionCS.numReadOnlyStorageTextures, errorMsgs);

		readIntField(computeVal, "numReadwriteStorageBuffers", reflectionCS.numReadwriteStorageBuffers, errorMsgs);
		readIntField(computeVal, "numReadwriteStorageTextures", reflectionCS.numReadwriteStorageTextures, errorMsgs);

		readIntField(computeVal, "threadCountX", reflectionCS.threadCountX, errorMsgs);
		readIntField(computeVal, "threadCountY", reflectionCS.threadCountY, errorMsgs);
		readIntField(computeVal, "threadCountZ", reflectionCS.threadCountZ, errorMsgs);

		readStringField(computeVal, "outputFile", reflectionCS.outputFile, errorMsgs);

		if (errorMsgs.size() > 0) {

			LogError(LOG_RENDER, "The following are missing from %s shader metadata file :  %s", filename.c_str(), errorMsgs.c_str());
			return false;
		}

		if (!fs::exists(reflectionCS.outputFile)) {
			LogError(LOG_RENDER, "Compute shader output file %s referenced in %s metadata file does not exist",
				reflectionCS.outputFile.c_str(), filename.c_str());
			return false;
		}

	return true;
	}


	void readIntField(const rapidjson::Value& stage, const char* key, uint32_t& dest, std::string& errors)
	{
		if (stage.HasMember(key) && stage[key].IsInt()) {
			dest = stage[key].GetInt();
		}
		else {
			errors.append(" ").append(key);

		}
	}

	void readStringField(const rapidjson::Value& stage, const char* key, std::string& dest, std::string& errors)
	{
		if (stage.HasMember(key) && stage[key].IsString()) {
			dest = stage[key].GetString();
		}
		else {
			errors.append(" ").append(key);

		}
	}

};