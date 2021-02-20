#include "D3D11Shader.h"

#include <d3dcompiler.h>

#include <iostream>
#include <fstream>

namespace Engine
{
	D3D11Shader::D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &vertexName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		ID3DBlob *vsBlob = nullptr;
		ID3DBlob *psBlob = nullptr;

		// Compile the vertex and pixel shaders

		std::string vertexPath = "Data/Shaders/D3D/" + vertexName + ".vert";
		std::string fragmentPath = "Data/Shaders/D3D/" + fragmentName + ".frag";
		std::string idStr = std::to_string(id);
		std::string vertexPathWithID = "Data/Shaders/D3D/" + idStr + ".vert";
		std::string fragmentPathWithID = "Data/Shaders/D3D/" + idStr + ".frag";

		// If we have defines then we load the shader files and insert the defines in them
		if (defines.length() != 0)
		{
			std::ifstream vertexFile(vertexPath, std::ios::ate);		// ate -> start reading at the end of the file. Useful for determining the file size

			if (!vertexFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << vertexPath.c_str() << "\n";
				return;
			}

			std::ifstream fragmentFile(fragmentPath, std::ios::ate);

			if (!fragmentFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << fragmentPath.c_str() << "\n";
				return;
			}

			std::string vertexCode;

			size_t vertFileSize = (size_t)vertexFile.tellg();
			vertexCode.resize(vertFileSize, 0);
			vertexFile.seekg(std::ios::beg);									// Go back the beginning of the file
			vertexFile.read(&vertexCode[0], vertFileSize);		// Now read it all at once
			vertexFile.close();



			std::string fragmentCode;

			size_t fragFileSize = (size_t)fragmentFile.tellg();
			fragmentCode.resize(fragFileSize, 0);
			fragmentFile.seekg(std::ios::beg);
			fragmentFile.read(&fragmentCode[0], fragFileSize);
			fragmentFile.close();

			vertexCode.insert(0, defines);
			fragmentCode.insert(0, defines);



			// We save the files and compile from file instead of compiling with string with the source because we would have a problem with the #include
			std::ofstream vertexWithDefinesFile(vertexPathWithID);
			vertexWithDefinesFile << vertexCode;
			vertexWithDefinesFile.close();

			std::ofstream fragmentWithDefinesFile(fragmentPathWithID);
			fragmentWithDefinesFile << fragmentCode;
			fragmentWithDefinesFile.close();

			std::wstring vp(vertexPathWithID.begin(), vertexPathWithID.end());
			HRESULT hr = CompileShaderFromFile(vp.c_str(), "VS", "vs_5_0", &vsBlob);
			if (FAILED(hr))
			{
				std::remove(vertexPathWithID.c_str());
				throw std::runtime_error("Failed to compile vertex shader");
			}
			std::remove(vertexPathWithID.c_str());


			std::wstring fp(fragmentPathWithID.begin(), fragmentPathWithID.end());
			hr = CompileShaderFromFile(fp.c_str(), "PS", "ps_5_0", &psBlob);
			if (FAILED(hr))
			{
				std::remove(fragmentPathWithID.c_str());
				throw std::runtime_error("Failed to compile fragment shader");
			}
			std::remove(fragmentPathWithID.c_str());
		}
		else
		{
			// If we don't have any defines then compile the base shader

			std::wstring vp(vertexPath.begin(), vertexPath.end());
			HRESULT hr = CompileShaderFromFile(vp.c_str(), "VS", "vs_5_0", &vsBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile vertex shader");
			}

			std::wstring fp(fragmentPath.begin(), fragmentPath.end());
			hr = CompileShaderFromFile(fp.c_str(), "PS", "ps_5_0", &psBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile fragment shader");
			}
		}


		// Create the vertex and pixel shaders
		HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
		if (FAILED(hr))
		{
			vsBlob->Release();
			throw std::runtime_error("Failed to create vertex shader");
		}
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
		if (FAILED(hr))
		{
			psBlob->Release();
			throw std::runtime_error("Failed to create fragment shader");
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDesc;
		int index = 0;

		for (size_t i = 0; i < descs.size(); i++)
		{
			const VertexInputDesc &desc = descs[i];

			for (size_t j = 0; j < desc.attribs.size(); j++)
			{
				const VertexAttribute &attrib = desc.attribs[j];

				D3D11_INPUT_ELEMENT_DESC d3ddesc = {};

				if (index == 0)
					d3ddesc.SemanticName = "POSITION";
				else if (index == 1)
					d3ddesc.SemanticName = "NORMAL";
				else if (index == 2)
					d3ddesc.SemanticName = "TEXCOORD";
				else if (index == 3)
					d3ddesc.SemanticName = "TANGENT";
				else if (index == 4)
					d3ddesc.SemanticName = "BINORMAL";
				else if (index == 5)
					d3ddesc.SemanticName = "COLOR";
				else if (index == 6)
					d3ddesc.SemanticName = "FOG";

				d3ddesc.SemanticIndex = 0;

				if (attrib.count == 4 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				else if (attrib.count == 3 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				else if (attrib.count == 2 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32_FLOAT;
				else if (attrib.count == 1 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32_FLOAT;
				else if (attrib.count == 4 && attrib.vertexAttribFormat == VertexAttributeFormat::INT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;

				d3ddesc.InputSlot = static_cast<UINT>(i);
				d3ddesc.AlignedByteOffset = attrib.offset;

				if (desc.instanced)
				{
					d3ddesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					d3ddesc.InstanceDataStepRate = 1;
				}
				else
				{
					d3ddesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					d3ddesc.InstanceDataStepRate = 0;
				}

				index++;

				layoutDesc.push_back(d3ddesc);
			}
		}
		

		hr = device->CreateInputLayout(layoutDesc.data(), static_cast<unsigned int>(layoutDesc.size()), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

		if (FAILED(hr))
		{
			std::cout << "Failed to create vertex input layout!\n";
			return;
		}
	}

	D3D11Shader::D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName, const std::string &defines, const std::vector<VertexInputDesc> &descs)
	{
		ID3DBlob *vsBlob = nullptr;
		ID3DBlob *psBlob = nullptr;
		ID3DBlob *gsBlob = nullptr;

		// Compile the vertex and pixel shaders

		std::string vertexPath = "Data/Shaders/D3D/" + vertexName + ".vert";
		std::string fragmentPath = "Data/Shaders/D3D/" + fragmentName + ".frag";
		std::string geometryPath = "Data/Shaders/D3D/" + geometryName + ".geom";
		std::string idStr = std::to_string(id);
		std::string vertexPathWithID = "Data/Shaders/D3D/" + idStr + ".vert";
		std::string fragmentPathWithID = "Data/Shaders/D3D/" + idStr + ".frag";
		std::string geometryPathWithID = "Data/Shaders/D3D/" + idStr + ".geom";

		// If we have defines then we load the shader files and insert the defines in them
		if (defines.length() != 0)
		{
			std::ifstream vertexFile(vertexPath, std::ios::ate);		// ate -> start reading at the end of the file. Useful for determining the file size

			if (!vertexFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << vertexPath.c_str() << "\n";
				return;
			}

			std::ifstream fragmentFile(fragmentPath, std::ios::ate);

			if (!fragmentFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << fragmentPath.c_str() << "\n";
				return;
			}

			std::ifstream geometryFile(geometryPath, std::ios::ate);

			if (!geometryFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << geometryPath.c_str() << "\n";
				return;
			}

			std::string vertexCode;

			size_t vertFileSize = (size_t)vertexFile.tellg();
			vertexCode.resize(vertFileSize, 0);
			vertexFile.seekg(std::ios::beg);									// Go back the beginning of the file
			vertexFile.read(&vertexCode[0], vertFileSize);		// Now read it all at once
			vertexFile.close();



			std::string fragmentCode;

			size_t fragFileSize = (size_t)fragmentFile.tellg();
			fragmentCode.resize(fragFileSize, 0);
			fragmentFile.seekg(std::ios::beg);
			fragmentFile.read(&fragmentCode[0], fragFileSize);
			fragmentFile.close();


			std::string geometryCode;

			size_t geomFileSize = (size_t)geometryFile.tellg();
			geometryCode.resize(geomFileSize, 0);
			geometryFile.seekg(std::ios::beg);
			geometryFile.read(&geometryCode[0], geomFileSize);
			geometryFile.close();

			vertexCode.insert(0, defines);
			fragmentCode.insert(0, defines);
			geometryCode.insert(0, defines);



			// We save the files and compile from file instead of compiling with string with the source because we would have a problem with the #include
			std::ofstream vertexWithDefinesFile(vertexPathWithID);
			vertexWithDefinesFile << vertexCode;
			vertexWithDefinesFile.close();

			std::ofstream fragmentWithDefinesFile(fragmentPathWithID);
			fragmentWithDefinesFile << fragmentCode;
			fragmentWithDefinesFile.close();

			std::ofstream geometryWithDefinesFile(geometryPathWithID);
			geometryWithDefinesFile << geometryCode;
			geometryWithDefinesFile.close();

			std::wstring vp(vertexPathWithID.begin(), vertexPathWithID.end());
			HRESULT hr = CompileShaderFromFile(vp.c_str(), "VS", "vs_5_0", &vsBlob);
			if (FAILED(hr))
			{
				std::remove(vertexPathWithID.c_str());
				throw std::runtime_error("Failed to compile vertex shader");
			}
			std::remove(vertexPathWithID.c_str());


			std::wstring fp(fragmentPathWithID.begin(), fragmentPathWithID.end());
			hr = CompileShaderFromFile(fp.c_str(), "PS", "ps_5_0", &psBlob);
			if (FAILED(hr))
			{
				std::remove(fragmentPathWithID.c_str());
				throw std::runtime_error("Failed to compile fragment shader");
			}
			std::remove(fragmentPathWithID.c_str());

			std::wstring gp(geometryPathWithID.begin(), geometryPathWithID.end());
			hr = CompileShaderFromFile(gp.c_str(), "GS", "gs_5_0", &gsBlob);
			if (FAILED(hr))
			{
				std::remove(geometryPathWithID.c_str());
				throw std::runtime_error("Failed to compile geometry shader");
			}
			std::remove(geometryPathWithID.c_str());
		}
		else
		{
			// If we don't have any defines then compile the base shader

			std::wstring vp(vertexPath.begin(), vertexPath.end());
			HRESULT hr = CompileShaderFromFile(vp.c_str(), "VS", "vs_5_0", &vsBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile vertex shader");
			}

			std::wstring fp(fragmentPath.begin(), fragmentPath.end());
			hr = CompileShaderFromFile(fp.c_str(), "PS", "ps_5_0", &psBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile fragment shader");
			}

			std::wstring gp(geometryPath.begin(), geometryPath.end());
			hr = CompileShaderFromFile(gp.c_str(), "GS", "gs_5_0", &gsBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile geometry shader");
			}
		}


		// Create the vertex and pixel shaders
		HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
		if (FAILED(hr))
		{
			vsBlob->Release();
			throw std::runtime_error("Failed to create vertex shader");
		}
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
		if (FAILED(hr))
		{
			psBlob->Release();
			throw std::runtime_error("Failed to create fragment shader");
		}
		hr = device->CreateGeometryShader(gsBlob->GetBufferPointer(), gsBlob->GetBufferSize(), nullptr, &geometryShader);
		if (FAILED(hr))
		{
			gsBlob->Release();
			throw std::runtime_error("Failed to create geometry shader");
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> layoutDesc;
		int index = 0;

		for (size_t i = 0; i < descs.size(); i++)
		{
			const VertexInputDesc &desc = descs[i];

			for (size_t j = 0; j < desc.attribs.size(); j++)
			{
				const VertexAttribute &attrib = desc.attribs[j];

				D3D11_INPUT_ELEMENT_DESC d3ddesc = {};

				if (index == 0)
					d3ddesc.SemanticName = "POSITION";
				else if (index == 1)
					d3ddesc.SemanticName = "NORMAL";
				else if (index == 2)
					d3ddesc.SemanticName = "TEXCOORD";
				else if (index == 3)
					d3ddesc.SemanticName = "TANGENT";
				else if (index == 4)
					d3ddesc.SemanticName = "BINORMAL";
				else if (index == 5)
					d3ddesc.SemanticName = "COLOR";
				else if (index == 6)
					d3ddesc.SemanticName = "FOG";

				d3ddesc.SemanticIndex = 0;

				if (attrib.count == 4 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				else if (attrib.count == 3 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				else if (attrib.count == 2 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32G32_FLOAT;
				else if (attrib.count == 1 && attrib.vertexAttribFormat == VertexAttributeFormat::FLOAT)
					d3ddesc.Format = DXGI_FORMAT_R32_FLOAT;
				else if (attrib.count == 4 && attrib.vertexAttribFormat == VertexAttributeFormat::INT)
					d3ddesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;

				d3ddesc.InputSlot = static_cast<UINT>(i);
				d3ddesc.AlignedByteOffset = attrib.offset;

				if (desc.instanced)
				{
					d3ddesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					d3ddesc.InstanceDataStepRate = 1;
				}
				else
				{
					d3ddesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					d3ddesc.InstanceDataStepRate = 0;
				}

				index++;

				layoutDesc.push_back(d3ddesc);
			}
		}


		hr = device->CreateInputLayout(layoutDesc.data(), static_cast<unsigned int>(layoutDesc.size()), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

		if (FAILED(hr))
		{
			std::cout << "Failed to create vertex input layout!\n";
			return;
		}
	}

	D3D11Shader::D3D11Shader(ID3D11Device *device, unsigned int id, const std::string &computeName, const std::string &defines)
	{
		ID3DBlob *csBlob = nullptr;
		inputLayout = nullptr;

		// Compile the vertex and pixel shaders

		std::string computePath = "Data/Shaders/D3D/" + computeName + ".comp";
		std::string idStr = std::to_string(id);
		std::string computePathWithID = "Data/Shaders/D3D/" + idStr + ".comp";

		// If we have defines then we load the shader files and insert the defines in them
		if (defines.length() != 0)
		{
			std::ifstream computeFile(computePath, std::ios::ate);

			if (!computeFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << computePath.c_str() << "\n";
				return;
			}

			std::string computeCode;

			size_t compFileSize = (size_t)computeFile.tellg();
			computeCode.resize(compFileSize, 0);
			computeFile.seekg(std::ios::beg);									// Go back the beginning of the file
			computeFile.read(&computeCode[0], compFileSize);		// Now read it all at once
			computeFile.close();


			computeCode.insert(0, defines);


			// We save the files and compile from file instead of compiling with string with the source because we would have a problem with the #include
			std::ofstream computeWithDefinesFile(computePathWithID);
			computeWithDefinesFile << computeCode;
			computeWithDefinesFile.close();


			std::wstring cp(computePathWithID.begin(), computePathWithID.end());
			HRESULT hr = CompileShaderFromFile(cp.c_str(), "CS", "cs_5_0", &csBlob);
			if (FAILED(hr))
			{
				std::remove(computePathWithID.c_str());
				throw std::runtime_error("Failed to compile compute shader");
			}
			std::remove(computePathWithID.c_str());
		}
		else
		{
			// If we don't have any defines then compile the base shader

			std::wstring cp(computePath.begin(), computePath.end());
			HRESULT hr = CompileShaderFromFile(cp.c_str(), "CS", "cs_5_0", &csBlob);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to compile compute shader");
			}
		}


		// Create the compute shader
		HRESULT hr = device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &computeShader);
		if (FAILED(hr))
		{
			csBlob->Release();
			throw std::runtime_error("Failed to create compute shader");
		}
	}

	D3D11Shader::~D3D11Shader()
	{
		if (vertexShader)
			vertexShader->Release();
		if (geometryShader)
			geometryShader->Release();
		if (pixelShader)
			pixelShader->Release();
		if (computeShader)
			computeShader->Release();
		if (inputLayout)
			inputLayout->Release();
	}

	void D3D11Shader::Bind(ID3D11DeviceContext *context)
	{
		context->IASetInputLayout(inputLayout);
		context->VSSetShader(vertexShader, nullptr, 0);
		if (geometryShader)
			context->GSSetShader(geometryShader, nullptr, 0);
		context->PSSetShader(pixelShader, nullptr, 0);
		
	}

	void D3D11Shader::BindCompute(ID3D11DeviceContext *context)
	{
		context->CSSetShader(computeShader, nullptr, 0);
	}

	void D3D11Shader::UnbindGeometry(ID3D11DeviceContext *context)
	{
		if (geometryShader)
			context->GSSetShader(nullptr, nullptr, 0);
	}

	void D3D11Shader::Reload()
	{
	}

	bool D3D11Shader::CheckIfModified()
	{
		return false;
	}

	HRESULT D3D11Shader::CompileShaderFromFile(const wchar_t *path, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob **blobOut)
	{
		HRESULT hr = S_OK;
		DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		shaderFlags |= D3DCOMPILE_DEBUG;

		// Disable optimizations to further improve shader debugging
		//shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		// We could also load prebuilt .cso files instead
		ID3DBlob *errorBlob = nullptr;
		hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel, shaderFlags, 0, blobOut, &errorBlob);

		if (FAILED(hr))
		{
			if (errorBlob)
			{
				std::cout << reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) << '\n';
				errorBlob->Release();
			}
			return hr;
		}

		if (errorBlob)
			errorBlob->Release();

		return S_OK;
	}

	HRESULT D3D11Shader::CompileShader(const void *shaderCode, size_t shaderCodeSize, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob **blobOut)
	{
		HRESULT hr = S_OK;
		DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		shaderFlags |= D3DCOMPILE_DEBUG;

		// Disable optimizations to further improve shader debugging
		//shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		// We could also load prebuilt .cso files instead
		ID3DBlob *errorBlob = nullptr;
	
		hr = D3DCompile(shaderCode, (SIZE_T)shaderCodeSize, nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, shaderModel, shaderFlags, 0, blobOut, &errorBlob);

		if (FAILED(hr))
		{
			if (errorBlob)
			{
				std::cout << reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) << '\n';
				errorBlob->Release();
			}
			return hr;
		}

		if (errorBlob)
			errorBlob->Release();

		return S_OK;
	}
}
