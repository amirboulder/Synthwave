#pragma once

#ifdef JPH_DEBUG_RENDERER

#include "core/src/pch.h"

#include "../renderer/renderUtil.hpp"
#include "../renderer/rendererConfig.hpp"
#include "../renderer/PipelineLibrary/PipelineLibrary.hpp"


JPH_NAMESPACE_BEGIN

using namespace JPH;

// This class is a part of the renderer because the call to draw all needs to happen
// when a renderPass is active in the renderer so its called by the renderer.draw all
// The batch creation happens at the same rate as physics updates as anything more than that would be a waste
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

		glm::vec4 color;

	private:
		atomic<uint32>			mRefCount = 0;
	};
	
public:

	flecs::world& ecs;

	SDL_GPUGraphicsPipeline* pipelineMain = NULL;
	SDL_GPUGraphicsPipeline* pipelineLine = NULL;

	SDL_GPUBuffer* lineVertexBuffer = NULL;

	SDL_GPURenderPass* renderPass = NULL;

	BodyManager::DrawSettings drawSettings;

	glm::mat4 view;
	glm::mat4 proj;

	vector<BatchImpl*> batches;
	vector<glm::mat4> modelMatrices;
	vector<LineVertex> lines;

	fisiksDebugRenderer(flecs::world& ecs)
		: ecs(ecs)

	{
		//requires RenderContext RendererConfig to be initialized by Renderer
		const RenderContext& renderContext = ecs.get<RenderContext>();
		const RenderConfig& config = ecs.get<RenderConfig>();
		
		drawSettings.mDrawBoundingBox = config.DrawBoundingBoxPhysics;
		drawSettings.mDrawShapeWireframe = config.DrawShapeWireframePhysics;

		pipelineMain = ecs.lookup("pipelinePhysics").get<Pipeline>().pipeline;
		pipelineLine = ecs.lookup("pipelineLineMultiSample").get<Pipeline>().pipeline;

		Initialize();

	}

	
	void setCameraUniforms(RVec3Arg inCameraPos, glm::mat4 view, glm::mat4 proj)
	{
		mCameraPos = inCameraPos;
		mCameraPosSet = true;

		this->view = view;
		this->proj = proj;

	}

	virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
	{
		// Convert color from 0-255 to 0.0-1.0 range
		lines.emplace_back(
			glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()),
			glm::vec4(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f)
		);
		lines.emplace_back(
			glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()),
			glm::vec4(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f)
		);
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

		RenderContext& renderContext = ecs.get_mut<RenderContext>();

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

		batch->vertexBuffer = SDL_CreateGPUBuffer(renderContext.device, &bufferCreateInfo);
		RenderUtil::uploadBufferData(renderContext.device, batch->vertexBuffer, vertices.data(),
			vertices.size() * sizeof(glm::vec3), SDL_GPU_BUFFERUSAGE_VERTEX);

		return batch;
	};

	//TODO use indices properly
	virtual Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount) override {

		const RenderContext& renderContext = ecs.get<RenderContext>();

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

		RenderUtil::uploadBufferData(renderContext.device, batch->vertexBuffer, vertices.data(),
			vertices.size() * sizeof(glm::vec3), SDL_GPU_BUFFERUSAGE_VERTEX);

		return batch;
	}

	bool CreateLineBatch() {
		const RenderContext& renderContext = ecs.get<RenderContext>();

		if (lines.size() == 0) {
			return true;
		}

		// Release old buffer if it exists
		if (lineVertexBuffer) {
			SDL_ReleaseGPUBuffer(renderContext.device, lineVertexBuffer);
			lineVertexBuffer = nullptr;
		}


		RenderUtil::uploadBufferData(
			renderContext.device,
			lineVertexBuffer,
			lines.data(),
			lines.size() * sizeof(LineVertex),
			SDL_GPU_BUFFERUSAGE_VERTEX
		);

		return lineVertexBuffer != nullptr; 
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

		BatchImpl* batch = static_cast<BatchImpl*>(lod->mTriangleBatch.GetPtr());
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
		batch->color = glm::vec4(inModelColor.r / 255.0f, inModelColor.g / 255.0f, inModelColor.b / 255.0f, inModelColor.a / 255.0f);
		modelMatrices.push_back(ConvertToGLMMat4(inModelMatrix));

		return;

	}

	void drawAll() {

		const FrameContext& frameContext = ecs.get<FrameContext>();

		SDL_BindGPUGraphicsPipeline(renderPass, pipelineMain);
		
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

			PerModelUniforms modelUniforms;
			modelUniforms.mvp = mvp;
			modelUniforms.model = meshMat;

			SDL_PushGPUVertexUniformData(frameContext.commandBuffer, 1, &modelUniforms, sizeof(modelUniforms));

			SDL_DrawGPUPrimitives(renderPass, batch->mTriangles.size() * 3, 1, 0, 0);

		}


		if (lines.size() > 0) {
			if (!CreateLineBatch()) {
				SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create line batch");
				return;
			}

			SDL_BindGPUGraphicsPipeline(renderPass, pipelineLine);

			SDL_GPUBufferBinding vertexBufferBinding = {};
			vertexBufferBinding.buffer = lineVertexBuffer;
			vertexBufferBinding.offset = 0;
			SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

			SDL_DrawGPUPrimitives(renderPass, lines.size(), 1, 0, 0);
		}
		

	}

	void configDrawSettings(bool drawBoundingBox,bool drawShapeWireframe) {

		drawSettings.mDrawBoundingBox = drawBoundingBox;
		drawSettings.mDrawShapeWireframe = drawShapeWireframe;

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
