#pragma once

#include "../ShaderCompilation/ShaderReflection.hpp"
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

		LogSuccess(LOG_RENDER, "PipelineLibrary Initialized");
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
			LogDebug(LOG_RENDER, "%s will be recompiled", path.string().c_str());
		}

		ShaderCompiler shaderCompiler;
		shaderCompiler.compileAllShaders(needsRecompilePaths);

		shaderMetaDataPaths.clear();
		createMetadataList();

		for (const fs::path& path : shaderMetaDataPaths) {

			//If unable to create shaders than we can't really run the program
			if (!createPipelineEnt(path)) {
				LogError(LOG_RENDER, "Pipeline entity creation failed  for %s ", path.c_str());
			}
		}

		LogSuccess(LOG_RENDER, "PipelineLibrary shaders synched");
	}

	//Put all the shader source paths found in shaderSourceDir into a vector
	void createSrcList() {

		for (const auto& entry : fs::directory_iterator(shaderSourceDir)) {
			if (!entry.is_regular_file()) continue;

			auto& p = entry.path();
			if (p.extension() != ".slang") continue;

			shaderSrcPaths.push_back(entry);
		}

		LogVerbose(LOG_RENDER, "PipelineLibrary created Src List");
	}

	void createMetadataList() {

		if (!fs::exists(shaderCompiledDir)) {
			LogDebug(LOG_RENDER, "PipelineLibraryCompiled shader directory %s does not exist, creating it", shaderCompiledDir.string().c_str());
			fs::create_directory(shaderCompiledDir);
		}

		for (const auto& entry : fs::directory_iterator(shaderCompiledDir)) {
			if (!entry.is_regular_file()) continue;
			if (entry.path().extension() != ".json") continue;

			shaderMetaDataPaths.push_back(entry);
		}

		LogVerbose(LOG_RENDER, "PipelineLibrary created Metadata List");
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

		LogVerbose(LOG_RENDER, "PipelineLibrary created Needs Compile List");
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


		ShaderReflectionData reflectionData;

		ShaderReflectionMapper::mapShaderReflection(metaDataFilePath, reflectionData);

		switch (reflectionData.type) {
		case ShaderType::Graphics:
			
		{
			flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<Pipeline>({});
			if (!pipelineEnt) {
				LogError(LOG_RENDER, "Failed to create entity: %s", entityName);
				return false;
			}

			Pipeline& pipelineRef = pipelineEnt.get_mut<Pipeline>();

			if (!pipelineRef.createPipeline(ecs, filename, reflectionData)) {
				return false;
			}

			break;
		}


		case ShaderType::Compute:

		{
			flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<ComputePipeline>({});
			if (!pipelineEnt) {
				LogError(LOG_RENDER, "Failed to create entity: %s", entityName);
				return false;
			}

			ComputePipeline& pipelineRef = pipelineEnt.get_mut<ComputePipeline>();

			ComputeShaderReflectionData & reflectionCS =  reflectionData.reflectionCS;
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

			break;
		}

		case ShaderType::Unknown:
			LogError(LOG_RENDER, "Could not determine shader type for: %s unable to create Pipeline Entity", filename.c_str());
			return false;
		}

		LogVerbose(LOG_RENDER, "PipelineLibrary pipeline entity created %s", entityName.c_str());

		return true;
	}

};