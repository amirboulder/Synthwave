#pragma once

#include <Jolt/Jolt.h>

//#include "../renderer/renderer.hpp"

#ifdef JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRenderer.h>

JPH_NAMESPACE_BEGIN

using namespace JPH;

struct FrameDataUniforms2 {
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection;
};


struct PerModelUniforms2 {
	glm::mat4 model;
	glm::mat4 mvp;
};


class MyDebugRenderer : public JPH::DebugRenderer
{

	/// Last provided camera position
	RVec3						mCameraPos;
	bool						mCameraPosSet = false;

	

	/// Implementation specific batch object
	class BatchImpl : public RefTargetVirtual
	{
	public:
		JPH_OVERRIDE_NEW_DELETE

		virtual void			AddRef() override { ++mRefCount; }
		virtual void			Release() override { if (--mRefCount == 0) delete this; }

		Array<Triangle>			mTriangles;

	private:
		atomic<uint32>			mRefCount = 0;
	};
	
public:


	SDL_Window* window = NULL;
	SDL_GPUDevice* device = NULL;

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;


	SDL_GPUBuffer* vertexBuffer = NULL;

	SDL_GPUTexture* depthTexture = NULL;

	SDL_GPUSampleCount sampleCountMSAA = SDL_GPU_SAMPLECOUNT_8;
	SDL_GPUTexture* msaaColorTarget;
	SDL_GPUTexture* resolveTarget;


	SDL_GPUCommandBuffer* commandBuffer = NULL;
	SDL_GPURenderPass* renderPass = NULL;
	SDL_GPUTexture* swapchainTexture;
	Uint32 swapchainWidth, swapchainHeight;

	glm::mat4 view;
	glm::mat4 proj;

	MyDebugRenderer(SDL_GPUDevice* device, SDL_Window* window, SDL_GPUTexture* resolveTarget, SDL_GPUTexture* msaaColorTarget, SDL_GPUTexture* depthTexture)
		: device(device),window(window),
		resolveTarget(resolveTarget),
		msaaColorTarget(msaaColorTarget),
		depthTexture(depthTexture)
		
	{
		//required by jolt
		Initialize();
	}

	void setCameraUnifroms(RVec3Arg inCameraPos, glm::mat4 view, glm::mat4 proj)
	{
		mCameraPos = inCameraPos;
		mCameraPosSet = true;

		this->view = view;
		this->proj = proj;

	}

	virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
	{
		// NO
	}

	virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override
	{
		// NO
	}

	virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override
	{
		// maybe?
	}

	virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override {

		BatchImpl* batch = new BatchImpl;
		if (inTriangles == nullptr || inTriangleCount == 0)
			return batch;

		batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
		return batch;

	};

	virtual Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount) override {

		BatchImpl* batch = new BatchImpl;
		if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
			return batch;

		// Convert indexed triangle list to triangle list
		batch->mTriangles.resize(inIndexCount / 3);
		for (size_t t = 0; t < batch->mTriangles.size(); ++t)
		{
			Triangle& triangle = batch->mTriangles[t];
			triangle.mV[0] = inVertices[inIndices[t * 3 + 0]];
			triangle.mV[1] = inVertices[inIndices[t * 3 + 1]];
			triangle.mV[2] = inVertices[inIndices[t * 3 + 2]];
		}


		return batch;
	}



	virtual void DrawGeometry(RMat44Arg inModelMatrix,
		const AABox& inWorldSpaceBounds,
		float inLODScaleSq,
		ColorArg inModelColor,
		const GeometryRef& inGeometry,
		ECullMode inCullMode = ECullMode::Off,
		ECastShadow inCastShadow = ECastShadow::Off,
		EDrawMode inDrawMode = EDrawMode::Solid) override
	{



		// Select LOD
		const LOD* lod = inGeometry->mLODs.data();
		if (mCameraPosSet)
			lod = &inGeometry->GetLOD(Vec3(mCameraPos), inWorldSpaceBounds, inLODScaleSq);
		if (!lod)
			return;

		const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());
		if (!batch || batch->mTriangles.empty())
			return;
		
		
		if (batch->mTriangles.empty()) {
			std::cerr << "No triangles to render" << std::endl;
			return;
		}

		//////////////////////////////////
		//Loading in the data from the physics system and sending it to the gpu
		// slow because we do this everyframe
		std::vector<glm::vec3> vertices;

		vertices.reserve(batch->mTriangles.size() * 3);
		for (const auto& triangle : batch->mTriangles) {
			for (int j = 0; j < 3; j++) {
				vertices.emplace_back(
					triangle.mV[j].mPosition.x,
					triangle.mV[j].mPosition.y,
					triangle.mV[j].mPosition.z
				);
			}
		}

		//create vertex buffer
		SDL_GPUBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bufferCreateInfo.size = vertices.size() * sizeof(glm::vec3);

		vertexBuffer = SDL_CreateGPUBuffer(device, &bufferCreateInfo);
		if (!vertexBuffer) {
			std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
			return;
		}

		// Upload vertex data
		SDL_GPUTransferBufferCreateInfo transferBufferCreateInfo = {};
		transferBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferBufferCreateInfo.size = bufferCreateInfo.size;


		SDL_GPUTransferBuffer* vertexTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transferBufferCreateInfo);
		if (!vertexTransferBuffer) {
			std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
			return;
		}

		// Map and copy data
		void* mappedData = SDL_MapGPUTransferBuffer(device, vertexTransferBuffer, false);
		if (!mappedData) {
			std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
			SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);
			return;
		}

		memcpy(mappedData, vertices.data(), vertices.size() * sizeof(glm::vec3));
		SDL_UnmapGPUTransferBuffer(device, vertexTransferBuffer);

		

		// Copy from transfer buffer to vertex buffer
		SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

		SDL_GPUTransferBufferLocation transferLocation = {};
		transferLocation.transfer_buffer = vertexTransferBuffer;
		transferLocation.offset = 0;

		SDL_GPUBufferRegion bufferRegion = {};
		bufferRegion.buffer = vertexBuffer;
		bufferRegion.offset = 0;
		bufferRegion.size = bufferCreateInfo.size;

		SDL_UploadToGPUBuffer(copyPass, &transferLocation, &bufferRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);

		SDL_ReleaseGPUTransferBuffer(device, vertexTransferBuffer);

		//////////////////////////////////////////////////////////////////

		//Rendering

		SDL_GPUBufferBinding vertexBufferBinding = {};
		vertexBufferBinding.buffer = vertexBuffer;
		vertexBufferBinding.offset = 0;

		
		SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
		//SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		glm::mat4 meshMat = ConvertToGLMMat4(inModelMatrix);


		glm::mat4 mvp = proj * view * meshMat;

		mvp = glm::transpose(mvp);

		PerModelUniforms2 modelUnifroms;
		modelUnifroms.mvp = mvp;
		modelUnifroms.model = meshMat;

		SDL_PushGPUVertexUniformData(commandBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));

		SDL_DrawGPUPrimitives(renderPass, vertices.size(), 1, 0, 0);


		SDL_ReleaseGPUBuffer(device, vertexBuffer);
		//SDL_ReleaseGPUBuffer(device, indexBuffer);


	}


	void createPipeline(EDrawMode inDrawMode = EDrawMode::Wireframe) {

		// Vertex input state
		SDL_GPUVertexAttribute vertexAttributes[1] = {};

		// Position attribute
		vertexAttributes[0].location = 0;
		vertexAttributes[0].buffer_slot = 0;
		vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		vertexAttributes[0].offset = 0;

		SDL_GPUVertexBufferDescription vertexBufferDescription = {};
		vertexBufferDescription.slot = 0;
		vertexBufferDescription.pitch = sizeof(glm::vec3);
		vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

		SDL_GPUVertexInputState vertexInputState = {};
		vertexInputState.vertex_buffer_descriptions = &vertexBufferDescription;
		vertexInputState.num_vertex_buffers = 1;
		vertexInputState.vertex_attributes = vertexAttributes;
		vertexInputState.num_vertex_attributes = 1;


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

		if (inDrawMode == EDrawMode::Solid) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		}
		else {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
		}

		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeInfo);
		if (!pipeline) {
			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
			return;
		}

		SDL_Log("Successfully created pipeline");
	}

	void beginRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* swapchainTexture, Uint32 swapchainWidth, Uint32 swapchainHeight) {


		this->commandBuffer = commandBuffer;

		this->swapchainTexture = swapchainTexture;
		this->swapchainWidth = swapchainWidth;
		this->swapchainHeight = swapchainHeight;



		SDL_GPUColorTargetInfo physicsColorTargetInfo = {
			.texture = msaaColorTarget,
			.load_op = SDL_GPU_LOADOP_LOAD,
			.store_op = SDL_GPU_STOREOP_RESOLVE,
			.resolve_texture = resolveTarget,
		};

		SDL_GPUDepthStencilTargetInfo depthTargetInfo = { 0 };
		depthTargetInfo.texture = depthTexture;
		depthTargetInfo.clear_depth = 1.0f;
		depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;


		// Begin render pass
		renderPass = SDL_BeginGPURenderPass(
			commandBuffer,
			&physicsColorTargetInfo, 1,
			&depthTargetInfo
		);


		FrameDataUniforms2 uniforms;
		uniforms.view = view;
		uniforms.projection = proj;
		uniforms.viewProjection = proj * view;

		SDL_PushGPUVertexUniformData(commandBuffer, 0, &uniforms, sizeof(uniforms));


		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

		

	}


	void endRenderPass() {

		// End render pass
		SDL_EndGPURenderPass(renderPass);
	};


	glm::mat4 ConvertToGLMMat4(const RMat44Arg& inModelMatrix)
	{
		glm::mat4 result;

		// Assuming inModelMatrix is row-major, and glm::mat4 is column-major
		for (int row = 0; row < 4; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				result[col][row] = inModelMatrix(row, col); // transpose while copying
			}
		}

		return result;
	}
};


JPH_NAMESPACE_END

#endif // JPH_DEBUG_RENDERER
