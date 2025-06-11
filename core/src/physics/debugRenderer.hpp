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

	GLuint triangleShaderID;
	//GLuint textShaderID;

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
	MyDebugRenderer(GLuint triangleShaderID)
		: triangleShaderID(triangleShaderID)
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
		// Select LOD
		const LOD* lod = inGeometry->mLODs.data();
		if (mCameraPosSet)
			lod = &inGeometry->GetLOD(Vec3(mCameraPos), inWorldSpaceBounds, inLODScaleSq);
		if (!lod)
			return;

		const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());
		if (!batch || batch->mTriangles.empty())
			return;

		// Set draw mode BEFORE drawing
		switch (inDrawMode)
		{
		case EDrawMode::Wireframe:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case EDrawMode::Solid:
		default:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		}

		GLuint VBO, VAO;
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);

		// Check if OpenGL objects were created successfully
		if (VAO == 0 || VBO == 0) {
			if (VAO != 0) glDeleteVertexArrays(1, &VAO);
			if (VBO != 0) glDeleteBuffers(1, &VBO);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore polygon mode
			return;
		}


		size_t totalVertices = batch->mTriangles.size() * 3; // 3 vertices per triangle
		size_t bufferSize = totalVertices * sizeof(Vertex);

		glNamedBufferData(VBO, bufferSize, nullptr, GL_STATIC_DRAW);

		// Copy vertex data from triangles to buffer
		Vertex* mappedBuffer = static_cast<Vertex*>(glMapNamedBuffer(VBO, GL_WRITE_ONLY));
		if (mappedBuffer) {
			size_t vertexIndex = 0;
			for (const auto& triangle : batch->mTriangles) {
				mappedBuffer[vertexIndex++] = triangle.mV[0];
				mappedBuffer[vertexIndex++] = triangle.mV[1];
				mappedBuffer[vertexIndex++] = triangle.mV[2];
			}
			glUnmapNamedBuffer(VBO);
		}
		else {
			// Fallback: use glNamedBufferSubData
			size_t offset = 0;
			for (const auto& triangle : batch->mTriangles) {
				glNamedBufferSubData(VBO, offset, sizeof(Vertex) * 3, triangle.mV);
				offset += sizeof(Vertex) * 3;
			}
		}

		// Configure VAO with proper vertex attributes for VertexData
		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));

		// Attribute 0: Position (Float3 - 3 floats)
		glEnableVertexArrayAttrib(VAO, 0);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, mPosition));

		// Attribute 1: Normal (Float3 - 3 floats)
		glEnableVertexArrayAttrib(VAO, 1);
		glVertexArrayAttribBinding(VAO, 1, 0);
		glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, mNormal));

		// Attribute 2: UV (Float2 - 2 floats)
		glEnableVertexArrayAttrib(VAO, 2);
		glVertexArrayAttribBinding(VAO, 2, 0);
		glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, mUV));

		// Attribute 3: Color (Color - assuming 4 floats RGBA)
		glEnableVertexArrayAttrib(VAO, 3);
		glVertexArrayAttribBinding(VAO, 3, 0);
		glVertexArrayAttribFormat(VAO, 3, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, mColor));


		// Use shader and bind VAO
		glUseProgram(triangleShaderID);
		Shader::setMat4("model", ConvertToGLMMat4(inModelMatrix), triangleShaderID);

		unsigned int location = glGetUniformLocation(triangleShaderID, "color");
		glUniform3f(location, inModelColor.r, inModelColor.g, inModelColor.b);

		glBindVertexArray(VAO);

		// Draw triangles
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(totalVertices));

		// Cleanup
		glBindVertexArray(0);
		glDeleteBuffers(1, &VBO);
		glDeleteVertexArrays(1, &VAO);

		// Restore polygon mode to default
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
