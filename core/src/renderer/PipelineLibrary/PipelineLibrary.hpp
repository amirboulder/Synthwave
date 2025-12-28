#pragma once

#include "core/src/pch.h"

#include "../pipeline.hpp"
#include "../computePipeline.hpp"

// Container class for pipelines
//TODO HOT reloading for shaders!!!
class PipelineLibrary {

public:

	flecs::world& ecs;

	PipelineLibrary(flecs::world& ecs)
		:ecs(ecs)
	{

	}

	//TODO don't create a slang session for each shader. Create it once and compile all shaders
	void createPipelineEnts() {

		///////////////creating shaders
		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig renderConfig = ecs.get<RenderConfig>();

		createPipelineEnt("pipelineUnlit", "shaders/slang/shaders.slang",
			"shaders/compiled/unlit.vert.spv", "shaders/compiled/unlit.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(1, 1, 0, 0), renderContext, PipelineType::Vertex,
			renderConfig.sampleCount);

		createPipelineEnt("pipelineGrid", "shaders/slang/gridshader.slang",
			"shaders/compiled/grid.vert.spv", "shaders/compiled/grid.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 1, 0, 0), renderContext, PipelineType::Vertex,
			renderConfig.sampleCount);

		createPipelineEnt("pipelineMtn", "shaders/slang/wireframe.slang",
			"shaders/compiled/wireframe.vert.spv", "shaders/compiled/wireframe.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 1, 0, 0), renderContext, PipelineType::Vertex,
			renderConfig.sampleCount);

		createPipelineEnt("pipelinePhysics", "shaders/slang/physicsRender.slang",
			"shaders/compiled/physicsRender.vert.spv", "shaders/compiled/physicsRender.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 0, 0, 0), renderContext, PipelineType::PhysicsDebug,
			renderConfig.sampleCount);

		createPipelineEnt("pipelineLineMultiSample", "shaders/slang/lineShader.slang",
			"shaders/compiled/lineShader.vert.spv", "shaders/compiled/lineShader.frag.spv",
			glm::ivec4(0, 1, 0, 0), glm::ivec4(0, 0, 0, 0), renderContext, PipelineType::LineVertex,
			renderConfig.sampleCount);

		createPipelineEnt("pipelineLine", "shaders/slang/lineShader.slang",
			"shaders/compiled/lineShader.vert.spv", "shaders/compiled/lineShader.frag.spv",
			glm::ivec4(0, 1, 0, 0), glm::ivec4(0, 0, 0, 0), renderContext, PipelineType::LineVertex,
			SDL_GPU_SAMPLECOUNT_1);

		createPipelineEnt("entIdPipeline", "shaders/slang/EntIDShader.slang",
			"shaders/compiled/entIDShader.vert.spv", "shaders/compiled/entIDShader.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 1, 0, 0), renderContext, PipelineType::EntityId,
			SDL_GPU_SAMPLECOUNT_1);
		
		createComputePipeline("OutlineComputePipeline", "shaders/slang/OutlineShader.slang",
			"shaders/compiled/OutlineShader.comp.spv",
			glm::ivec4(0, 1, 0, 0), renderContext);

		//TODO move this
		ecs.entity("RenderState")
			.set<RenderState>({ ecs.lookup("pipelineUnlit") });
	}

	static bool validateShaderExistence(const std::string& filePathVS, const std::string& filePathFS) {
		namespace fs = std::filesystem;
		return fs::exists(filePathVS) && fs::exists(filePathFS);
	}

	static bool validateShaderExistence(const std::string& filePath) {
		namespace fs = std::filesystem;
		return fs::exists(filePath);
	}

	bool createPipelineEnt(const std::string entityName, const std::string filePathShaderSrc,
		const std::string& filePathVS, const std::string& filePathFS, glm::ivec4 paramsVS,
		glm::ivec4 paramsFS, const RenderContext& renderContext, PipelineType type, SDL_GPUSampleCount sampleCount) {

		flecs::entity pipelineEnt = ecs.entity(entityName.c_str()).set<Pipeline>({});

		if (!pipelineEnt) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Failed to create entity: %s" RESET, entityName);
			return false;
		}

		Pipeline& pipelneRef = pipelineEnt.get_mut<Pipeline>();

		if (!validateShaderExistence(filePathVS, filePathFS)) {
			int returnVal = shader::generateSpirvShaders(filePathShaderSrc.c_str(), filePathVS.c_str(), filePathFS.c_str());

			if (returnVal != 0) {
				return false;
			}
		}

		if (!RenderUtil::loadShaderSPRIV(renderContext.device, pipelneRef.vertexShader,
			filePathVS, SDL_GPU_SHADERSTAGE_VERTEX, paramsVS.x, paramsVS.y, paramsVS.z, paramsVS.w)) return false;
		if (!RenderUtil::loadShaderSPRIV(renderContext.device, pipelneRef.fragmentShader,
			filePathFS, SDL_GPU_SHADERSTAGE_FRAGMENT, paramsFS.x, paramsFS.y, paramsFS.z, paramsFS.w)) return false;

		switch (type)
		{
		case PipelineType::Vertex:
			if (!pipelneRef.createPipeline(ecs, entityName.c_str(), sampleCount, false)) return false;

			break;

		case PipelineType::LineVertex:

			if (!pipelneRef.createLineVertPipeline(ecs, entityName.c_str(), sampleCount)) return false;

			break;

		case PipelineType::PhysicsDebug:

			if (!pipelneRef.createPhysicsDebugPipeline(ecs, entityName.c_str(), sampleCount)) return false;

			break;
		case PipelineType::EntityId:

			if (!pipelneRef.createEntIdPipeline(ecs, entityName.c_str(), sampleCount)) return false;

			break;

		default:
			break;
		}

		pipelneRef.type = type;

		return true;
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

		//TODO fix hardcoded numbers. get them from shader reflection data
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