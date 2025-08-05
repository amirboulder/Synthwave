#pragma once

namespace PL {

	// Helper function to load binary file data
	std::vector<Uint8> loadBinaryFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << filename << std::endl;
			return {};
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<Uint8> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
			std::cerr << "Failed to read file: " << filename << std::endl;
			return {};
		}

		return buffer;
	}



	bool loadVertexShader(
		SDL_GPUDevice* device,
		SDL_GPUShader*& vertexShader,
		const std::string& filename,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{
		// Load the SPIR-V bytecode from file
		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
		if (spirvCode.empty()) {
			std::cerr << "Failed to load vertex shader: " << filename << std::endl;
			return false;
		}

		SDL_GPUShaderCreateInfo createinfo = {};
		createinfo.num_samplers = sampler_count;
		createinfo.num_storage_buffers = storage_buffer_count;
		createinfo.num_storage_textures = storage_texture_count;
		createinfo.num_uniform_buffers = uniform_buffer_count;
		createinfo.props = 0;
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = spirvCode.data();
		createinfo.code_size = spirvCode.size();
		createinfo.entrypoint = "main";
		createinfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

		vertexShader = SDL_CreateGPUShader(device, &createinfo);

		if (vertexShader == NULL) {
			SDL_Log("Failed to create vertex shader!");
			return false;
		}
	}

	bool loadFragmentShader(
		SDL_GPUDevice* device,
		SDL_GPUShader*& fragmentShader,
		const std::string& filename,
		Uint32 sampler_count,
		Uint32 uniform_buffer_count,
		Uint32 storage_buffer_count,
		Uint32 storage_texture_count)
	{

		std::vector<Uint8> spirvCode = loadBinaryFile(filename);
		if (spirvCode.empty()) {
			std::cerr << "Failed to load fragment shader: " << filename << std::endl;
			return false;
		}

		SDL_GPUShaderCreateInfo createinfo = {};
		createinfo.num_samplers = sampler_count;
		createinfo.num_storage_buffers = storage_buffer_count;
		createinfo.num_storage_textures = storage_texture_count;
		createinfo.num_uniform_buffers = uniform_buffer_count;
		createinfo.props = 0;
		createinfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
		createinfo.code = spirvCode.data();
		createinfo.code_size = spirvCode.size();
		createinfo.entrypoint = "main";
		createinfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

		fragmentShader = SDL_CreateGPUShader(device, &createinfo);

		if (fragmentShader == NULL) {
			SDL_Log("Failed to create fragment shader!");
			return false;
		}
	}
}


struct PerModelUniforms {
	glm::mat4 model;
	glm::mat4 mvp;
};


class Pipeline {

public:

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;

	vector <ModelInstance> & models;
	vector<Transform> & transforms;

	int drawType = 0;

	Pipeline(vector <ModelInstance>& models, vector<Transform>& transforms)
		: models(models),
		transforms(transforms)
	{

	}


	void draw(SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* commandBuffer, glm::mat4 viewProj) {

		switch (drawType) {
		case 0:
			drawIndexed(renderPass, commandBuffer, viewProj);
			break;
		case 1:
			drawVerts(renderPass, commandBuffer, viewProj);
			break;
		}

	}

	

	bool createPipeline(SDL_Window* window,SDL_GPUDevice* device, SDL_GPUSampleCount sampleCountMSAA, bool line = false)
	{

		// Vertex input state
		SDL_GPUVertexAttribute vertexAttributes[4] = {};

		// Position attribute
		vertexAttributes[0].location = 0;
		vertexAttributes[0].buffer_slot = 0;
		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[0].offset = 0;

		// normal attribute
		vertexAttributes[1].location = 1;
		vertexAttributes[1].buffer_slot = 0;
		vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[1].offset = sizeof(glm::vec3);

		// texture Coordinates attribute
		vertexAttributes[2].location = 2;
		vertexAttributes[2].buffer_slot = 0;
		vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		vertexAttributes[2].offset = sizeof(glm::vec3) + sizeof(glm::vec3);

		// Color attribute
		vertexAttributes[3].location = 3;
		vertexAttributes[3].buffer_slot = 0;
		vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		vertexAttributes[3].offset = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2);

		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
		vertexBufferDescription.slot = 0;
		vertexBufferDescription.pitch = sizeof(VertexData);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 4;



		// Create pipeline
		SDL_GPUColorTargetDescription coldescs = {};
		coldescs.format = SDL_GetGPUSwapchainTextureFormat(device, window);

		SDL_GPUGraphicsPipelineCreateInfo pipeInfo = {};
		SDL_zero(pipeInfo);
		pipeInfo.vertex_shader = vertexShader;
		pipeInfo.fragment_shader = fragmentShader;
		pipeInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

		pipeInfo.target_info.color_target_descriptions = &coldescs;
		pipeInfo.target_info.num_color_targets = 1;
		pipeInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
		pipeInfo.target_info.has_depth_stencil_target = true;

		pipeInfo.depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_LESS,
		.enable_depth_test = true,
		.enable_depth_write = true,
		};

		//MSAA
		pipeInfo.multisample_state = {
			.sample_count = sampleCountMSAA,  // Enable MSAA in pipeline
			.sample_mask = 0  // Use all samples
		};

		pipeInfo.vertex_input_state = vertexInputState;

		pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
		if (!pipeline) {
			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
			return false;
		}

		if (line) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
			pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
			if (!pipeline) {
				SDL_Log("Failed to create line pipeline: %s", SDL_GetError());
				return false;
			}

		}

		SDL_Log("Built pipeline");

		return true;
	}


	void drawVerts(SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* commandBuffer,glm::mat4 viewProj) {

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

		for (int i = 0; i < models.size(); i++) {


			ModelInstance& model = models[i];
			Transform& transform = transforms[i];

			glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
			glm::mat4 modelRototaion = glm::toMat4(transform.rotation);
			glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
			glm::mat4 modelMat = modelTranslation * modelRototaion * modelScale;


			for (int j = 0; j < model.meshes.size(); j++) {

				MeshInstance& mesh = model.meshes[j];

				SDL_GPUBufferBinding vertexBufferBinding = {};
				vertexBufferBinding.buffer = mesh.vertexBuffer;
				vertexBufferBinding.offset = 0;


				SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

				glm::mat4 meshTrans = glm::translate(glm::mat4(1.0f), mesh.transform.position);
				glm::mat4 meshRot = glm::toMat4(mesh.transform.rotation);
				glm::mat4 meshScale = glm::scale(glm::mat4(1.0f), mesh.transform.scale);
				glm::mat4 meshMat = meshTrans * meshRot * meshScale;

				meshMat = modelMat * meshMat;

				glm::mat4 mvp = viewProj * meshMat;

				mvp = glm::transpose(mvp);

				PerModelUniforms modelUnifroms;
				modelUnifroms.mvp = mvp;
				modelUnifroms.model = meshMat;

				SDL_PushGPUVertexUniformData(commandBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));

				SDL_DrawGPUPrimitives(renderPass, mesh.size, 1, 0, 0);
			}


		}

	}

	void drawIndexed(SDL_GPURenderPass* renderPass, SDL_GPUCommandBuffer* commandBuffer, glm::mat4 viewProj) {

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

		for (int i = 0; i < models.size(); i++) {


			ModelInstance& model = models[i];
			Transform& transform = transforms[i];

			glm::mat4 modelTranslation = glm::translate(glm::mat4(1.0f), transform.position);
			glm::mat4 modelRototaion = glm::toMat4(transform.rotation);
			glm::mat4 modelScale = glm::scale(glm::mat4(1.0f), transform.scale);
			glm::mat4 modelMat = modelTranslation * modelRototaion * modelScale;


			for (int j = 0; j < model.meshes.size(); j++) {

				MeshInstance& mesh = model.meshes[j];

				SDL_GPUBufferBinding vertexBufferBinding = {};
				vertexBufferBinding.buffer = mesh.vertexBuffer;
				vertexBufferBinding.offset = 0;

				SDL_GPUBufferBinding indexBufferBinding = {};
				indexBufferBinding.buffer = mesh.indexBuffer;
				indexBufferBinding.offset = 0;

				SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
				SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);


				glm::mat4 meshTrans = glm::translate(glm::mat4(1.0f), mesh.transform.position);
				glm::mat4 meshRot = glm::toMat4(mesh.transform.rotation);
				glm::mat4 meshScale = glm::scale(glm::mat4(1.0f), mesh.transform.scale);
				glm::mat4 meshMat = meshTrans * meshRot * meshScale;

				meshMat = modelMat * meshMat;

				glm::mat4 mvp = viewProj * meshMat;

				mvp = glm::transpose(mvp);

				PerModelUniforms modelUnifroms;
				modelUnifroms.mvp = mvp;
				modelUnifroms.model = meshMat;

				SDL_PushGPUVertexUniformData(commandBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));

				SDL_DrawGPUIndexedPrimitives(renderPass, mesh.size, 1, 0, 0, 0);
			}


		}

	}
	


};
