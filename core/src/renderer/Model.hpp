#pragma once

#include "Mesh.hpp"
#include "Material.hpp"
#include "renderUtil.hpp"

#include "GeometryPool.hpp"



Mesh createGridMesh(uint32_t size) {

	Mesh mesh;

	GridGenerator::generateGrid(size, mesh.vertices, mesh.indices);

	mesh.subMeshes[0].indexCount = mesh.indices.size();
	mesh.subMeshes[0].vertexCount = mesh.vertices.size();

	mesh.subMeshes[0].baseVertex = 0;
	mesh.subMeshes[0].firstIndex = 0;
	mesh.subMeshes[0].materialID = 0;

	mesh.subMeshCount = 1;
}




