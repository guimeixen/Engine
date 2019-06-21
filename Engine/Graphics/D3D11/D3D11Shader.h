#pragma once

#include "Graphics\Shader.h"
#include "Graphics\VertexTypes.h"

#include <d3d11_1.h>

#include <string>

namespace Engine
{
	class D3D11Shader : public Shader
	{
	public:
		D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs);
		D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs);
		D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &computeName, const std::string &defines);
		~D3D11Shader();

		void Bind(ID3D11DeviceContext *context);
		void BindCompute(ID3D11DeviceContext *context);
		void UnbindGeometry(ID3D11DeviceContext *context);

		bool HasGeometry() const { return geometryShader != nullptr; }
		bool HasCompute() const { return computeShader != nullptr; }

	private:
		HRESULT CompileShaderFromFile(const wchar_t *path, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob **blobOut);
		HRESULT CompileShader(const void *shaderCode, size_t shaderCodeSize, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob **blobOut);

	private:
		ID3D11VertexShader *vertexShader = nullptr;
		ID3D11GeometryShader *geometryShader = nullptr;
		ID3D11PixelShader *pixelShader = nullptr;
		ID3D11ComputeShader *computeShader = nullptr;
		ID3D11InputLayout* inputLayout;
	};
}
