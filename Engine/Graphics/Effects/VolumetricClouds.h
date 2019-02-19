#pragma once

#include "Graphics\FrameGraph.h"
#include "Graphics\Mesh.h"
#include "Graphics\Material.h"

#include "Game\ComponentManagers\ScriptManager.h"

namespace Engine
{
	struct VolumetricCloudsData
	{
		float cloudCoverage;
		float cloudStartHeight;
		float cloudLayerThickness;
		float cloudLayerTopHeight;
		float timeScale;
		float hgForward;
		float densityMult;
		float ambientMult;
		float detailScale;
		float highCloudsCoverage;
		float highCloudsTimeScale;
		float silverLiningIntensity;
		float silverLiningSpread;
		float forwardSilverLiningIntensity;
		float directLightMult;
		glm::vec4 ambientTopColor;
		glm::vec4 ambientBottomColor;
	};

	class VolumetricClouds
	{
	public:
		void Init(Renderer *renderer, ScriptManager &scriptManager, FrameGraph &frameGraph, const Mesh &quadMesh);
		void Dispose();
		void Resize(unsigned int width, unsigned int height, FrameGraph &frameGraph);
		void EndFrame();

		Texture *GetCloudsTexture() const { return cloudReprojectionFB->GetColorTexture(); }
		VolumetricCloudsData &GetVolumetricCloudsData() { return volCloudsData; }
		unsigned int GetFrameNumber() const { return frameNumber; }
		unsigned int GetUpdateBlockSize() const { return cloudUpdateBlockSize; }
		glm::mat4 GetJitterMatrix() const;
		
		void SetCamera(Camera *camera);

	private:
		void PerformVolumetricCloudsPass();
		void PerformCloudsReprojectionPass();
		float Remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax);

	private:
		Renderer *renderer;
		Camera *camera;
		Framebuffer *cloudsLowResFB;
		Framebuffer *cloudReprojectionFB;
		Texture *previousFrameTexture;
		Mesh quadMesh;
		MaterialInstance *cloudMaterial;
		MaterialInstance *cloudReprojectionMaterial;
		Texture *baseNoiseTexture;
		Texture *highFreqNoiseTexture;
		Texture *weatherTexture;

		VolumetricCloudsData volCloudsData;

		unsigned int cloudsFBWidth;
		unsigned int cloudsFBHeight;
		unsigned int cloudUpdateBlockSize = 4;
		unsigned int frameNumbers[16];
		unsigned int frameNumber;
		unsigned int frameCount = 0;
	};
}
