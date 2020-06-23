#include "VolumetricClouds.h"

#include "Graphics/VertexArray.h"
#include "Program/Random.h"
#include "Program/Log.h"

namespace Engine
{
	void VolumetricClouds::Init(Renderer *renderer, ScriptManager &scriptManager, FrameGraph &frameGraph, const Mesh &quadMesh)
	{
		baseNoiseTexture = nullptr;
		highFreqNoiseTexture = nullptr;
		this->renderer = renderer;
		this->quadMesh = quadMesh;
		cloudsFBWidth = renderer->GetWidth() / 2;
		cloudsFBHeight = renderer->GetHeight() / 2;

		// Make sure the clouds texture is divisible by the block size
		while (cloudsFBWidth % cloudUpdateBlockSize != 0)
		{
			cloudsFBWidth++;
		}
		while (cloudsFBHeight % cloudUpdateBlockSize != 0)
		{
			cloudsFBHeight++;
		}
		unsigned int cloudFBUpdateTextureWidth = cloudsFBWidth / cloudUpdateBlockSize;
		unsigned int cloudFBUpdateTextureHeight = cloudsFBHeight / cloudUpdateBlockSize;

		// Add the clouds rendering pass
		Pass &cloudsPass = frameGraph.AddPass("cloudsPass");
		AttachmentInfo colorAttach = {};
		colorAttach.width = cloudFBUpdateTextureWidth;
		colorAttach.height = cloudFBUpdateTextureHeight;
		colorAttach.params = { TextureWrap::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::RGBA16F, TextureDataType::FLOAT, false, false, true };

		cloudsPass.AddTextureOutput("cloudsLowRes", colorAttach);

		cloudsPass.OnSetup([renderer, &scriptManager, this, &frameGraph](const Pass *thisPass)
		{
			cloudsLowResFB = thisPass->GetFramebuffer();
			cloudMaterial = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/clouds_mat.lua", this->quadMesh.vao->GetVertexInputDescs());
			if (cloudMaterial)
			{
				cloudMaterial->textures[0] = baseNoiseTexture;
				cloudMaterial->textures[1] = highFreqNoiseTexture;
				cloudMaterial->textures[2] = weatherTexture;
				renderer->UpdateMaterialInstance(cloudMaterial);
			}
		});
		cloudsPass.OnExecute([this]() {PerformVolumetricCloudsPass(); });

		// Add the clouds reprojection pass
		Pass &cloudReprojectionPass = frameGraph.AddPass("cloudsReprojectionPass");
		colorAttach.width = cloudsFBWidth;
		colorAttach.height = cloudsFBHeight;

		cloudReprojectionPass.AddTextureInput("cloudsLowRes");
		cloudReprojectionPass.AddTextureOutput("cloudsTexture", colorAttach);	

		cloudReprojectionPass.OnSetup([renderer, &scriptManager, this](const Pass *thisPass)
		{
			cloudReprojectionFB = thisPass->GetFramebuffer();
			cloudReprojectionMaterial = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/clouds_reprojection_mat.lua", this->quadMesh.vao->GetVertexInputDescs());
			if (cloudReprojectionMaterial)
			{
				cloudReprojectionMaterial->textures[0] = cloudsLowResFB->GetColorTexture();
				cloudReprojectionMaterial->textures[1] = previousFrameTexture;
				renderer->UpdateMaterialInstance(cloudReprojectionMaterial);
			}
		});
		cloudReprojectionPass.OnExecute([this]() {PerformCloudsReprojectionPass(); });

		previousFrameTexture = renderer->CreateTexture2DFromData(cloudsFBWidth, cloudsFBHeight, colorAttach.params, nullptr);


		// Create the 3D noise textures

		struct TexData
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};

		const unsigned int resolution = 128;
		TexData *noise = new TexData[resolution * resolution * resolution];

		FILE *file = nullptr;
		file = fopen("Data/Resources/Textures/clouds/noise.data", "rb");

		if (file == nullptr)
		{
			// Generate the noise if the file doesn't exist and save it to file
			float stepSize = 1.0f / resolution;
			unsigned int index = 0;
			glm::vec3 coord = glm::vec3(0.0f);
			const float cellCount = 4;

			for (unsigned int z = 0; z < resolution; z++)
			{
				for (unsigned int y = 0; y < resolution; y++)
				{
					for (unsigned int x = 0; x < resolution; x++)
					{
						coord.x = x * stepSize;
						coord.y = y * stepSize;
						coord.z = z * stepSize;

						float r = Random::Perlin3D(coord.x, coord.y, coord.z, 8.0f, 3.0f, 2.0f, 0.5f);
						//float g = Random::Perlin3D(coord.x, coord.y, coord.z, 16.0f, 4.0f, 2.0f, 0.5f);
						float worley0 = 1.0f - Random::WorleyNoise(coord, cellCount * 2.0f);
						float worley1 = 1.0f - Random::WorleyNoise(coord, cellCount * 8.0f);
						float worley2 = 1.0f - Random::WorleyNoise(coord, cellCount * 14.0f);

						float worleyFBM = worley0 * 0.625f + worley1 * 0.25f + worley2 * 0.125f;

						float perlinWorley = Remap(r, 0.0f, 1.0f, worleyFBM, 1.0f);

						//worley0 = 1.0f - Random::WorleyNoise(coord, cellCount);
						worley1 = 1.0f - Random::WorleyNoise(coord, cellCount * 2);
						worley2 = 1.0f - Random::WorleyNoise(coord, cellCount * 4);
						float worley3 = 1.0f - Random::WorleyNoise(coord, cellCount * 8);
						float worley4 = 1.0f - Random::WorleyNoise(coord, cellCount * 16);

						float worleyFBM0 = worley1 * 0.625f + worley2 * 0.25f + worley3 * 0.125f;
						float worleyFBM1 = worley2 * 0.625f + worley3 * 0.25f + worley3 * 0.125f;
						float worleyFBM2 = worley3 * 0.75f + worley4 * 0.25f;

						index = z * resolution*resolution + y * resolution + x;
						noise[index].r = static_cast<unsigned char>(perlinWorley * 255.0f);
						noise[index].g = static_cast<unsigned char>(worleyFBM0 * 255.0f);
						noise[index].b = static_cast<unsigned char>(worleyFBM1 * 255.0f);
						noise[index].a = static_cast<unsigned char>(worleyFBM2 * 255.0f);
					}
				}
			}

			std::fstream file = std::fstream("Data/Resources/Textures/clouds/noise.data", std::ios::out | std::ios::binary);
			file.write((char*)&noise[0], resolution * resolution * resolution * sizeof(TexData));
			file.close();
		}
		else
		{
			fread(noise, 1, resolution * resolution * resolution * sizeof(TexData), file);
			fclose(file);
		}

		struct HighFreqNoise
		{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;	// not used
		};
		const unsigned int highFreqRes = 32;
		HighFreqNoise *highFreqNoise = new HighFreqNoise[highFreqRes * highFreqRes * highFreqRes];

		file = fopen("Data/Resources/Textures/clouds/highFreqNoise.data", "rb");

		if (file == nullptr)
		{
			// Generate the noise if the file doesn't exist and save it to file
			float stepSize = 1.0f / highFreqRes;
			unsigned int index = 0;
			glm::vec3 coord = glm::vec3(0.0f);
			const float cellCount = 2;

			for (unsigned int z = 0; z < highFreqRes; z++)
			{
				for (unsigned int y = 0; y < highFreqRes; y++)
				{
					for (unsigned int x = 0; x < highFreqRes; x++)
					{
						coord.x = x * stepSize;
						coord.y = y * stepSize;
						coord.z = z * stepSize;

						float worley0 = 1.0f - Random::WorleyNoise(coord, cellCount);
						float worley1 = 1.0f - Random::WorleyNoise(coord, cellCount * 2.0f);
						float worley2 = 1.0f - Random::WorleyNoise(coord, cellCount * 4.0f);
						float worley3 = 1.0f - Random::WorleyNoise(coord, cellCount * 8.0f);

						float worleyFBM0 = worley0 * 0.625f + worley1 * 0.25f + worley2 * 0.125f;
						float worleyFBM1 = worley1 * 0.625f + worley2 * 0.25f + worley3 * 0.125f;
						float worleyFBM2 = worley2 * 0.75f + worley3 * 0.25f;

						index = z * highFreqRes * highFreqRes + y * highFreqRes + x;
						highFreqNoise[index].r = static_cast<unsigned char>(worleyFBM0 * 255.0f);
						highFreqNoise[index].g = static_cast<unsigned char>(worleyFBM1 * 255.0f);
						highFreqNoise[index].b = static_cast<unsigned char>(worleyFBM2 * 255.0f);
						highFreqNoise[index].a = 255;
					}
				}
			}

			std::fstream file = std::fstream("Data/Resources/Textures/clouds/highFreqNoise.data", std::ios::out | std::ios::binary);
			file.write((char*)&highFreqNoise[0], highFreqRes * highFreqRes * highFreqRes * sizeof(HighFreqNoise));
			file.close();
		}
		else
		{
			fread(highFreqNoise, 1, highFreqRes * highFreqRes * highFreqRes * sizeof(HighFreqNoise), file);
			fclose(file);
		}

		TextureParams noiseParams = { TextureWrap::MIRRORED_REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA,TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, false, false };
		baseNoiseTexture = renderer->CreateTexture3DFromData(resolution, resolution, resolution, noiseParams, noise);
		highFreqNoiseTexture = renderer->CreateTexture3DFromData(highFreqRes, highFreqRes, highFreqRes, noiseParams, highFreqNoise);

		TextureParams params = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA,TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, false, false };
		weatherTexture = renderer->CreateTexture2D("Data/Resources/Textures/clouds/weather.png", params);

		delete[] noise;
		delete[] highFreqNoise;

		volCloudsData.ambientBottomColor = glm::vec4(0.5f, 0.71f, 1.0f, 0.0f);
		volCloudsData.ambientTopColor = glm::vec4(1.0f);
		volCloudsData.ambientMult = 0.265f;
		volCloudsData.cloudCoverage = 0.45f;
		volCloudsData.cloudStartHeight = 2200.0f;
		volCloudsData.cloudLayerThickness = 2250.0f;
		volCloudsData.cloudLayerTopHeight = volCloudsData.cloudStartHeight + volCloudsData.cloudLayerThickness;
		volCloudsData.densityMult = 2.27f;
		volCloudsData.detailScale = 9.0f;
		volCloudsData.directLightMult = 2.5f;
		volCloudsData.forwardSilverLiningIntensity = 0.35f;
		volCloudsData.hgForward = 0.6f;
		volCloudsData.highCloudsCoverage = 0.0f;
		volCloudsData.highCloudsTimeScale = 0.01f;
		volCloudsData.silverLiningIntensity = 0.65f;
		volCloudsData.silverLiningSpread = 0.88f;
		volCloudsData.timeScale = 0.001f;

		int i = 0;
		for (i = 0; i < 16; i++)
		{
			frameNumbers[i] = i;
		}
		while (i-- > 0)
		{
			int k = frameNumbers[i];
			int j = (int)(Random::Float() * 1000.0f) % 16;
			frameNumbers[i] = frameNumbers[j];
			frameNumbers[j] = k;
		}
		
		frameNumber = frameNumbers[0];
		//frameNumber = 0;
	}

	void VolumetricClouds::Dispose()
	{
		if (baseNoiseTexture)
			baseNoiseTexture->RemoveReference();
		if (highFreqNoiseTexture)
			highFreqNoiseTexture->RemoveReference();
		if (weatherTexture)
			weatherTexture->RemoveReference();
		if (previousFrameTexture)
			previousFrameTexture->RemoveReference();

		Log::Print(LogLevel::LEVEL_INFO, "Disposing volumetric clouds\n");
	}

	void VolumetricClouds::Resize(unsigned int width, unsigned int height, FrameGraph &frameGraph)
	{
		//cloudMaterial->textures[3] = frameGraph.GetPass("base").GetFramebuffer()->GetDepthTexture();
		//renderer->UpdateMaterialInstance(cloudMaterial);
	}

	void VolumetricClouds::EndFrame()
	{
		renderer->CopyImage(cloudReprojectionFB->GetColorTexture(), previousFrameTexture);

		frameCount++;
		frameNumber = frameNumbers[frameCount % (cloudUpdateBlockSize * cloudUpdateBlockSize)];
		//frameNumber = (frameNumber + 1) % (cloudUpdateBlockSize * cloudUpdateBlockSize);
	}

	glm::mat4 VolumetricClouds::GetJitterMatrix() const
	{
		// Create the jitter matrix that will be use to translate the projection matrix
		int x = frameNumber % cloudUpdateBlockSize;
		int y = frameNumber / cloudUpdateBlockSize;

		float offsetX = x * 2.0f / cloudsFBWidth;		// The size of a pixel is 1/width but because the range we'er applying the offset is in [-1,1] we have to multiply by 2 to get the size of a pixel
		float offsetY = y * 2.0f / cloudsFBHeight;

		glm::mat4 jitterMatrix = glm::mat4(1.0f);
		jitterMatrix[3] = glm::vec4(offsetX, offsetY, 0.0f, 1.0f);

		return jitterMatrix;
	}

	void VolumetricClouds::SetCamera(Camera *camera)
	{
		if (!camera)
			return;
		this->camera = camera;
	}

	void VolumetricClouds::PerformVolumetricCloudsPass()
	{
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.matInstance = cloudMaterial;
		ri.shaderPass = 0;

		renderer->SetCamera(camera);
		renderer->Submit(ri);
	}

	void VolumetricClouds::PerformCloudsReprojectionPass()
	{
		RenderItem ri = {};
		ri.mesh = &quadMesh;
		ri.matInstance = cloudReprojectionMaterial;
		ri.shaderPass = 0;

		renderer->Submit(ri);
	}

	float VolumetricClouds::Remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
	{
		return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
	}
}