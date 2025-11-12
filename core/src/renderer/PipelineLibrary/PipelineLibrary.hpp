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

	void createPipelineEnts() {

		///////////////creating shaders
		RenderContext& renderContext = ecs.get_mut<RenderContext>();

		flecs::entity entUnlitPipeline = ecs.entity("pipelineUnlit").set<Pipeline>({});
		Pipeline& unlit = entUnlitPipeline.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/shaders.slang", "shaders/compiled/VertexShader.spv", "shaders/compiled/FragmentShader.spv");
		RenderUtil::loadShaderSPRIV(renderContext.device, unlit.vertexShader, "shaders/compiled/VertexShader.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderContext.device, unlit.fragmentShader, "shaders/compiled/FragmentShader.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0, 0, 0);
		unlit.createPipeline(ecs, "Blinn-Phong", false);

		flecs::entity entGridPipeline = ecs.entity("pipelineGrid").set<Pipeline>({});
		Pipeline& gridPipeline = entGridPipeline.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/gridshader.slang", "shaders/compiled/grid.vert.spv", "shaders/compiled/grid.frag.spv");
		RenderUtil::loadShaderSPRIV(renderContext.device, gridPipeline.vertexShader, "shaders/compiled/grid.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderContext.device, gridPipeline.fragmentShader, "shaders/compiled/grid.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);
		gridPipeline.createPipeline(ecs, "Grid", false);

		flecs::entity e_Mtn = ecs.entity("pipelineMtn").set<Pipeline>({});
		Pipeline& mtnPipeline = e_Mtn.get_mut<Pipeline>();
		//shader::generateSpirvShaders("shaders/slang/wireframe.slang", "shaders/compiled/wireframe.vert.spv", "shaders/compiled/wireframe.frag.spv");
		RenderUtil::loadShaderSPRIV(renderContext.device, mtnPipeline.vertexShader, "shaders/compiled/wireframe.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(renderContext.device, mtnPipeline.fragmentShader, "shaders/compiled/wireframe.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);
		mtnPipeline.createPipeline(ecs, "Mtn", false);

		//TODO move this 
		ecs.entity("RenderState")
			.set<RenderState>({ entUnlitPipeline });
	}

};