#pragma once

#include "VertexTypes.h"

#include "include\glm\glm.hpp"

#include <vector>

namespace Engine
{
	struct Mesh;
	struct MaterialInstance;
	class Material;
	class Texture;
	class Frustum;
	class Shader;

	struct Viewport
	{
		int x;
		int y;
		unsigned int width;
		unsigned int height;
	};

	struct BlendState
	{
		bool enableBlending;
		unsigned int srcColorFactor;
		unsigned int dstColorFactor;
		unsigned int srcAlphaFactor;
		unsigned int dstAlphaFactor;
		bool enableColorWriting;
	};

	struct DepthStencilState
	{
		bool depthEnable;
		bool depthWrite;
		unsigned int depthFunc;
	};

	struct RasterizerState
	{
		unsigned int cullFace;
		unsigned int frontFace;
		bool enableCulling;
	};

	struct ShaderPass
	{
		Shader *shader;
		BlendState blendState;
		DepthStencilState depthStencilState;
		RasterizerState rasterizerState;
		std::vector<VertexInputDesc> vertexInputDescs;
		unsigned int id;
		unsigned int queueID;
		unsigned int topology;
		unsigned int pipelineID;
		unsigned int stateID;
		bool isCompute;
	};

	struct RenderItem
	{
		const Mesh *mesh;
		const MaterialInstance *matInstance;
		const void* meshParams;				// transform, bone transforms,...
		unsigned int meshParamsSize;
		const void* instanceData;
		unsigned int instanceDataSize;
		unsigned int shaderPass;
		const glm::mat4 *transform;
		const void *materialData;
		unsigned int materialDataSize;
		unsigned long long sortKey;
	};

	struct DispatchItem
	{
		unsigned int numGroupsX;
		unsigned int numGroupsY;
		unsigned int numGroupsZ;
		const MaterialInstance *matInstance;
		unsigned int shaderPass;
		const void *materialData;
		unsigned int materialDataSize;
	};

	typedef std::vector<unsigned int> VisibilityIndices;
	typedef std::vector<RenderItem> RenderQueue;

	class RenderQueueGenerator
	{
	public:
		virtual void Cull(unsigned int passAndFrustumCount, unsigned int *passIds, const Frustum *frustums, std::vector<VisibilityIndices*> &out) = 0;
		virtual void GetRenderItems(unsigned int passCount, unsigned int *passIds, const VisibilityIndices &visibility, RenderQueue &outQueues) = 0;
	};
}
