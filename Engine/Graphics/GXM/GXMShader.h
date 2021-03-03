#pragma once

#include "Graphics/Shader.h"
#include "Program/FileManager.h"
#include "Graphics/VertexTypes.h"

#include "psp2/gxm.h"

namespace Engine
{
	class GXMShader : public ShaderProgram
	{
	public:
		GXMShader(SceGxmShaderPatcher *shaderPatcher, FileManager *fileManager, const std::string &vertexName, const std::string &fragmentName, const std::vector<VertexInputDesc> &descs, const BlendState &blendState);
		~GXMShader();

		void Dispose(SceGxmShaderPatcher *shaderPatcher);
		bool CheckIfModified() override;
		void Reload() override;
		void Use();

		const SceGxmProgramParameter *GetParameter(bool vertexStage, const char *name);
		const SceGxmProgramParameter *GetModelMatrixParam() const { return modelMatrixParam; }

		SceGxmVertexProgram *GetVertexProgram() const { return vertexProgram; }
		SceGxmFragmentProgram *GetFragmentProgram() const { return fragmentProgram; }

		const SceGxmProgram *GetVertexProgramGxp() const { return vertexProgramGxp; }
		const SceGxmProgram *GetFragmentProgramGxp() const { return fragmentProgramGxp; }

		SceGxmShaderPatcherId GetVertexID() const { return vertexProgramID; }
		SceGxmShaderPatcherId GetFragmentID() const { return fragmentProgramID; }

	private:
		SceGxmShaderPatcherId vertexProgramID;
		SceGxmShaderPatcherId fragmentProgramID;
		SceGxmVertexProgram *vertexProgram;
		SceGxmFragmentProgram *fragmentProgram;

		const SceGxmProgram *vertexProgramGxp;
		const SceGxmProgram *fragmentProgramGxp;

		const SceGxmProgramParameter *modelMatrixParam;
	};
}
