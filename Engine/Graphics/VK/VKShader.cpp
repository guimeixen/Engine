#include "VKShader.h"

#include "Program\Log.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace Engine
{
	VKShader::VKShader(unsigned int id, const std::string &vertexName, const std::string &fragmentName, const std::string &defines)
	{
		isCompiled = false;
		this->id = id;
		idStr = std::to_string(id);
		this->defines = defines;
		this->vertexName = vertexName;
		this->fragmentName = fragmentName;
		vertexExists = false;
		geometryExists = false;
		fragmentExists = false;
		computeExists = false;
		vertexNeedsCompile = false;
		geometryNeedsCompile = false;
		fragmentNeedsCompile = false;
		vertexModule = VK_NULL_HANDLE;
		geometryModule = VK_NULL_HANDLE;
		fragmentModule = VK_NULL_HANDLE;
		computeModule = VK_NULL_HANDLE;

		//std::ifstream vertexFile(vertexPath, std::ios::ate | std::ios::binary);		// ate -> start reading at the end of the file. Useful for determining the file size
		//std::ifstream fragmentFile(fragmentPath, std::ios::ate | std::ios::binary);

		// We first check if the compiled shader exists
		// If it does, then we check if it was compiled earlier than the base shader and if it was then we mark it as in need of compilation
		// Then we check if the compiled shader does not exist or needs compilation and if it is true then we load the base shader
		// If we have any defines then we insert them in the loaded shader code and save it to a temporary file.
		// When building the pipeline we call Compile() check the necessary flags and execute the program with the corrects paths 
		// The compiled shader files are saved with their id's because they are unique and we can check if a certain permutation has been compiled

		std::string baseVertexPath = "Data/Shaders/Vulkan/src/" + vertexName + ".vert";			// Check if we can reload or not the shaders and use .vert or _vert.spv
		std::string baseFragmentPath = "Data/Shaders/Vulkan/src/" + fragmentName + ".frag";

		// Check if the compiled file already exists so we don't save to do it again
		std::string compiledVertexPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_vert.spv";
		std::string compiledFragmentPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_frag.spv";

		vertexExists = std::filesystem::exists(compiledVertexPath);
		fragmentExists = std::filesystem::exists(compiledFragmentPath);

		if (vertexExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledVertexPath);
			auto baseVertexWriteTime = std::filesystem::last_write_time(baseVertexPath);

			lastVertexWriteTime = baseVertexWriteTime;

			// If the compiled time is older than the when the base shader was modified the we need to compile it again
			if (compiledTime < baseVertexWriteTime)
				vertexNeedsCompile = true;
		}
		if (fragmentExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledFragmentPath);
			auto baseFragmentWriteTime = std::filesystem::last_write_time(baseFragmentPath);

			lastFragmentWriteTime = baseFragmentWriteTime;

			// If the compiled time is older than the when the base shader was modified the we need to compile it again
			if (compiledTime < baseFragmentWriteTime)
				fragmentNeedsCompile = true;
		}

		if (vertexExists == false || vertexNeedsCompile)
			ReadShaderFile(baseVertexPath, ShaderType::VERTEX);

		if (fragmentExists == false || fragmentNeedsCompile)
		{
			ReadShaderFile(baseFragmentPath, ShaderType::FRAGMENT);
			/*std::ifstream fragmentFile(baseFragmentPath);

			if (!fragmentFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << baseFragmentPath.c_str() << "\n";
			}

			std::stringstream buf;
			buf << fragmentFile.rdbuf();
			fragmentCode = buf.str();

			// There was a shader that wasn't reading correctly, it was adding some text at the end of the shader
			/*size_t fragFileSize = (size_t)fragmentFile.tellg();
			fragmentCode.resize(fragFileSize, ' ');
			fragmentFile.seekg(std::ios::beg);
			fragmentFile.read(&fragmentCode[0], fragFileSize);
			fragmentFile.close();*/
		}
		
		if (defines.length() != 0)
		{
			// We load the base shader above and insert the defines here and then save it to a temporary file so it can be loaded and then be compiled
			// But only save the ones with inserted defines, because the ones with no defines are equal to the base shader		
			if (vertexExists == false || vertexNeedsCompile)
				WriteShaderFileWithDefines(ShaderType::VERTEX);

			if (fragmentExists == false || fragmentNeedsCompile)
				WriteShaderFileWithDefines(ShaderType::FRAGMENT);
		}
	}

	VKShader::VKShader(unsigned int id, const std::string &vertexName, const std::string &geometryName, const std::string &fragmentName, const std::string &defines)
	{
		isCompiled = false;
		this->id = id;
		idStr = std::to_string(id);
		this->defines = defines;
		this->vertexName = vertexName;
		this->geometryName = geometryName;
		this->fragmentName = fragmentName;
		vertexExists = false;
		geometryExists = false;
		fragmentExists = false;
		computeExists = false;
		vertexNeedsCompile = false;
		geometryNeedsCompile = false;
		fragmentNeedsCompile = false;
		vertexModule = VK_NULL_HANDLE;
		geometryModule = VK_NULL_HANDLE;
		fragmentModule = VK_NULL_HANDLE;
		computeModule = VK_NULL_HANDLE;

		//std::ifstream vertexFile(vertexPath, std::ios::ate | std::ios::binary);		// ate -> start reading at the end of the file. Useful for determining the file size
		//std::ifstream fragmentFile(fragmentPath, std::ios::ate | std::ios::binary);

		// We first check if the compiled shader exists
		// If it does, then we check if it was compiled earlier than the base shader and if it was then we mark it as in need of compilation
		// Then we check if the compiled shader does not exist or needs compilation and if it is true then we load the base shader
		// If we have any defines then we insert them in the loaded shader code and save it to a temporary file.
		// When building the pipeline we call Compile() check the necessary flags and execute the program with the corrects paths 
		// The compiled shader files are saved with their id's because they are unique and we can check if a certain permutation has been compiled

		std::string baseVertexPath = "Data/Shaders/Vulkan/src/" + vertexName + ".vert";			// Check if we can reload or not the shaders and use .vert or _vert.spv
		std::string baseGeometryPath = "Data/Shaders/Vulkan/src/" + geometryName + ".geom";
		std::string baseFragmentPath = "Data/Shaders/Vulkan/src/" + fragmentName + ".frag";

		// Check if the compiled file already exists so we don't save to do it again
		std::string compiledVertexPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_vert.spv";
		std::string compiledGeometryPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_geom.spv";
		std::string compiledFragmentPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_frag.spv";

		vertexExists = std::filesystem::exists(compiledVertexPath);
		geometryExists = std::filesystem::exists(compiledGeometryPath);
		fragmentExists = std::filesystem::exists(compiledFragmentPath);

		if (vertexExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledVertexPath);
			auto baseVertexWriteTime = std::filesystem::last_write_time(baseVertexPath);

			lastVertexWriteTime = baseVertexWriteTime;

			// If the compiled time is older than the when the base shader was modified the we need to compile it again
			if (compiledTime < baseVertexWriteTime)
				vertexNeedsCompile = true;
		}
		if (geometryExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledGeometryPath);
			auto baseGeometryWriteTime = std::filesystem::last_write_time(baseGeometryPath);

			lastGeometryWriteTime = baseGeometryWriteTime;

			// If the compiled time is older than the when the base shader was modified the we need to compile it again
			if (compiledTime < baseGeometryWriteTime)
				geometryNeedsCompile = true;
		}
		if (fragmentExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledFragmentPath);
			auto baseFragmentWriteTime = std::filesystem::last_write_time(baseFragmentPath);

			lastFragmentWriteTime = baseFragmentWriteTime;

			// If the compiled time is older than the when the base shader was modified the we need to compile it again
			if (compiledTime < baseFragmentWriteTime)
				fragmentNeedsCompile = true;
		}

		if (vertexExists == false || vertexNeedsCompile)
		{
			std::ifstream vertexFile(baseVertexPath, std::ios::ate);		// ate -> start reading at the end of the file. Useful for determining the file size

			if (!vertexFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << baseVertexPath.c_str() << "\n";
			}

			size_t vertFileSize = (size_t)vertexFile.tellg();
			vertexCode.resize(vertFileSize, 0);
			vertexFile.seekg(std::ios::beg);									// Go back the beginning of the file
			vertexFile.read(&vertexCode[0], vertFileSize);		// Now read it all at once
			vertexFile.close();
		}

		if (geometryExists == false || geometryNeedsCompile)
		{
			std::ifstream geometryFile(baseGeometryPath, std::ios::ate);

			if (!geometryFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << baseGeometryPath.c_str() << "\n";
			}

			size_t geomFileSize = (size_t)geometryFile.tellg();
			geometryCode.resize(geomFileSize, 0);
			geometryFile.seekg(std::ios::beg);
			geometryFile.read(&geometryCode[0], geomFileSize);
			geometryFile.close();
		}

		if (fragmentExists == false || fragmentNeedsCompile)
		{
			std::ifstream fragmentFile(baseFragmentPath, std::ios::ate);

			if (!fragmentFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << baseFragmentPath.c_str() << "\n";
			}

			size_t fragFileSize = (size_t)fragmentFile.tellg();
			fragmentCode.resize(fragFileSize, 0);
			fragmentFile.seekg(std::ios::beg);
			fragmentFile.read(&fragmentCode[0], fragFileSize);
			fragmentFile.close();
		}

		if (defines.length() != 0)
		{
			// We load the base shader above and insert the defines and then save it to a temporary file so it can be loaded to be compiled
			// But only save the ones with inserted defines, because the ones with no defines are equal to the base shader

			if (vertexExists == false || vertexNeedsCompile)
			{
				vertexCode.insert(13, defines);

				std::ofstream vertexWithDefinesFile("Data/Shaders/Vulkan/src/" + idStr + ".vert");
				vertexWithDefinesFile << vertexCode;
				vertexWithDefinesFile.close();
			}

			if (geometryExists == false || geometryNeedsCompile)
			{
				geometryCode.insert(13, defines);

				std::ofstream geometryWithDefinesFile("Data/Shaders/Vulkan/src/" + idStr + ".geom");
				geometryWithDefinesFile << geometryCode;
				geometryWithDefinesFile.close();
			}

			if (fragmentExists == false || fragmentNeedsCompile)
			{
				fragmentCode.insert(13, defines);

				std::ofstream fragmentWithDefinesFile("Data/Shaders/Vulkan/src/" + idStr + ".frag");
				fragmentWithDefinesFile << fragmentCode;
				fragmentWithDefinesFile.close();
			}
		}
	}

	VKShader::VKShader(unsigned int id, const std::string &computeName, const std::string &defines)
	{
		isCompiled = false;
		this->id = id;
		idStr = std::to_string(id);
		this->defines = defines;
		this->computeName = computeName;
		vertexExists = false;
		geometryExists = false;
		fragmentExists = false;
		computeExists = false;
		vertexNeedsCompile = false;
		geometryNeedsCompile = false;
		fragmentNeedsCompile = false;
		computeNeedsCompile = false;
		vertexModule = VK_NULL_HANDLE;
		geometryModule = VK_NULL_HANDLE;
		fragmentModule = VK_NULL_HANDLE;
		computeModule = VK_NULL_HANDLE;

		// We first check if the compiled shader exists
		// If it does, then we check if it was compiled earlier than the base shader and if it was then we mark it as in need of compilation
		// Then we check if the compiled shader does not exist or needs compilation and if it is true then we load the base shader
		// If we have any defines then we insert them in the loaded shader code and save it to a temporary file.
		// When building the pipeline we call Compile() check the necessary flags and execute the program with the corrects paths 
		// The compiled shader files are saved with their id's because they are unique and we can check if a certain permutation has been compiled

		std::string baseComputePath = "Data/Shaders/Vulkan/src/" + computeName + ".comp";			// Check if we can reload or not the shaders and use .vert or _vert.spv

		// Check if the compiled file already exists so we don't save to do it again
		std::string compiledComputePath = "Data/Shaders/Vulkan/spirv/" + idStr + "_comp.spv";

		computeExists = std::filesystem::exists(compiledComputePath);

		if (computeExists)
		{
			auto compiledTime = std::filesystem::last_write_time(compiledComputePath);
			auto baseComputeWriteTime = std::filesystem::last_write_time(baseComputePath);

			lastComputeWriteTime = baseComputeWriteTime;

			// If the compiled time is older than when the base shader was modified the we need to compile it again
			if (compiledTime < baseComputeWriteTime)
				computeNeedsCompile = true;
		}

		if (computeExists == false || computeNeedsCompile)
		{
			std::ifstream computeFile(baseComputePath, std::ios::ate);		// ate -> start reading at the end of the file. Useful for determining the file size

			if (!computeFile.is_open())
			{
				std::cout << "Error -> Failed to open file : " << baseComputePath.c_str() << "\n";
			}

			size_t computeFileSize = (size_t)computeFile.tellg();
			computeCode.resize(computeFileSize, 0);
			computeFile.seekg(std::ios::beg);									// Go back the beginning of the file
			computeFile.read(&computeCode[0], computeFileSize);		// Now read it all at once
			computeFile.close();
		}

		if (defines.length() != 0)
		{
			// We load the base shader above and insert the defines and then save it to a temporary file so it can be loaded to be compiled
			// But only save the ones with inserted defines, because the ones with no defines are equal to the base shader

			if (computeExists == false || computeNeedsCompile)
			{
				computeCode.insert(13, defines);

				std::ofstream computeWithDefinesFile("Data/Shaders/Vulkan/src/" + idStr + ".comp");
				computeWithDefinesFile << computeCode;
				computeWithDefinesFile.close();
			}
		}
	}

	void VKShader::Compile(VkDevice device)
	{
		if (isCompiled)
			return;

		// Load the shader with the defines and compile it and then delete the file

		std::string vertexShaderWithDefines;
		std::string geometryShaderWithDefines;
		std::string fragmentShaderWithDefines;
		std::string computeShaderWithDefines;

		if (defines.length() != 0)
		{
			if (vertexName.length() != 0 && (!vertexExists || vertexNeedsCompile))
			{
				vertexShaderWithDefines = "Data/Shaders/Vulkan/src/" + idStr + ".vert";
				CompileShader(vertexShaderWithDefines, ShaderType::VERTEX);
			}
			if (geometryName.length() != 0 && (!geometryExists || geometryNeedsCompile))
			{
				geometryShaderWithDefines = "Data/Shaders/Vulkan/src/" + idStr + ".geom";
				CompileShader(geometryShaderWithDefines, ShaderType::GEOMETRY);
			}
			if (fragmentName.length() != 0 && (!fragmentExists || fragmentNeedsCompile))
			{
				fragmentShaderWithDefines = "Data/Shaders/Vulkan/src/" + idStr + ".frag";
				CompileShader(fragmentShaderWithDefines, ShaderType::FRAGMENT);
			}
			if (computeName.length() != 0 && (!computeExists || computeNeedsCompile))
			{
				computeShaderWithDefines = "Data/Shaders/Vulkan/src/" + idStr + ".comp";
				CompileShader(computeShaderWithDefines, ShaderType::COMPUTE);
			}
		}
		else
		{
			if (vertexName.length() != 0 && (!vertexExists || vertexNeedsCompile))
			{
				vertexShaderWithDefines = "Data/Shaders/Vulkan/src/" + vertexName + ".vert";
				CompileShader(vertexShaderWithDefines, ShaderType::VERTEX);
			}
			if (geometryName.length() != 0 && (!geometryExists || geometryNeedsCompile))
			{
				geometryShaderWithDefines = "Data/Shaders/Vulkan/src/" + geometryName + ".geom";
				CompileShader(geometryShaderWithDefines, ShaderType::GEOMETRY);
			}
			if (fragmentName.length() != 0 && (!fragmentExists || fragmentNeedsCompile))
			{
				fragmentShaderWithDefines = "Data/Shaders/Vulkan/src/" + fragmentName + ".frag";
				CompileShader(fragmentShaderWithDefines, ShaderType::FRAGMENT);
			}
			if (computeName.length() != 0 && (!computeExists || computeNeedsCompile))
			{
				computeShaderWithDefines = "Data/Shaders/Vulkan/src/" + computeName + ".comp";
				CompileShader(computeShaderWithDefines, ShaderType::COMPUTE);
			}
		}
			
		isCompiled = true;
	}

	void VKShader::ReadShaderFile(const std::string& path, ShaderType type)
	{
		std::ifstream file(path, std::ios::ate);

		if (!file.is_open())
		{
			std::cout << "Error -> Failed to open file : " << path.c_str() << "\n";
		}

		size_t fileSize = (size_t)file.tellg();
		file.seekg(std::ios::beg);

		if (type == ShaderType::VERTEX)
		{
			vertexCode.clear();
			vertexCode.resize(fileSize, 0);
			file.read(&vertexCode[0], fileSize);
		}
		else if (type == ShaderType::GEOMETRY)
		{
			geometryCode.clear();
			geometryCode.resize(fileSize, 0);
			file.read(&geometryCode[0], fileSize);
		}
		else if (type == ShaderType::FRAGMENT)
		{
			fragmentCode.clear();
			fragmentCode.resize(fileSize, 0);
			file.read(&fragmentCode[0], fileSize);
		}
		else if (type == ShaderType::COMPUTE)
		{
			computeCode.clear();
			computeCode.resize(fileSize, 0);
			file.read(&computeCode[0], fileSize);
		}
		
		file.close();
	}

	void VKShader::WriteShaderFileWithDefines(ShaderType type)
	{
		if (type == ShaderType::VERTEX)
		{
			std::ofstream fileWithDefines("Data/Shaders/Vulkan/src/" + idStr + ".vert");
			vertexCode.insert(13, defines);
			fileWithDefines << vertexCode;
			fileWithDefines.close();
		}
		else if (type == ShaderType::GEOMETRY)
		{
			std::ofstream fileWithDefines("Data/Shaders/Vulkan/src/" + idStr + ".geom");
			geometryCode.insert(13, defines);
			fileWithDefines << geometryCode;
			fileWithDefines.close();
		}
		else if (type == ShaderType::FRAGMENT)
		{
			std::ofstream fileWithDefines("Data/Shaders/Vulkan/src/" + idStr + ".frag");
			fragmentCode.insert(13, defines);
			fileWithDefines << fragmentCode;
			fileWithDefines.close();
		}
		else if (type == ShaderType::COMPUTE)
		{
			std::ofstream fileWithDefines("Data/Shaders/Vulkan/src/" + idStr + ".comp");
			computeCode.insert(13, defines);
			fileWithDefines << computeCode;
			fileWithDefines.close();
		}			
	}

	void VKShader::CompileShader(const std::string& path, ShaderType type)
	{
		std::string command = "glslangValidator.exe -V ";
		command += path;
		command += " -o Data/Shaders/Vulkan/spirv/" + idStr;

		switch (type)
		{
		case Engine::ShaderType::VERTEX:
			command += "_vert.spv";
			Log::Print(LogLevel::LEVEL_INFO, "Compiling shader: %s.vert\n", vertexName.c_str());
			vertexNeedsCompile = false;
			break;
		case Engine::ShaderType::GEOMETRY:
			command += "_geom.spv";
			Log::Print(LogLevel::LEVEL_INFO, "Compiling shader: %s.geom\n", geometryName.c_str());
			geometryNeedsCompile = false;
			break;
		case Engine::ShaderType::FRAGMENT:
			command += "_frag.spv";
			Log::Print(LogLevel::LEVEL_INFO, "Compiling shader: %s.frag\n", fragmentName.c_str());
			fragmentNeedsCompile = false;
			break;	
		case Engine::ShaderType::COMPUTE:
			command += "_comp.spv";
			Log::Print(LogLevel::LEVEL_INFO, "Compiling shader: %s.comp\n", computeName.c_str());
			computeNeedsCompile = false;
			break;
		}

		if (std::system(command.c_str()) != 0)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Failed to compile shader\n");

			// Fow now if the shader fails to compile, don't remove the generated shader file so we can check what went wrong
			//if (defines.length() != 0)
				//std::remove(path.c_str());

			return;
		}
		if (defines.length() != 0)
			std::remove(path.c_str());
	}

	bool VKShader::CreateShaderModule(VkDevice device)
	{
		if (vertexModule != VK_NULL_HANDLE || fragmentModule != VK_NULL_HANDLE || geometryModule != VK_NULL_HANDLE || computeModule != VK_NULL_HANDLE)
		{
			Log::Print(LogLevel::LEVEL_ERROR, "Attempting to recreate shader module on a valid module!");
			return false;
		}

		if (computeName.length() != 0)
		{
			// Don't load the shader code if we're not using shader reload
			std::string computePath = "Data/Shaders/Vulkan/spirv/" + idStr + "_comp.spv";			// Check if we can reload or not the shaders and use .vert or _vert.spv

			std::ifstream computeFile(computePath, std::ios::ate | std::ios::binary);		// ate -> start reading at the end of the file. Useful for determining the file size

			if (!computeFile.is_open())
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to open compiled shader  %s, file : %s\n", computeName.c_str(), computePath.c_str());
			}

			size_t compFileSize = (size_t)computeFile.tellg();

			std::vector<char> computeCode(compFileSize);

			computeFile.seekg(0);									// Go back the beginning of the file
			computeFile.read(computeCode.data(), compFileSize);		// Now read it all at once

			// Compute module
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = computeCode.size();
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(computeCode.data());

			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &computeModule) != VK_SUCCESS)
			{
				std::cout << "Error -> Failed to create shader module!";
			}
		}
		else
		{
			// Don't load the shader code if we're not using shader reload
			std::string vertexPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_vert.spv";			// Check if we can reload or not the shaders and use .vert or _vert.spv
			std::string fragmentPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_frag.spv";

			std::ifstream vertexFile(vertexPath, std::ios::ate | std::ios::binary);		// ate -> start reading at the end of the file. Useful for determining the file size
			std::ifstream fragmentFile(fragmentPath, std::ios::ate | std::ios::binary);

			if (!vertexFile.is_open())
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to open compiled shader  %s, file : %s\n", vertexName.c_str(), vertexPath.c_str());
			}

			if (!fragmentFile.is_open())
			{
				Log::Print(LogLevel::LEVEL_ERROR, "Error -> Failed to open compiled shader  %s, file : %s\n", fragmentName.c_str(), fragmentPath.c_str());
			}

			size_t vertFileSize = (size_t)vertexFile.tellg();
			size_t fragFileSize = (size_t)fragmentFile.tellg();

			std::vector<char> vertexCode(vertFileSize);
			std::vector<char> fragmentCode(fragFileSize);

			vertexFile.seekg(0);									// Go back the beginning of the file
			vertexFile.read(vertexCode.data(), vertFileSize);		// Now read it all at once
			vertexFile.close();

			fragmentFile.seekg(0);
			fragmentFile.read(fragmentCode.data(), fragFileSize);
			fragmentFile.close();

			// Vertex module
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = vertexCode.size();
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(vertexCode.data());

			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &vertexModule) != VK_SUCCESS)
			{
				std::cout << "Error -> Failed to create shader module!";
			}

			// Fragment module 
			moduleInfo.codeSize = fragmentCode.size();
			moduleInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentCode.data());

			if (vkCreateShaderModule(device, &moduleInfo, nullptr, &fragmentModule) != VK_SUCCESS)
			{
				std::cout << "Error -> Failed to create shader module!";
			}


			// Geometry module
			if (geometryName.length() != 0)
			{
				// Don't load the shader code if we're not using shader reload
				std::string geometryPath = "Data/Shaders/Vulkan/spirv/" + idStr + "_geom.spv";			// Check if we can reload or not the shaders and use .vert or _vert.spv

				std::ifstream geometryFile(geometryPath, std::ios::ate | std::ios::binary);		// ate -> start reading at the end of the file. Useful for determining the file size

				if (!geometryFile.is_open())
				{
					std::cout << "Error -> Failed to open file : " << geometryPath.c_str() << "\n";
				}

				size_t geomFileSize = (size_t)geometryFile.tellg();

				std::vector<char> geometryCode(geomFileSize);

				geometryFile.seekg(0);									// Go back the beginning of the file
				geometryFile.read(geometryCode.data(), geomFileSize);		// Now read it all at once
				geometryFile.close();

				// Vertex module
				VkShaderModuleCreateInfo moduleInfo = {};
				moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				moduleInfo.codeSize = geometryCode.size();
				moduleInfo.pCode = reinterpret_cast<const uint32_t*>(geometryCode.data());

				if (vkCreateShaderModule(device, &moduleInfo, nullptr, &geometryModule) != VK_SUCCESS)
				{
					std::cout << "Error -> Failed to create shader module!";
				}
			}
		}

		return true;
	}

	void VKShader::CheckIfModifiedAndReload()
	{
		if (computeName.size() > 0)
		{
			std::string baseComputePath = "Data/Shaders/Vulkan/src/" + computeName + ".comp";

			auto baseComputeWriteTime = std::filesystem::last_write_time(baseComputePath);

			if (baseComputeWriteTime > lastComputeWriteTime)
			{
				lastComputeWriteTime = baseComputeWriteTime;
				computeNeedsCompile = true;
				isCompiled = false;

				ReadShaderFile(baseComputePath, ShaderType::COMPUTE);

				if (defines.length() != 0)
					WriteShaderFileWithDefines(ShaderType::COMPUTE);
			}
		}
		else
		{
			std::string baseVertexPath = "Data/Shaders/Vulkan/src/" + vertexName + ".vert";
			std::string baseFragmentPath = "Data/Shaders/Vulkan/src/" + fragmentName + ".frag";

			auto baseVertexWriteTime = std::filesystem::last_write_time(baseVertexPath);
			auto baseFragmentWriteTime = std::filesystem::last_write_time(baseFragmentPath);

			if (baseVertexWriteTime > lastVertexWriteTime)
			{
				lastVertexWriteTime = baseVertexWriteTime;
				vertexNeedsCompile = true;
				isCompiled = false;

				ReadShaderFile(baseVertexPath, ShaderType::VERTEX);

				if (defines.length() != 0)
					WriteShaderFileWithDefines(ShaderType::VERTEX);
			}

			if (baseFragmentWriteTime > lastFragmentWriteTime)
			{
				lastFragmentWriteTime = baseFragmentWriteTime;
				fragmentNeedsCompile = true;
				isCompiled = false;

				ReadShaderFile(baseFragmentPath, ShaderType::FRAGMENT);

				if (defines.length() != 0)
					WriteShaderFileWithDefines(ShaderType::FRAGMENT);
			}

			if (geometryName.size() > 0)
			{
				std::string baseGeometryPath = "Data/Shaders/Vulkan/src/" + geometryName + ".geom";

				auto baseGeometryWriteTime = std::filesystem::last_write_time(baseGeometryPath);

				if (baseGeometryWriteTime > lastGeometryWriteTime)
				{
					lastGeometryWriteTime = baseGeometryWriteTime;
					geometryNeedsCompile = true;
					isCompiled = false;

					ReadShaderFile(baseGeometryPath, ShaderType::GEOMETRY);

					if (defines.length() != 0)
						WriteShaderFileWithDefines(ShaderType::GEOMETRY);
				}
			}
		}
	}

	void VKShader::Dispose(VkDevice device)
	{
		if (vertexModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, vertexModule, nullptr);
		if (geometryModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, geometryModule, nullptr);
		if (fragmentModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, fragmentModule, nullptr);
		if (computeModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(device, computeModule, nullptr);

		vertexModule = VK_NULL_HANDLE;
		geometryModule = VK_NULL_HANDLE;
		fragmentModule = VK_NULL_HANDLE;
		computeModule = VK_NULL_HANDLE;
	}

	VkPipelineShaderStageCreateInfo VKShader::GetVertexStageInfo()
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexModule;
		vertShaderStageInfo.pName = "main";

		return vertShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo VKShader::GetGeometryStageInfo()
	{
		VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = geometryModule;
		geomShaderStageInfo.pName = "main";

		return geomShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo VKShader::GetFragmentStageInfo()
	{
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentModule;
		fragShaderStageInfo.pName = "main";

		return fragShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo VKShader::GetComputeStageInfo()
	{
		VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
		compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compShaderStageInfo.module = computeModule;
		compShaderStageInfo.pName = "main";

		return compShaderStageInfo;
	}
}
