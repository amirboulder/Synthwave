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
		SDL_SetLogPriority(LOG_RENDER, SDL_LOG_PRIORITY_TRACE);


		syncShaders();

		///////////////creating shaders
		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig renderConfig = ecs.get<RenderConfig>();

		createComputePipeline("PipelineOutlineCompute", "shaders/slang/OutlineCompute.slang",
			"shaders/compiled/OutlineCompute.comp.spv",
			glm::ivec4(0, 1, 0, 0), renderContext);

	}

	bool createPipelineEnt(const fs::path& metaDataFilePath) {

		std::string filename = metaDataFilePath.stem().string();

		string entityName = "pipeline";
		entityName += filename;

		flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<Pipeline>({});

		if (!pipelineEnt) {

			LogError(LOG_RENDER, "Failed to create entity: %s", entityName);
			return false;
		}

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

			//TODO maybe put the code in this if statemet into a seprate function
			if (shaderType == "graphics") {

				ShaderReflectionData reflectionVS;
				ShaderReflectionData reflectionFS;

				PipelineType pipelineType = PipelineType::NONE;


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

				Pipeline& pipelneRef = pipelineEnt.get_mut<Pipeline>();

				if (!fs::exists(reflectionVS.outputFile)) {
					LogError(LOG_RENDER, "output file %s mentioned in %s  is missing",
						reflectionVS.outputFile.c_str(), metaDataFilePath.string().c_str());
					return false;
				}
				if (!fs::exists(reflectionFS.outputFile)) {
					LogError(LOG_RENDER, "output file %s mentioned in %s  is missing",
						reflectionFS.outputFile.c_str(), metaDataFilePath.string().c_str());
					return false;
				}

				pipelneRef.createPipeline(ecs, reflectionVS, reflectionFS, pipelineType);

			}
			else if (doc["shaderType"].GetString() == "compute") {

				//TODO compute shader
			}


		}
		else {

			LogError(LOG_RENDER, "shader metadata file %s is missing shaderType",
				metaDataFilePath.string().c_str());

		}

		return true;

	}


	//TODO REMOVE
	static bool validateShaderExistence(const std::string& filePathVS, const std::string& filePathFS) {
		namespace fs = std::filesystem;
		return fs::exists(filePathVS) && fs::exists(filePathFS);
	}

	//TODO REMOVE
	static bool validateShaderExistence(const std::string& filePath) {
		namespace fs = std::filesystem;
		return fs::exists(filePath);
	}


	bool createComputePipeline(const std::string entityName, const std::string filePathShaderSrc,
		const std::string& filePathCS,
		glm::ivec4 paramsCS, const RenderContext& renderContext) {

		flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<ComputePipeline>({});

		if (!pipelineEnt) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Failed to create entity: %s" RESET, entityName.c_str());
			return false;
		}

		ComputePipeline& pipelineRef = pipelineEnt.get_mut<ComputePipeline>();

		if (!validateShaderExistence(filePathCS)) {
			int returnVal = shader::generateSpirvComputeShaders(filePathShaderSrc.c_str(), filePathCS.c_str());

			if (returnVal != 0) {
				return false;
			}
		}

		//TODO fix hardCoded numbers. get them from shader reflection data
		uint32_t samplers = 1;
		uint32_t readonlyStorageTextures = 0;
		uint32_t readonlyStorageBuffers = 0;
		uint32_t readwriteStorageTextures = 1;
		uint32_t readwriteStorageBuffers = 0;
		uint32_t uniformBuffers = 1;
		uint32_t threadcountX = 8;
		uint32_t threadcountY = 8;
		uint32_t threadcountZ = 1;

		if (!pipelineRef.createPipeline(ecs, entityName.c_str(), filePathCS.c_str(),
			samplers,
			readonlyStorageTextures,
			readonlyStorageBuffers,
			readwriteStorageTextures,
			readwriteStorageBuffers,
			uniformBuffers,
			threadcountX,
			threadcountY,
			threadcountZ
		)) return false;

		
		return true;

	}

};