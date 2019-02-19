#pragma once

#include "Mesh.h"

namespace Engine
{
	class Renderer;

	namespace MeshDefaults
	{
		// Use default size 0.5 to make a unit cube
		Mesh CreateCube(Renderer *renderer, float size = 0.5f, bool lines = false, bool instanced = false);		// lines == true won't make the cube render as lines but will make it ready to be rendered as lines with a material that uses lines topology
		Mesh CreateLine(Renderer *renderer, float size = 0.5f);
		Mesh CreateSphere(Renderer *renderer, float radius = 0.5f, bool instanced = false);
		Mesh CreateQuad(Renderer *renderer, float size = 1.0f);
		Mesh CreatePlane(Renderer *renderer, float size = 0.5f);
		Mesh CreateScreenSpaceGrid(Renderer *renderer, unsigned int resolution = 1);
		Mesh CreateFromVertices(Renderer *renderer, const VertexInputDesc &desc, unsigned int vertexCount, const void *vertices, unsigned int verticesSize, unsigned int indexCount = 0, const unsigned short *indices = nullptr, unsigned int indexSize = 0);
	}
}
