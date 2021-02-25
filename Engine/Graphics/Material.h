#pragma once

#include "MaterialInfo.h"
#include "Game/ComponentManagers/ScriptManager.h"
#include "RendererStructs.h"
#include "Buffers.h"
#include "Texture.h"

#include <string>
#include <map>
#include <vector>

namespace Engine
{
	class Shader;
	class Material;
	class Renderer;

	enum class MaterialParameterType
	{
		FLOAT,
		INT,
		VEC2,
		VEC4,
		COLOR3,
		COLOR4
	};

	struct MaterialParameter
	{
		MaterialParameterType type;
		unsigned int offset;
		unsigned int size;
	};

	struct MaterialInstance
	{
		std::string path;
		char name[64];
		Material *baseMaterial;
		std::vector<Texture*> textures;
		std::vector<Buffer*> buffers;
		unsigned int graphicsSetID;
		unsigned int computeSetID;
		unsigned int computePipelineLayoutIdx;

		std::vector<MaterialParameter> parameters;
		unsigned int lastParamOffset;
		char materialData[128];

		void AddParameter(MaterialParameterType type);
		void SetParameterValue(const MaterialParameter &param, const void *value);
		const std::vector<MaterialParameter> &GetMaterialParameters() const { return parameters; }
		const void *GetMaterialData() const { return materialData; }
	};

	struct TextureInfo
	{
		std::string name;
		TextureType type;
		TextureParams params;
		bool useAlpha;
		bool storeData;
	};

	struct BufferInfo
	{
		BufferType type;
		bool readOnly;
	};

	class Material
	{
	public:
		Material(Renderer *renderer, const std::string &matPath, const std::string &defines, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs);

		static MaterialInstance *LoadMaterialInstance(Renderer *renderer, const std::string &path, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs);
		static MaterialInstance *LoadMaterialInstanceFromBaseMat(Renderer *renderer, const std::string &baseMatPath, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &inputDescs);
		static unsigned int SetOptions(unsigned int options, const std::string &define);

		const std::string &GetName() const { return name; }
		const std::string &GetPath() const { return path; }
		void SetShowInEditor(bool show) { showInEditor = show; }
		bool ShowInEditor() const { return showInEditor; }

		const ShaderPass &GetShaderPass(unsigned int index) const;
		std::vector<ShaderPass> &GetShaderPasses() { return shaderPasses; }
		unsigned int GetShaderPassIndex(const std::string &passName) const;

		//unsigned int GetMeshParamsSize() const { return meshParamsSize; }

		unsigned int GetTextureCount() const { return static_cast<unsigned int>(texturesInfo.size()); }
		const std::vector<TextureInfo> &GetTexturesInfo() const { return texturesInfo; }

	private:
		BlendFactor CompareBlendString(const std::string &str);
		TextureWrap TextureWrapFromString(const std::string &str);
		TextureInternalFormat TextureInternalFormatFromString(const std::string &str);
		TextureFilter TextureFilterFromString(const std::string &str);
		Topology CompareTopologyString(const std::string &str);

	private:
		std::string name;
		std::string path;
		bool showInEditor;

		//unsigned int meshParamsSize;
		std::vector<unsigned int> passIDs;
		std::vector<ShaderPass> shaderPasses;
		std::vector<TextureInfo> texturesInfo;
		std::vector<BufferInfo> buffersInfo;
		std::vector<VertexInputDesc> inputDescs;
	};
}
