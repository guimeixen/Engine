#pragma once

#include "VertexTypes.h"

namespace Engine
{
	class VertexArray;

	struct Mesh
	{
		VertexArray *vao;
		unsigned int vertexCount;
		unsigned int vertexOffset;
		unsigned int indexCount;
		unsigned int indexOffset;
		unsigned int instanceCount;
		unsigned int instanceOffset;
	};
}
