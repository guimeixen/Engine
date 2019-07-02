#include "GXMShader.h"

#include "Program/Log.h"

namespace Engine
{
	GXMShader::GXMShader(SceGxmShaderPatcher *shaderPatcher, FileManager *fileManager, const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState)
	{
		modelMatrixParam = nullptr;
		Log::Print(LogLevel::LEVEL_INFO, "Loading vertex shader %s  and fragment shader  %s\n", vertexName.c_str(), fragmentName.c_str());

		std::string vertexPath = "Data/Shaders/GXM/" + vertexName + "_v_cg.gxp";
		std::string fragmentPath = "Data/Shaders/GXM/" + fragmentName + "_f_cg.gxp";

		std::ifstream vertexFile = fileManager->OpenForReading(vertexPath, std::ios::ate | std::ios::binary);
		if (!vertexFile.is_open())
		{
			Log::Print(LogLevel::LEVEL_INFO, "Failed to open vertex shader file: %s\n", vertexPath.c_str());
			return;
		}
		std::ifstream fragmentFile = fileManager->OpenForReading(fragmentPath, std::ios::ate | std::ios::binary);
		if (!fragmentFile.is_open())
		{
			Log::Print(LogLevel::LEVEL_INFO, "Failed to open fragment shader file: %s\n", fragmentPath.c_str());
			return;
		}

		vertexProgramGxp = (SceGxmProgram*)fileManager->ReadEntireFile(vertexFile, true);
		fragmentProgramGxp = (SceGxmProgram*)fileManager->ReadEntireFile(fragmentFile, true);

		sceGxmShaderPatcherRegisterProgram(shaderPatcher, vertexProgramGxp, &vertexProgramID);
		Log::Print(LogLevel::LEVEL_INFO, "Registered vertex program\n");
		sceGxmShaderPatcherRegisterProgram(shaderPatcher, fragmentProgramGxp, &fragmentProgramID);
		Log::Print(LogLevel::LEVEL_INFO, "Registered fragment program\n");

		// Right now use just the first desc and one vertex stream

		std::vector<SceGxmVertexAttribute> vertexAttributes;
		std::vector<SceGxmVertexStream> vertexStreams;

		if (descs.size() > 0)
		{
			for (size_t i = 0; i < 1; i++)
			{
				const VertexInputDesc &desc = descs[i];

				SceGxmVertexStream stream = {};
				stream.stride = desc.stride;
				if (desc.instanced)
					stream.indexSource = SCE_GXM_INDEX_SOURCE_INSTANCE_16BIT;
				else
					stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

				vertexStreams.push_back(stream);

				for (size_t j = 0; j < desc.attribs.size(); j++)
				{
					const VertexAttribute &attrib = desc.attribs[j];

					SceGxmVertexAttribute va = {};
					va.componentCount = attrib.count;
					va.offset = attrib.offset;
					va.streamIndex = i;

					if (attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
						va.format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
					else if (attrib.vertexAttribFormat == VertexAttributeFormat::INT)
						va.format = SCE_GXM_ATTRIBUTE_FORMAT_S16;

					if (j == 0)
					{
						const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(vertexProgramGxp, "inPos");
						if (param)
						{
							va.regIndex = sceGxmProgramParameterGetResourceIndex(param);
						}
						else
						{
							Log::Print(LogLevel::LEVEL_ERROR, "Unable to find program parameter! The first vertex attribute must be named pos on shader %s\n", vertexName.c_str());
							return;
						}
					}
					else if (j == 1)
					{
						const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(vertexProgramGxp, "inUv");
						if (param)
						{
							va.regIndex = sceGxmProgramParameterGetResourceIndex(param);
						}
						else
						{
							Log::Print(LogLevel::LEVEL_ERROR, "Unable to find program parameter! The second vertex attribute must be named inUv on shader %s\n", vertexName.c_str());
							return;
						}
					}
					else if (j == 2)
					{
						const SceGxmProgramParameter *param = sceGxmProgramFindParameterByName(vertexProgramGxp, "inNormal");
						if (param)
						{
							va.regIndex = sceGxmProgramParameterGetResourceIndex(param);
						}
						else
						{
							Log::Print(LogLevel::LEVEL_ERROR, "Unable to find program parameter! The third vertex attribute must be named inNormal on shader %s\n", vertexName.c_str());
							return;
						}
					}

					vertexAttributes.push_back(va);
				}
			}
		}
		else
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Vertex Input Desc size == 0");
			return;
		}

		SceGxmBlendInfo blending = {};

		if (blendState.enableColorWriting)
		{
			blending.colorMask = SCE_GXM_COLOR_MASK_ALL;
			blending.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
			blending.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
		}
		else
		{
			// Used for shadow mapping so libgxm disables the fragment shader
			blending.colorMask = SCE_GXM_COLOR_MASK_NONE;
			blending.colorFunc = SCE_GXM_BLEND_FUNC_NONE;
			blending.alphaFunc = SCE_GXM_BLEND_FUNC_NONE;
		}
		
		blending.colorSrc = (SceGxmBlendFactor)blendState.srcColorFactor;
		blending.colorDst = (SceGxmBlendFactor)blendState.dstColorFactor;
		
		blending.alphaSrc = (SceGxmBlendFactor)blendState.srcAlphaFactor;
		blending.alphaDst = (SceGxmBlendFactor)blendState.dstAlphaFactor;

		sceGxmShaderPatcherCreateVertexProgram(shaderPatcher, vertexProgramID, vertexAttributes.data(), (unsigned int)vertexAttributes.size(), vertexStreams.data(), (unsigned int)vertexStreams.size(), &vertexProgram);
		Log::Print(LogLevel::LEVEL_INFO, "Created vertex program\n");

		if (blendState.enableBlending || blendState.enableColorWriting == false)
			sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, fragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, &blending, vertexProgramGxp, &fragmentProgram);
		else
			sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, fragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4, SCE_GXM_MULTISAMPLE_NONE, nullptr, vertexProgramGxp, &fragmentProgram);

		Log::Print(LogLevel::LEVEL_INFO, "Created fragment program\n");
		
		Log::Print(LogLevel::LEVEL_INFO, "V Parameter count: %d\n", sceGxmProgramGetParameterCount(vertexProgramGxp));
		Log::Print(LogLevel::LEVEL_INFO, "F Parameter count: %d\n", sceGxmProgramGetParameterCount(fragmentProgramGxp));

		Log::Print(LogLevel::LEVEL_INFO, "V DEFAULT UNIFORM BUFFER SIZE:  %u\n", sceGxmProgramGetDefaultUniformBufferSize(vertexProgramGxp));
		Log::Print(LogLevel::LEVEL_INFO, "F DEFAULT UNIFORM BUFFER SIZE:  %u\n", sceGxmProgramGetDefaultUniformBufferSize(fragmentProgramGxp));

		modelMatrixParam = sceGxmProgramFindParameterByName(vertexProgramGxp, "modelMatrix");
	}

	GXMShader::~GXMShader()
	{
	}

	void GXMShader::Dispose(SceGxmShaderPatcher *shaderPatcher)
	{
		sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher, vertexProgram);
		sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher, fragmentProgram);

		sceGxmShaderPatcherUnregisterProgram(shaderPatcher, vertexProgramID);
		sceGxmShaderPatcherUnregisterProgram(shaderPatcher, fragmentProgramID);

		Log::Print(LogLevel::LEVEL_INFO, "Disposed shader\n");
	}

	void GXMShader::Use()
	{
	}

	const SceGxmProgramParameter *GXMShader::GetParameter(bool vertexStage, const char *name)
	{
		if (vertexStage)
			return sceGxmProgramFindParameterByName(vertexProgramGxp, name);
		else
			return sceGxmProgramFindParameterByName(fragmentProgramGxp, name);
	}
}
