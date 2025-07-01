#pragma once

#include <Jolt/Jolt.h>

#ifdef JPH_DEBUG_RENDERER

#include <Jolt/Renderer/DebugRenderer.h>

JPH_NAMESPACE_BEGIN

#include "../renderer/Shader.hpp"

using namespace JPH;


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
	MyDebugRenderer()
		
	{
		Initialize();

	}

	void SetCameraPos(RVec3Arg inCameraPos)
	{
		mCameraPos = inCameraPos;
		mCameraPosSet = true;
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
