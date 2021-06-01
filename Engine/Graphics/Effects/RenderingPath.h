#pragma once

#include "Graphics/Camera/Camera.h"
#include "Graphics/FrameGraph.h"
#include "CascadedShadowMap.h"
#include "Graphics/RendererStructs.h"
#include "VolumetricClouds.h"
#include "TimeOfDayManager.h"
#include "VCTGI.h"
#include "ProjectedGridWater.h"

namespace Engine
{
	class Game;
	class Renderer;
	class Buffer;

	enum class DebugType
	{
		CSM_SHADOW_MAP,
		BRIGHT_PASS,
		REFLECTION,
		VOXELS_TEXTURE,
		REFRACTION,
		ASSET_TEXTURE
	};

	struct DebugSettings
	{
		bool enable;
		DebugType type;
		bool enableWater;
		bool enableDebugDraw;
		bool enableVoxelVis;
		int mipLevel;
		int zSlice;
	};

	enum class RenderingPathType
	{
		FORWARD,
		FORWARD_PLUS,
	};

	class RenderingPath
	{
	public:
		RenderingPath();

		virtual void Init(Game *game);
		virtual void Dispose();
		virtual void Resize(unsigned int width, unsigned int height);
		virtual void Render();

		void EnableTerrainEditing();
		void DisableTerrainEditing();
		void UpdateTerrainEditTexture(Texture* texture);
		void SetAssetTextureAtlas(Texture* texture);

		void SetMainCamera(Camera *camera);
		Camera *GetMainCamera() const { return mainCamera; }

		FrameUBO &GetFrameData() { return frameData; }
		DirLight &GetMainDirLight() { return mainDirectionalLight; }
		DebugSettings &GetDebugSettings() { return debugSettings; }
		TimeOfDayManager &GetTOD() { return tod; }
		VolumetricClouds &GetVolumetricClouds() { return volumetricClouds; }
		VCTGI &GetVCTGI() { return vctgi; }
		ProjectedGridWater &GetProjectedGridWater() { return projectedGridWater; }
		FrameGraph &GetFrameGraph() { return frameGraph; }
		Font &GetFont() { return font; }

		Framebuffer *GetFinalFBForEditor() const { return postProcessFB; }

		void SaveRenderingSettings(const std::string &path);
		void LoadRenderingSettings(const std::string &path);

		void EnableUI(bool enable) { enableUI = enable; font.Enable(enable); }		// Disable font so it doesn't accumulate the text being added by Text, Button, etc
		bool IsUIEnabled() const { return enableUI; }

		void LockMainLightToTOD(bool lock) { lockMainLightToTOD = lock; }
		bool IsMainLightLockedToTOD() const { return lockMainLightToTOD; }

		void SetBaseLightShaftsIntensity(float val) { baseLightShaftsIntensity = val; }
		float GetBaseLightShaftsIntensity() const { return baseLightShaftsIntensity; }

	private:
		void SetupCSMPass();
		void SetupBloomPasses();
		void SetupReflectionPass();
		void SetupRefractionPass();
		void SetupVoxelizationPass();
		void SetupFXAAPass();
		void SetupTerrainEditPass();

		void PerformCSMPass();
		void PerformBrightPass();
		void PerformDownsample4Pass();
		void PerformDownsample8Pass();
		void PerformDownsample16Pass();
		void PerformUpsample8Pass();
		void PerformUpsample4Pass();
		void PerformReflectionPass();
		void PerformRefractionPass();

	protected:
		Game *game;
		Renderer *renderer;
		FrameGraph frameGraph;
		Camera *mainCamera;
		Camera uiCamera;
		unsigned int width;
		unsigned int height;
		RenderingPathType renderingPathType;

		float baseLightShaftsIntensity = 0.0f;
		bool lockMainLightToTOD = true;

		Buffer* frameDataUBO;
		Buffer* mainLightUBO;
		FrameUBO frameData;
		DirLight mainDirectionalLight;
		DebugSettings debugSettings;

		Mesh quadMesh;

		TimeOfDayManager tod;
		VolumetricClouds volumetricClouds;
		VCTGI vctgi;
		ProjectedGridWater projectedGridWater;

		RenderQueue renderQueues[8];

		// Shadows
		unsigned int csmQueueID;
		Framebuffer *csmFB;
		CSMInfo csmInfo;
		
		unsigned int voxelizationQueueID;
		unsigned int opaqueQueueID;
		unsigned int transparentQueueID;

		// UI Pass
		unsigned int uiQueueID;
		bool enableUI;
		Font font;

		// Debug draw
		unsigned int debugDrawQueueId;

		// Post Process Pass
		MaterialInstance *postProcMatInstance;
		unsigned int postProcPassID;
		Framebuffer *postProcessFB;

		// Reflections
		Framebuffer *reflectionFB;
		Framebuffer *refractionFB;

		// HDR Pass
		Framebuffer *hdrFB;

		// Bloom
		Framebuffer *brightPassFB;
		Framebuffer *downsampleFB[3];
		Framebuffer *upsampleFB[2];
		MaterialInstance *brightPassMatInstance;
		MaterialInstance *downsample4MatInstance;
		unsigned int downsample4PassID;
		MaterialInstance *downsample8MatInstance;
		unsigned int downsample8PassID;
		MaterialInstance *downsample16MatInstance;
		unsigned int downsample16PassID;
		MaterialInstance *upsample8MatInstance;
		unsigned int upsample8PassID;
		MaterialInstance *upsample4MatInstance;
		unsigned int upsample4PassID;

		// FXAA
		MaterialInstance *fxaaMat;
		Framebuffer *fxaaFB;

		// Terrain Edit Compute Pass
		MaterialInstance *terrainEditMat;
		bool isTerrainEditingEnabled = false;

		// Debug
		MaterialInstance* debugMat;
		struct DebugMaterialData
		{
			//glm::vec2 translation;
			//float scale;
			int isShadowMap;
		};
		DebugMaterialData debugMatData;
		Texture* assetTextureAtlas;
	};
}
