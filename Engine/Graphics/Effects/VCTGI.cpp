#include "VCTGI.h"

#include "Graphics/Renderer.h"
#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Buffers.h"

#include "Program/Input.h"
#include "Program/Log.h"

#include "include/glm/gtc/matrix_transform.hpp"

#include <iostream>

namespace Engine
{
	VCTGI::VCTGI()
	{
		cube = {};
		indirectBuffer = nullptr;
		voxelTexture = nullptr;
		voxelsPositionsBuffer = nullptr;
		lastCamPos = glm::vec3(0.0f);
		voxelVis.volumeSize = (float)VOXEL_RES;
		voxelVis.mipLevel = 0;
		voxelizationData = {};
		voxelMipmapData = {};
		voxelMipmapMat = nullptr;
		voxelCubeVisMat = nullptr;
		fillPositionsMat = nullptr;
		renderer = nullptr;
		giBaseMat = nullptr;

	}

	void VCTGI::Init(Renderer *renderer, FrameGraph &frameGraph, ScriptManager &scriptManager)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Initializing VCTGI\n");

		this->renderer = renderer;

		struct DrawElementsIndirectCommand
		{
			unsigned int vertexCount;
			unsigned int instanceCount;
			unsigned int firstIndex;
			unsigned int baseVertex;
			unsigned int baseInstance;
		};

		DrawElementsIndirectCommand cmd = {};
		cmd.vertexCount = 36;

		// Create the cube mesh to visualize the voxels
		cube = MeshDefaults::CreateCube(renderer);
			
		TextureParams params = {};
		params.filter = TextureFilter::LINEAR;
		params.wrap = TextureWrap::CLAMP_TO_BORDER;
		params.format = TextureFormat::RGBA;
		params.internalFormat = TextureInternalFormat::RGBA8;
		params.type = TextureDataType::UNSIGNED_BYTE;
		params.useMipmapping = true;
		params.usedAsStorageInGraphics = true;
		params.sampled = true;
		params.imageViewsWithDifferentFormats = true;

		voxelTexture = renderer->CreateTexture3DFromData(VOXEL_RES, VOXEL_RES, VOXEL_RES, params, nullptr);
		voxelsPositionsBuffer = renderer->CreateSSBO(VOXEL_RES * VOXEL_RES * VOXEL_RES * sizeof(glm::vec4), nullptr, sizeof(glm::vec4), BufferUsage::STATIC);
		voxelsPositionsBuffer->AddReference();
		indirectBuffer = renderer->CreateDrawIndirectBuffer(sizeof(cmd), &cmd);
		indirectBuffer->AddReference();

		SetupFillPositionsPass(frameGraph);
		SetupMipMapPass(frameGraph);
	}

	void VCTGI::Voxelize(const RenderQueue &toVoxelizeQueue)
	{
		if (Input::IsKeyPressed(KEY_V))
			voxelized = false;

		if (!voxelized)
		{
			//renderer->SetCamera(mainCamera);
			renderer->BindImage(0, 0, voxelTexture, ImageAccess::WRITE_ONLY);
			renderer->Submit(toVoxelizeQueue);
			voxelized = true;
			needsClear = true;
		}
	}
	
	void VCTGI::RenderVoxelVisualization(unsigned int mip)
	{
		const unsigned int mipVoxelRes = VOXEL_RES >> mip;

		voxelVis.volumeSize = (float)mipVoxelRes;
		voxelVis.mipLevel = (float)mip;

		RenderItem ri = {};
		ri.mesh = &cube;
		ri.shaderPass = 0;
		ri.matInstance = voxelCubeVisMat;
		ri.materialData = &voxelVis;
		ri.materialDataSize = sizeof(voxelVis);

		renderer->SubmitIndirect(ri, indirectBuffer);
	}

	void VCTGI::Dispose()
	{
		if (cube.vao)
			delete cube.vao;

		if (indirectBuffer)
		{
			indirectBuffer->RemoveReference();
			indirectBuffer = nullptr;
		}
		if (voxelsPositionsBuffer)
		{
			voxelsPositionsBuffer->RemoveReference();
			voxelsPositionsBuffer = nullptr;
		}
		if (voxelTexture)
		{
			voxelTexture->RemoveReference();
			voxelTexture = nullptr;
		}
		Log::Print(LogLevel::LEVEL_INFO, "Disposed VCTGI\n");
	}

	void VCTGI::UpdateProjection(const glm::vec3 &camPos)
	{
		float absX = fabsf(lastCamPos.x - camPos.x);
		float absY = fabsf(lastCamPos.y - camPos.y);
		float absZ = fabsf(lastCamPos.z - camPos.z);
		if (absX > 0.01f || absY > 0.01f || absZ > 0.01f)
		{
			voxelized = false;
			lastCamPos = camPos;
		}

		if (!voxelized)
		{
			float halfSize = voxelGridSize * 0.5f;

			glm::mat4 ortho = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, 0.0f, voxelGridSize);

			float interval = voxelGridSize / 8.0f;

			glm::vec3 newCamPos = glm::round(camPos / interval) * interval;

			glm::mat4 viewX = glm::lookAt(glm::vec3(newCamPos.x + halfSize, newCamPos.y, newCamPos.z), newCamPos, glm::vec3(0, 1, 0));
			glm::mat4 viewY = glm::lookAt(glm::vec3(newCamPos.x, newCamPos.y + halfSize, newCamPos.z), newCamPos, glm::vec3(0, 0, -1));
			glm::mat4 viewZ = glm::lookAt(glm::vec3(newCamPos.x, newCamPos.y, newCamPos.z + halfSize), newCamPos, glm::vec3(0, 1, 0));

			voxelizationData.orthoProjX = ortho * viewX;
			voxelizationData.orthoProjY = ortho * viewY;
			voxelizationData.orthoProjZ = ortho * viewZ;
		}
	}

	void VCTGI::EndFrame()
	{
		if (needsClear && !voxelized)
		{
			renderer->ClearImage(voxelTexture);
			needsClear = false;
		}
	}

	void VCTGI::CreateMat(ScriptManager &scriptManager)
	{
		fillPositionsMat = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/vctgi/fill_positions_mat.lua", {});
		voxelCubeVisMat = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/vctgi/voxel_cube_vis_mat.lua", cube.vao->GetVertexInputDescs());
		voxelMipmapMat = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/vctgi/voxel_mipmap_mat.lua", {});
	}

	void VCTGI::SetupFillPositionsPass(FrameGraph &frameGraph)
	{
		Pass &fillVoxelsVisBufferPass = frameGraph.AddPass("fillPositions");
		fillVoxelsVisBufferPass.SetIsCompute(true);
		fillVoxelsVisBufferPass.AddImageInput("voxelTextureMipmapped");
		fillVoxelsVisBufferPass.AddBufferOutput("voxelPositionsBuffer", voxelsPositionsBuffer);
		fillVoxelsVisBufferPass.AddBufferOutput("voxelsIndirectBuffer", indirectBuffer);

		fillVoxelsVisBufferPass.OnBarriers([this]()
		{
			// Make sure the voxel texture has been written to and the two buffers have been read
			BarrierImage bi = {};
			bi.image = voxelTexture;
			bi.readToWrite = false;
			bi.baseMip = 0;
			bi.numMips = voxelTexture->GetMipLevels();

			BarrierBuffer bb1 = {};
			bb1.buffer = indirectBuffer;
			bb1.readToWrite = true;
			BarrierBuffer bb2 = {};
			bb2.buffer = voxelsPositionsBuffer;
			bb2.readToWrite = true;

			Barrier b = {};
			b.images.push_back(bi);
			b.buffers.push_back(bb1);
			b.buffers.push_back(bb2);
			b.srcStage = PipelineStage::FRAGMENT | PipelineStage::VERTEX | PipelineStage::INDIRECT;
			b.dstStage = PipelineStage::COMPUTE;

			renderer->PerformBarrier(b);
		});

		fillVoxelsVisBufferPass.OnExecute([this]()
		{
			static const unsigned int localSize = 4;

			const unsigned int mip = (unsigned int)voxelVis.mipLevel;

			DispatchItem dispatchItem = {};
			dispatchItem.numGroupsX = (unsigned int)voxelVis.volumeSize / localSize;
			dispatchItem.numGroupsY = (unsigned int)voxelVis.volumeSize / localSize;
			dispatchItem.numGroupsZ = (unsigned int)voxelVis.volumeSize / localSize;
			dispatchItem.matInstance = fillPositionsMat;
			dispatchItem.shaderPass = 0;
			dispatchItem.materialData = &mip;
			dispatchItem.materialDataSize = sizeof(mip);

			//renderer->BindImage(0, mip, voxelTexture, ImageAccess::READ_ONLY);
			renderer->Dispatch(dispatchItem);
		});
	}

	void VCTGI::SetupMipMapPass(FrameGraph &frameGraph)
	{
		Pass &mipMapsPass = frameGraph.AddPass("voxelMipMaps");
		mipMapsPass.SetIsCompute(true);
		mipMapsPass.AddImageInput("voxelTexture");
		mipMapsPass.AddImageOutput("voxelTextureMipmapped", voxelTexture);

		mipMapsPass.OnBarriers([this]()
		{
			BarrierImage bi = {};
			bi.image = voxelTexture;
			bi.readToWrite = false;
			bi.baseMip = 0;
			bi.numMips = 1;		// Transition the first mip from write to read

			Barrier b = {};
			b.images.push_back(bi);
			b.srcStage = PipelineStage::FRAGMENT | PipelineStage::VERTEX | PipelineStage::INDIRECT;
			b.dstStage = PipelineStage::COMPUTE;

			renderer->PerformBarrier(b);
		});

		mipMapsPass.OnExecute([this]()
		{
			const unsigned int secondMipRes = VOXEL_RES >> 1;

			DispatchItem dispatchItem = {};

			for (unsigned int i = 0; i < voxelTexture->GetMipLevels() - 1; i++)
			{
				unsigned int dstMipRes = secondMipRes >> i;

				voxelMipmapData.mipIndex = i;		// In the shader we have an array of 7 images the first element is the second (1) mip, so we start with the index at 0
				voxelMipmapData.dstMipRes = dstMipRes;

				unsigned int count = dstMipRes / 4;

				if (dstMipRes == 2 || dstMipRes == 1)
					count = 1;

				dispatchItem.numGroupsX = count;
				dispatchItem.numGroupsY = count;
				dispatchItem.numGroupsZ = count;
				dispatchItem.matInstance = voxelMipmapMat;
				dispatchItem.shaderPass = 0;
				dispatchItem.materialData = &voxelMipmapData;
				dispatchItem.materialDataSize = sizeof(voxelMipmapData);

				//voxelTexture->BindAsImage(1, i + 1, true, ImageAccess::WRITE_ONLY, TextureInternalFormat::RGBA8);
				//renderer->BindImage(0, i + 1, voxelTexture, ImageAccess::WRITE_ONLY);
				renderer->Dispatch(dispatchItem);

				BarrierImage bi = {};
				bi.image = voxelTexture;
				bi.readToWrite = false;
				bi.baseMip = i + 1;
				bi.numMips = 1;

				Barrier b = {};
				b.srcStage = PipelineStage::COMPUTE;
				b.dstStage = PipelineStage::COMPUTE;
				b.images.push_back(bi);

				renderer->PerformBarrier(b);
			}
		});
	}
}
