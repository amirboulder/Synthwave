#pragma once

#include "core/src/pch.h"

#include "../pipeline.hpp"

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

		createPipelineEnt("pipelineUnlit", "shaders/slang/shaders.slang",
			"shaders/compiled/VertexShader.spv", "shaders/compiled/FragmentShader.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(1, 0, 0, 0), renderContext);

		createPipelineEnt("pipelineGrid", "shaders/slang/gridshader.slang",
			"shaders/compiled/grid.vert.spv", "shaders/compiled/grid.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 0, 0, 0), renderContext);

		createPipelineEnt("pipelineMtn", "shaders/slang/wireframe.slang",
			"shaders/compiled/wireframe.vert.spv", "shaders/compiled/wireframe.frag.spv",
			glm::ivec4(0, 2, 0, 0), glm::ivec4(0, 0, 0, 0), renderContext);

		//TODO move this
		ecs.entity("RenderState")
			.set<RenderState>({ ecs.lookup("pipelineUnlit") });
	}

	static bool validateShaderExistance(const std::string& filePathVS, const std::string& filePathFS) {
		namespace fs = std::filesystem;
		return fs::exists(filePathVS) && fs::exists(filePathFS);
	}

	bool createPipelineEnt(const std::string shaderName, const std::string filePathShaderSrc,
		const std::string& filePathVS, const std::string& filePathFS, glm::ivec4 paramsVS,
		glm::ivec4 paramsFS, const RenderContext& renderContext) {

		flecs::entity pipelineEnt = ecs.entity(shaderName.c_str()).set<Pipeline>({});

		if (!pipelineEnt) {

			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, ERROR "Failed to create entity: %s" RESET, shaderName);
			return false;
		}

		Pipeline& pipelneRef = pipelineEnt.get_mut<Pipeline>();

		if (!validateShaderExistance(filePathVS, filePathFS)) {
			int returnVal = shader::generateSpirvShaders(filePathShaderSrc.c_str(), filePathVS.c_str(), filePathFS.c_str());

			if (returnVal != 0) {
				return false;
			}
		}

		if (!RenderUtil::loadShaderSPRIV(renderContext.device, pipelneRef.vertexShader,
			filePathVS, SDL_GPU_SHADERSTAGE_VERTEX, paramsVS.x, paramsVS.y, paramsVS.z, paramsVS.w)) return false;
		if(!RenderUtil::loadShaderSPRIV(renderContext.device, pipelneRef.fragmentShader,
			filePathFS, SDL_GPU_SHADERSTAGE_FRAGMENT, paramsFS.x, paramsFS.y, paramsFS.z, paramsFS.w)) return false;
		if(!pipelneRef.createPipeline(ecs, shaderName.c_str(), false)) return false;

		return true;
	}

};