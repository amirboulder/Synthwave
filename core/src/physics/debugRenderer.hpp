#pragma once


// This class only exists in debug mode

#ifdef JPH_DEBUG_RENDERER

#include <Jolt/Jolt.h>

#include "../renderer/renderUtil.hpp"
#include "../renderer/rendererConfig.hpp"

#include <Jolt/Renderer/DebugRenderer.h>

JPH_NAMESPACE_BEGIN

using namespace JPH;


class fisiksDebugRenderer : public JPH::DebugRenderer
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

		SDL_GPUBuffer* vertexBuffer = NULL;

	private:
		atomic<uint32>			mRefCount = 0;
	};
	
public:

	flecs::world& ecs;

	SDL_GPUGraphicsPipeline* pipeline = NULL;

	SDL_GPUShader* vertexShader = NULL;
	SDL_GPUShader* fragmentShader = NULL;

	SDL_GPURenderPass* renderPass = NULL;

	BodyManager::DrawSettings drawSettings;

	

	glm::mat4 view;
	glm::mat4 proj;

	//used for all the rendering that happens outside main renderPass cleared in the next frame
	vector<const BatchImpl*> batches;
	vector<glm::mat4> modelMatrices;

	fisiksDebugRenderer(flecs::world& ecs)
		: ecs(ecs)

	{

		const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();
		const RendererConfig& config = ecs.get<RendererConfig>();

		
		drawSettings.mDrawBoundingBox = config.DrawBoundingBoxPhysics;
		drawSettings.mDrawShapeWireframe = config.DrawShapeWireframePhysics;

		//shader::generateSpirvShaders("shaders/slang/physicsRender.slang", "shaders/compiled/physicsRender.vert.spv", "shaders/compiled/physicsRender.frag.spv");
		RenderUtil::loadShaderSPRIV(rendercontext.device, vertexShader, "shaders/compiled/physicsRender.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 0, 2, 0, 0);
		RenderUtil::loadShaderSPRIV(rendercontext.device, fragmentShader, "shaders/compiled/physicsRender.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0, 0, 0);

		createPipeline();

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
		//TODO collect all lines so that we can draw bounding boxes!
		cout << "fisiksDebugRenderer:: DrawLine was called \n";
	}

	virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override
	{
		// NO
		cout << "fisiksDebugRenderer:: DrawTriangle was called \n";
	}

	virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override
	{
		// maybe?
		cout << "fisiksDebugRenderer:: DrawText3D was called \n";
	}

	virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override {

		RenderConxtext& rendercontext = ecs.get_mut<RenderConxtext>();

		BatchImpl* batch = new BatchImpl;
		if (inTriangles == nullptr || inTriangleCount == 0)
			return batch;

		batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);

		// Create vertex data from triangles
		std::vector<glm::vec3> vertices;
		vertices.reserve(inTriangleCount * 3);

		for (int i = 0; i < inTriangleCount; ++i) {
			for (int j = 0; j < 3; ++j) {
				vertices.emplace_back(
					inTriangles[i].mV[j].mPosition.x,
					inTriangles[i].mV[j].mPosition.y,
					inTriangles[i].mV[j].mPosition.z
				);
			}
		}

		// Create and upload buffer
		SDL_GPUBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
		bufferCreateInfo.size = vertices.size() * sizeof(glm::vec3);

		batch->vertexBuffer = SDL_CreateGPUBuffer(rendercontext.device, &bufferCreateInfo);
		RenderUtil::uploadBufferData(rendercontext.device, batch->vertexBuffer, vertices.data(),
			vertices.size() * sizeof(glm::vec3), SDL_GPU_BUFFERUSAGE_VERTEX);

		return batch;
	};

	//TODO use indcies properly
	virtual Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount) override {

		const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();

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

		RenderUtil::uploadBufferData(rendercontext.device, batch->vertexBuffer, vertices.data(),
			vertices.size() * sizeof(glm::vec3), SDL_GPU_BUFFERUSAGE_VERTEX);

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
		if (!batch)
			return;
		
		
		if (batch->mTriangles.empty()) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No triangles to render in physics debug rendering");
			return;
		}

		if (!batch->vertexBuffer) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "batch->vertexBuffer is null skipped physics debug rendering");
			return;
		}


		batches.push_back(batch);
		modelMatrices.push_back(ConvertToGLMMat4(inModelMatrix));

		return;

	}

	void drawAll() {

		//Bind pipeline

		const FrameContext& frameContext = ecs.get<FrameContext>();

		SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
		
		for (int i = 0; i < batches.size(); i++ ) {

			auto batch = batches[i];

			SDL_GPUBufferBinding vertexBufferBinding = {};
			vertexBufferBinding.buffer = batch->vertexBuffer;
			vertexBufferBinding.offset = 0;


			SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
			//SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

			glm::mat4 meshMat = modelMatrices[i];

			glm::mat4 mvp = proj * view * meshMat;

			mvp = glm::transpose(mvp);

			PerModelUniforms modelUnifroms;
			modelUnifroms.mvp = mvp;
			modelUnifroms.model = meshMat;

			SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 1, &modelUnifroms, sizeof(modelUnifroms));


			SDL_DrawGPUPrimitives(renderPass, batch->mTriangles.size() * 3, 1, 0, 0);

		}

		//Batches are cleared during phyiscs update	

	}

	void configDrawSettings(bool drawBoundingBox,bool drawShapeWireframe) {

		// keep drawBoundingBox false until DrawLine is implimented
		drawSettings.mDrawBoundingBox = false;
		drawSettings.mDrawShapeWireframe = drawShapeWireframe;
		

	}


	void createPipeline(EDrawMode inDrawMode = EDrawMode::Wireframe) {

		 const RenderConxtext& rendercontext = ecs.get<RenderConxtext>();
		 const RendererConfig& renderConfig = ecs.get<RendererConfig>();

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
		coldescs.format = SDL_GetGPUSwapchainTextureFormat(rendercontext.device, rendercontext.window);

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
			.sample_count = renderConfig.sampleCountMSAA,  // Enable MSAA in pipeline
			.sample_mask = 0  // Use all samples
		};

		pipeInfo.vertex_input_state = vertexInputState;

		if (drawSettings.mDrawShapeWireframe ) {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
		}
		else {
			pipeInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
		}

		pipeInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
		pipeInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
		pipeInfo.props = 0;

		pipeline = SDL_CreateGPUGraphicsPipeline(rendercontext.device, &pipeInfo);
		if (!pipeline) {
			SDL_Log("Failed to create fill pipeline: %s", SDL_GetError());
			return;
		}

		SDL_Log("Successfully created pipeline");
	}

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
