#pragma once

#include "Game\ComponentManagers\ScriptManager.h"
#include "Graphics\Renderer.h"
#include "Graphics/FrameGraph.h"

namespace Engine
{
	class Texture;
	struct MaterialInstance;
	class Camera;

	struct VoxelizationData
	{
		glm::mat4 orthoProjX;
		glm::mat4 orthoProjY;
		glm::mat4 orthoProjZ;
	};

	class VCTGI
	{
	public:
		VCTGI();

		void Init(Renderer *renderer, FrameGraph &frameGraph, ScriptManager &scriptManager);
		void Voxelize(const RenderQueue &toVoxelizeQueue);
		void RenderVoxelVisualization(unsigned int mip);
		void Dispose();

		void UpdateProjection(const glm::vec3 &camPos);
		void SetVoxelGridSize(float size) { voxelGridSize = size; voxelized = false; }

		Texture *GetVoxelTexture() const { return voxelTexture; }
		Buffer *GetIndirectBuffer() const { return indirectBuffer; }
		Buffer *GetVoxelPositionsBuffer() const { return voxelsPositionsBuffer; }

		const VoxelizationData &GetVoxelizationData() const { return voxelizationData; }
		float GetVoxelGridSize() const { return voxelGridSize; }
		float GetVoxelScale() const { return 1.0f / voxelGridSize; }

		void EndFrame();

		void CreateMat(ScriptManager &scriptManager);

	private:
		void SetupFillPositionsPass(FrameGraph &frameGraph);
		void SetupMipMapPass(FrameGraph &frameGraph);

	private:		
		struct VoxelMipmapData
		{
			int mipIndex;
			int dstMipRes;
		};
		struct VoxelVis
		{
			float volumeSize;
			float mipLevel;
		};

		Renderer *renderer;
		Texture *voxelTexture;
		Buffer *indirectBuffer;
		Buffer *voxelsPositionsBuffer;

		Mesh cube;
		MaterialInstance *giBaseMat;

		MaterialInstance *voxelMipmapMat;
		MaterialInstance *fillPositionsMat;
		MaterialInstance *voxelCubeVisMat;

		bool voxelized = false;
		bool needsClear = false;
		glm::vec3 lastCamPos;

		const unsigned int VOXEL_RES = 128;
		float voxelGridSize = 16.0f;
		VoxelizationData voxelizationData;
		VoxelMipmapData voxelMipmapData;
		VoxelVis voxelVis;
	};

}

