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

	void syncShaders() {

		if (!fs::exists(shaderSourceDir)) {
			//We can Create directory but if there is no directory there are no shaders
			//TODO throw fatal error
			return;
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

			createPipelineEnt(path);
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


	void createPipelineEnts() {

		//TODO Remove once done testing
		//SDL_SetLogPriority(LOG_RENDER, SDL_LOG_PRIORITY_TRACE);

		syncShaders();

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


				processGraphicsShaderMetadata(doc, reflectionVS, reflectionFS, pipelineType);

				pipelneRef.createPipeline(ecs, filename, reflectionVS, reflectionFS, pipelineType);

			}
			else if (shaderType == "compute") {

				ComputeShaderReflectionData reflectionCS;

				processComputeShaderMetadata(doc, reflectionCS);


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

		}

		return true;

	}

	bool processGraphicsShaderMetadata(const rapidjson::Document & doc,
		ShaderReflectionData & reflectionVS,
		ShaderReflectionData & reflectionFS,
		PipelineType & pipelineType) {

		if (doc.HasMember("vertex")) {

			if (doc["vertex"].HasMember("numSamplers") && doc["vertex"]["numSamplers"].IsInt()) {
				reflectionVS.numSamplers = doc["vertex"]["numSamplers"].GetInt();
			}
			if (doc["vertex"].HasMember("numStorageBuffers") && doc["vertex"]["numStorageBuffers"].IsInt()) {
				reflectionVS.numStorageBuffers = doc["vertex"]["numStorageBuffers"].GetInt();
			}
			if (doc["vertex"].HasMember("numStorageTextures") && doc["vertex"]["numStorageTextures"].IsInt()) {
				reflectionVS.numStorageTextures = doc["vertex"]["numStorageTextures"].GetInt();
			}
			if (doc["vertex"].HasMember("numUniformBuffers") && doc["vertex"]["numUniformBuffers"].IsInt()) {
				reflectionVS.numUniformBuffers = doc["vertex"]["numUniformBuffers"].GetInt();
			}
			if (doc["vertex"].HasMember("outputFile") && doc["vertex"]["outputFile"].IsString()) {
				reflectionVS.outputFile = doc["vertex"]["outputFile"].GetString();
			}


		}

		if (doc.HasMember("fragment")) {

			if (doc["fragment"].HasMember("numSamplers") && doc["fragment"]["numSamplers"].IsInt()) {
				reflectionFS.numSamplers = doc["fragment"]["numSamplers"].GetInt();
			}
			if (doc["fragment"].HasMember("numStorageBuffers") && doc["fragment"]["numStorageBuffers"].IsInt()) {
				reflectionFS.numStorageBuffers = doc["fragment"]["numStorageBuffers"].GetInt();
			}
			if (doc["fragment"].HasMember("numStorageTextures") && doc["fragment"]["numStorageTextures"].IsInt()) {
				reflectionFS.numStorageTextures = doc["fragment"]["numStorageTextures"].GetInt();
			}
			if (doc["fragment"].HasMember("numUniformBuffers") && doc["fragment"]["numUniformBuffers"].IsInt()) {
				reflectionFS.numUniformBuffers = doc["fragment"]["numUniformBuffers"].GetInt();
			}
			if (doc["fragment"].HasMember("outputFile") && doc["fragment"]["outputFile"].IsString()) {
				reflectionFS.outputFile = doc["fragment"]["outputFile"].GetString();
			}

		}

		if (doc.HasMember("pipelineType") && doc["pipelineType"].IsInt()) {
			int rawValue = doc["pipelineType"].GetInt();
			pipelineType = static_cast<PipelineType>(rawValue);
		}

		if (!fs::exists(reflectionVS.outputFile)) {
			LogError(LOG_RENDER, "output file %s is missing",
				reflectionVS.outputFile.c_str());
			return false;
		}
		if (!fs::exists(reflectionFS.outputFile)) {
			LogError(LOG_RENDER, "output file %s is missing",
				reflectionFS.outputFile.c_str());
			return false;
		}

		return true;

	}

	bool processComputeShaderMetadata(const rapidjson::Document& doc, ComputeShaderReflectionData & reflectionCS) {

		if (doc.HasMember("compute")) {

			if (doc["compute"].HasMember("numSamplers") && doc["compute"]["numSamplers"].IsInt()) {
				reflectionCS.numSamplers = doc["compute"]["numSamplers"].GetInt();
			}
			if (doc["compute"].HasMember("numUniformBuffers") && doc["compute"]["numUniformBuffers"].IsInt()) {
				reflectionCS.numUniformBuffers = doc["compute"]["numUniformBuffers"].GetInt();
			}
			if (doc["compute"].HasMember("numReadOnlyStorageBuffers") && doc["compute"]["numReadOnlyStorageBuffers"].IsInt()) {
				reflectionCS.numReadOnlyStorageBuffers = doc["compute"]["numReadOnlyStorageBuffers"].GetInt();
			}
			if (doc["compute"].HasMember("numReadOnlyStorageTextures") && doc["compute"]["numReadOnlyStorageTextures"].IsInt()) {
				reflectionCS.numReadOnlyStorageTextures = doc["compute"]["numReadOnlyStorageTextures"].GetInt();
			}
			if (doc["compute"].HasMember("numReadwriteStorageTextures") && doc["compute"]["numReadwriteStorageTextures"].IsInt()) {
				reflectionCS.numReadwriteStorageTextures = doc["compute"]["numReadwriteStorageTextures"].GetInt();
			}
			if (doc["compute"].HasMember("numReadwriteStorageBuffers") && doc["compute"]["numReadwriteStorageBuffers"].IsInt()) {
				reflectionCS.numReadwriteStorageBuffers = doc["compute"]["numReadwriteStorageBuffers"].GetInt();
			}
			if (doc["compute"].HasMember("threadCountX") && doc["compute"]["threadCountX"].IsInt()) {
				reflectionCS.threadCountX = doc["compute"]["threadCountX"].GetInt();
			}
			if (doc["compute"].HasMember("threadCountY") && doc["compute"]["threadCountY"].IsInt()) {
				reflectionCS.threadCountY = doc["compute"]["threadCountY"].GetInt();
			}
			if (doc["compute"].HasMember("threadCountZ") && doc["compute"]["threadCountZ"].IsInt()) {
				reflectionCS.threadCountZ = doc["compute"]["threadCountZ"].GetInt();
			}

			if (doc["compute"].HasMember("outputFile") && doc["compute"]["outputFile"].IsString()) {
				reflectionCS.outputFile = doc["compute"]["outputFile"].GetString();
			}

			if (!fs::exists(reflectionCS.outputFile)) {
				LogError(LOG_RENDER, "output file %s is missing",
					reflectionCS.outputFile.c_str());
				return false;
			}
		}

		return true;
	}


	

};