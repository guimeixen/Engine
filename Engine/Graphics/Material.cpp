#include "Material.h"

#include "Shader.h"
#include "ResourcesLoader.h"
#include "Renderer.h"

#include "Program/FileManager.h"
#include "Program/StringID.h"
#include "Program/Log.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cstdio>

namespace Engine
{
	Material::Material(Renderer *renderer, const std::string &matPath, const std::string &defines, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Loading new material: %s\n", matPath.c_str());

		showInEditor = true;
		path = matPath;

		inputDescs = descs;

		name = matPath.substr(matPath.find_last_of('/') + 1);
		// Remove the extension
		name.pop_back();
		name.pop_back();
		name.pop_back();
		name.pop_back();

		// Load the lua material file
		scriptManager.ExecuteFile(matPath);

		lua_State *L = scriptManager.GetLuaState();
		luabridge::LuaRef matTable = luabridge::getGlobal(L, name.c_str());

		Log::Print(LogLevel::LEVEL_INFO, "Loading passes table\n");
		Log::Print(LogLevel::LEVEL_INFO, "\t%s\n", matTable.tostring().c_str());
		// Load passes table
		luabridge::LuaRef passesTable = matTable["passes"];
		//Log::Print(LogLevel::LEVEL_INFO, "Loaded table\n");
		if (!passesTable.isNil())
		{
			//Log::Print(LogLevel::LEVEL_INFO, "Loading passes\n");
			std::unordered_map<std::string, luabridge::LuaRef> values = scriptManager.GetKeyValueMap(passesTable);

			// Loop over every pass of this material
			for (auto &pair : values)
			{
				if (pair.second.isTable())
				{
					ShaderPass pass = {};
					pass.blendState = { false, Renderer::GetBlendFactorValue(SRC_ALPHA), Renderer::GetBlendFactorValue(ONE_MINUS_SRC_ALPHA), Renderer::GetBlendFactorValue(SRC_ALPHA), Renderer::GetBlendFactorValue(ONE_MINUS_SRC_ALPHA), true };			
					pass.pipelineID = std::numeric_limits<unsigned int>::max();
					pass.rasterizerState.enableCulling = true;
					pass.rasterizerState.frontFace = Renderer::GetFrontFace("ccw");
					pass.rasterizerState.cullFace = Renderer::GetCullMode("back");
					pass.depthStencilState.depthEnable = true;
					pass.depthStencilState.depthWrite = true;
					pass.depthStencilState.depthFunc = Renderer::GetDepthFunc("less");
					pass.topology = Renderer::GetTopologyValue(Topology::TRIANGLES);
					pass.queueID = 0;
					pass.isCompute = false;

					std::string shaderName;
					std::string vertexName;
					std::string geometryName;
					std::string fragmentName;

					if (descs.size() > 0)
					{
						pass.vertexInputDescs.resize(descs.size());
						for (size_t i = 0; i < pass.vertexInputDescs.size(); i++)
						{
							pass.vertexInputDescs[i] = descs[i];
						}
					}

					pass.id = SID(pair.first);

					Log::Print(LogLevel::LEVEL_INFO, "Pass id: %u\n", pass.id);

					luabridge::LuaRef ref = pair.second["queue"];
					if (ref.isString())
						pass.queueID = SID(ref.cast<std::string>());
					else
						pass.queueID = pass.id;				// If a queue was not provided, use the passID as the id.

					ref = pair.second["shader"];

					if (ref.isString())
					{
						shaderName = ref.cast<std::string>();
					}
					else
					{
						luabridge::LuaRef ref = pair.second["vertex"];
						if (ref.isString())
							vertexName = ref.cast<std::string>();

						ref = pair.second["geometry"];
						if (ref.isString())
							geometryName = ref.cast<std::string>();

						ref = pair.second["fragment"];
						if (ref.isString())
							fragmentName = ref.cast<std::string>();

						// Try with compute shader
						if (vertexName.length() == 0 || fragmentName.length() == 0)
						{
							ref = pair.second["computeShader"];
							if (ref.isString())
							{
								shaderName = ref.cast<std::string>();
								pass.isCompute = true;
							}
							else
							{
								std::cout << "Error in pass name on material : " << matPath << '\n';
								return;
							}
						}
					}

					ref = pair.second["topology"];
					if (ref.isString())
						pass.topology = Renderer::GetTopologyValue(CompareTopologyString(ref.cast<std::string>()));

					ref = pair.second["blending"];
					if (ref.isBoolean())
						pass.blendState.enableBlending = ref.cast<bool>();

					ref = pair.second["srcBlendColor"];
					if (ref.isString())
						pass.blendState.srcColorFactor = Renderer::GetBlendFactorValue(CompareBlendString(ref.cast<std::string>()));

					ref = pair.second["srcBlendAlpha"];
					if (ref.isString())
						pass.blendState.srcAlphaFactor = Renderer::GetBlendFactorValue(CompareBlendString(ref.cast<std::string>()));

					ref = pair.second["dstBlendColor"];
					if (ref.isString())
						pass.blendState.dstColorFactor = Renderer::GetBlendFactorValue(CompareBlendString(ref.cast<std::string>()));

					ref = pair.second["dstBlendAlpha"];
					if (ref.isString())
						pass.blendState.dstAlphaFactor = Renderer::GetBlendFactorValue(CompareBlendString(ref.cast<std::string>()));
					
					ref = pair.second["enableColorWriting"];
					if (ref.isBoolean())
						pass.blendState.enableColorWriting = ref.cast<bool>();

					ref = pair.second["depthTest"];
					if (ref.isBoolean())
						pass.depthStencilState.depthEnable = ref.cast<bool>();

					ref = pair.second["depthWrite"];
					if (ref.isBoolean())
						pass.depthStencilState.depthWrite = ref.cast<bool>();

					ref = pair.second["depthFunc"];
					if (ref.isString())
					{
						std::string s = ref.cast<std::string>();

						pass.depthStencilState.depthFunc = Renderer::GetDepthFunc(s);
					}

					ref = pair.second["cullface"];
					if (ref.isString())
					{
						std::string s = ref.cast<std::string>();

						if (s == "none")
							pass.rasterizerState.enableCulling = false;

						pass.rasterizerState.cullFace = Renderer::GetCullMode(s);
					}

					ref = pair.second["frontFace"];
					if (ref.isString())
					{
						std::string s = ref.cast<std::string>();
						pass.rasterizerState.frontFace = Renderer::GetFrontFace(s);
					}

					if (pass.isCompute)
					{
						pass.shader = renderer->CreateComputeShader(shaderName, defines);
					}
					else
					{
						if (shaderName.length())
						{
							pass.shader = renderer->CreateShader(shaderName, shaderName, defines, inputDescs, pass.blendState);
						}
						else if (vertexName.length() != 0 && fragmentName.length() != 0)
						{
							if (geometryName.length() == 0)
							{
								pass.shader = renderer->CreateShader(vertexName, fragmentName, defines, inputDescs, pass.blendState);
							}
							else
							{
								pass.shader = renderer->CreateShaderWithGeometry(vertexName, geometryName, fragmentName, defines, inputDescs);
							}
						}
					}			

					shaderPasses.push_back(pass);
					Log::Print(LogLevel::LEVEL_INFO, "Added pass %u\n", shaderPasses.size() - 1);
				}
			}
		}

		// Load material ubo

		/*luabridge::LuaRef materiaUBOTable = matTable["materialUBO"];
		if (!materiaUBOTable.isNil())
		{
			unsigned int matUBOSize = 0;

			std::map<std::string, luabridge::LuaRef> values = scriptManager.GetKeyValueMap(materiaUBOTable);
			for (auto &pair : values)
			{
				std::string varType;
				if (pair.second.isString())
				{
					varType = pair.second.cast<std::string>();
					if (varType == "float")
						matUBOSize += sizeof(float);
					else if (varType == "int")
						matUBOSize += sizeof(int);
					else if (varType == "vec2")
						matUBOSize += sizeof(glm::vec2);
					else if (varType == "vec3")
						matUBOSize += sizeof(glm::vec3);
					else if (varType == "vec4")
						matUBOSize += sizeof(glm::vec4);
					else if (varType == "mat3")
						matUBOSize += sizeof(glm::mat3);
					else if (varType == "mat4")
						matUBOSize += sizeof(glm::mat4);
				}
			}

			//std::cout << "Mat ubo size: " << matUBOSize << '\n';

			//materialUBO = UniformBuffer::Create(renderer, nullptr, matUBOSize, BufferUsage::DYNAMIC);
			//materialUBO->BindTo(1);
		}

		// Load mesh params ubo
		meshParamsSize = 0;

		luabridge::LuaRef objectUBOTable = matTable["objectUBO"];
		if (!objectUBOTable.isNil())
		{
			std::map<std::string, luabridge::LuaRef> values = scriptManager.GetKeyValueMap(objectUBOTable);
			for (auto &pair : values)
			{
				std::string varType;
				if (pair.second.isString())
				{
					varType = pair.second.cast<std::string>();
					if (varType == "float")
						meshParamsSize += sizeof(float);
					else if (varType == "int")
						meshParamsSize += sizeof(int);
					else if (varType == "vec2")
						meshParamsSize += sizeof(glm::vec2);
					else if (varType == "vec3")
						meshParamsSize += sizeof(glm::vec3);
					else if (varType == "vec4")
						meshParamsSize += sizeof(glm::vec4);
					else if (varType == "mat3")
						meshParamsSize += sizeof(glm::mat3);
					else if (varType == "mat4")
						meshParamsSize += sizeof(glm::mat4);
					else if (varType == "mat4")
						meshParamsSize += sizeof(glm::mat4);
					else if (varType.substr(0, 5) == "mat4[")
					{
						std::string arraySizeStr = varType.substr(5);
						arraySizeStr.pop_back();
						meshParamsSize += sizeof(glm::mat4) * (unsigned int)std::stoi(arraySizeStr);
					}
				}
			}

			//std::cout << "Mesh params ubo size: " << meshParamsSize << '\n';
		}*/

		Log::Print(LogLevel::LEVEL_INFO, "Loading resources table\n");

		// Load resources table
		luabridge::LuaRef resourcesTable = matTable["resources"];
		if (!resourcesTable.isNil())
		{
			std::unordered_map<std::string, luabridge::LuaRef> values = scriptManager.GetKeyValueMap(resourcesTable);

			for (auto it = values.begin(); it != values.end(); ++it)
			{
				if (it->second.isTable())
				{
					TextureInfo texInfo = {};
					texInfo.name = it->first;
					texInfo.params = { TextureWrap::REPEAT, TextureFilter::LINEAR, TextureFormat::RGBA, TextureInternalFormat::SRGB8_ALPHA8, TextureDataType::UNSIGNED_BYTE, true, false, false, false };

					BufferInfo bufInfo = {};
					bufInfo.type = BufferType::ShaderStorageBuffer;

					bool isTexture = true;

					luabridge::LuaRef resTypeRef = it->second["resType"];
					std::string resTypeStr;
					if (resTypeRef.isString())
					{
						resTypeStr = resTypeRef.cast<std::string>();
						if (resTypeStr == "texture2D")
							texInfo.type = TextureType::TEXTURE2D;
						else if (resTypeStr == "texture3D")
							texInfo.type = TextureType::TEXTURE3D;
						else if (resTypeStr == "textureCube")
							texInfo.type = TextureType::TEXTURE_CUBE;
						else if (resTypeStr == "ssbo")
						{
							bufInfo.type = BufferType::ShaderStorageBuffer;
							isTexture = false;
						}
						else
						{
							std::cout << "Error! Material resource: " << it->second << " needs a type\n";
							return;
						}
					}

					if (!isTexture)
					{
						buffersInfo.push_back(bufInfo);
					}
					else
					{
						luabridge::LuaRef uvRef = it->second["uv"];
						if (uvRef.isString())
							texInfo.params.wrap = TextureWrapFromString(uvRef.cast<std::string>());

						luabridge::LuaRef texFormatRef = it->second["texFormat"];
						if (texFormatRef.isString())
						{
							texInfo.params.internalFormat = TextureInternalFormatFromString(texFormatRef.cast<std::string>());
							if (texInfo.params.internalFormat == TextureInternalFormat::RED8)
								texInfo.params.format = TextureFormat::RED;
							else if (texInfo.params.internalFormat == TextureInternalFormat::RGB8)
								texInfo.params.format = TextureFormat::RGB;
							else if (texInfo.params.internalFormat == TextureInternalFormat::R16F)
								texInfo.params.format = TextureFormat::RED;
						}

						luabridge::LuaRef texFilterRef = it->second["filter"];
						if (texFilterRef.isString())
						{
							texInfo.params.filter = TextureFilterFromString(texFilterRef.cast<std::string>());
						}

						luabridge::LuaRef useAlphaRef = it->second["alpha"];
						if (useAlphaRef.isBoolean())
						{
							texInfo.useAlpha = useAlphaRef.cast<bool>();
							if (!texInfo.useAlpha)
							{
								if (texInfo.params.format == TextureFormat::RGBA)
								{
									texInfo.params.format = TextureFormat::RGB;
									texInfo.params.internalFormat = TextureInternalFormat::RGB8;
								}
							}
						}

						luabridge::LuaRef storeDataRef = it->second["storeData"];
						if (storeDataRef.isBoolean())
							texInfo.storeData = storeDataRef.cast<bool>();

						luabridge::LuaRef useMipMapsRef = it->second["useMipMaps"];
						if (useMipMapsRef.isBoolean())
							texInfo.params.useMipmapping = useMipMapsRef.cast<bool>();

						luabridge::LuaRef usedInComputeRef = it->second["usedAsStorageInCompute"];
						if (usedInComputeRef.isBoolean())
							texInfo.params.usedAsStorageInCompute = usedInComputeRef.cast<bool>();

						texturesInfo.push_back(texInfo);
					}		
				}
			}
		}

		Log::Print(LogLevel::LEVEL_INFO, "Finished loading material\n");
	}

	MaterialInstance *Material::LoadMaterialInstance(Renderer *renderer, const std::string &path, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &descs)
	{
		Log::Print(LogLevel::LEVEL_INFO, "Loading material instance for %s\n", path.c_str());

		std::ifstream file = renderer->GetFileManager()->OpenForReading(path);

		if (!file.is_open())
		{
			Log::Print(LogLevel::LEVEL_ERROR, "ERROR -> Could not open material instance file: %s\n", path.c_str());
			return nullptr;
		} 

		MaterialInstance *mi = new MaterialInstance;
		mi->path = path;
		mi->lastParamOffset = 0;
		mi->computeSetID = std::numeric_limits<unsigned int>::max();
		mi->graphicsSetID = std::numeric_limits<unsigned int>::max();
		memset(mi->name, 0, 64);

		std::string line;
		unsigned int options = 0;
		unsigned int textureIdx = 0;
		std::vector<std::string> cubemapFaces;
		std::vector<std::string> texturePaths;

		while (std::getline(file, line))
		{
			if (line.substr(0, 8) == "defines=")
			{
				std::string str = line.substr(8);

				size_t first = str.find('\"');
				size_t last = str.find('\"', first + 1);
				std::string define = str.substr(first + 1, last - first - 1);

				options = SetOptions(options, define);

				while (first != std::string::npos || last != std::string::npos)
				{
					first = str.find(",\"", last);
					if (first == std::string::npos)
						break;
					last = str.find('\"', first + 2);
					if (last == std::string::npos)
						break;

					define = str.substr(first + 2, last - first - 2);

					options = SetOptions(options, define);
				}
			}
			else if (line.substr(0, 8) == "baseMat=")
			{
				std::string defines = "";
				/*if ((options & NORMAL_MATRIX) == NORMAL_MATRIX)
					defines += "#define NORMAL_MATRIX\n";*/
				if ((options & INSTANCING) == INSTANCING)
					defines += "#define INSTANCING\n";
				if ((options & ANIMATED) == ANIMATED)
					defines += "#define ANIMATED\n";
				if ((options & ALPHA) == ALPHA)
					defines += "#define ALPHA\n";

#ifdef EDITOR
				defines += "#define EDITOR\n";
#endif

				mi->baseMaterial = ResourcesLoader::LoadMaterial(renderer, line.substr(8), defines, scriptManager, descs);
				mi->textures.resize(mi->baseMaterial->texturesInfo.size());
			}
			else
			{
				//texturePaths.push_back(line.substr(line.find('=') + 1));
				//Log::Print(LogLevel::LEVEL_INFO, "%s\n", texturePaths[texturePaths.size()-1].c_str());

				std::string path = line.substr(line.find('=') + 1);
				std::string name;
				for (size_t i = 0; i < mi->textures.size(); i++)
				{
					name = mi->baseMaterial->texturesInfo[i].name;
					if (line.substr(0, name.length()) == name)
					{
						mi->textures[textureIdx] = renderer->CreateTexture2D(path, mi->baseMaterial->texturesInfo[i].params, mi->baseMaterial->texturesInfo[i].storeData);
						textureIdx++;
						break;
					}
				}
			}

			/*for (size_t i = 0; i < mi->textures.size(); i++)
			{
				std::string s(mi->baseMaterial->texturesInfo[i].name + '=');
				size_t length = s.length();
				if (line.substr(0, length) == s)
				{
					if (mi->baseMaterial->texturesInfo[i].type == TextureType::TEXTURE_CUBE)
					{
						//mi->textures[textureID] = ResourcesLoader::LoadTextureCube(line.substr(length), mi->baseMaterial->texturesInfo[i].params);		// Allow loading a cubemap from just one file or 6
						cubemapFaces.push_back(line.substr(length));
						if (cubemapFaces.size() >= 6)
						{
							mi->textures[textureID] = renderer->CreateTextureCube(cubemapFaces, mi->baseMaterial->texturesInfo[i].params);
							cubemapFaces.clear();
							textureID++;
						}
						break;
					}
					else
					{
						mi->textures[textureID] = renderer->CreateTexture2D(line.substr(length), mi->baseMaterial->texturesInfo[i].params, mi->baseMaterial->texturesInfo[i].storeData);
						textureID++;
						break;
					}
				}
			}*/
		}

		//Log::Print(LogLevel::LEVEL_INFO, "Loading material instance textures\n");

		// TODO : instead of adding this limit, load the textures for the texturePaths we have and for the missing one load the default white texture
		// Make sure that we don't load more textures than we can
		// We could have space for 2 texture but missing a texture path or we could have space for only 1 texture but have 2 texture paths
		//size_t texturesToLoad = mi->textures.size() > texturePaths.size() ? texturePaths.size() : mi->textures.size();

		/* textureIndex = 0;
		for (size_t i = 0; i < texturesToLoad; i++)
		{
			if (mi->baseMaterial->texturesInfo[i].type == TextureType::TEXTURE_CUBE)
			{
				// Read six texture paths to load the cubemap
				for (size_t i = 0; i < 6; i++)
				{
					cubemapFaces.push_back(texturePaths[textureIndex]);
					textureIndex++;
				}

				mi->textures[i] = renderer->CreateTextureCube(cubemapFaces, mi->baseMaterial->texturesInfo[i].params);			// Allow loading a cubemap from just one file or 6
				cubemapFaces.clear();
				textureIndex = 0;
			}
			else
			{
				Log::Print(LogLevel::LEVEL_INFO, "Loading texture\n");
				mi->textures[i] = renderer->CreateTexture2D(texturePaths[i], mi->baseMaterial->texturesInfo[i].params, mi->baseMaterial->texturesInfo[i].storeData);
			}
		}*/

		return mi;
	}

	MaterialInstance *Material::LoadMaterialInstanceFromBaseMat(Renderer *renderer, const std::string &baseMatPath, ScriptManager &scriptManager, const std::vector<VertexInputDesc> &inputDescs)
	{
		MaterialInstance *mi = new MaterialInstance;
		mi->lastParamOffset = 0;

		std::string defines = "";

#ifdef EDITOR
		defines += "#define EDITOR\n";
#endif

		mi->baseMaterial = ResourcesLoader::LoadMaterial(renderer, baseMatPath, defines, scriptManager, inputDescs);

		mi->textures.resize(mi->baseMaterial->texturesInfo.size());
		mi->buffers.resize(mi->baseMaterial->buffersInfo.size());

		/*for (size_t i = 0; i < mi->textures.size(); i++)
		{
			if (mi->baseMaterial->texturesInfo[i].params.usedAsStorageInCompute)
			{
				//mi->textures[i] = renderer->CreateTexture2DFromData(???, ???, mi->baseMaterial->texturesInfo[i].params, nullptr);
			}
		}*/

		return mi;
	}

	unsigned int Material::SetOptions(unsigned int options, const std::string &define)
	{
		if (define == "NORMAL_MATRIX")
			options |= NORMAL_MATRIX;
		else if (define == "INSTANCING")
			options |= INSTANCING;
		else if (define == "ANIMATED")
			options |= ANIMATED;
		else if (define == "ALPHA")
			options |= ALPHA;
		else
			std::cout << "Error! Shader define not found: " << define << "\n";

		return options;
	}

	BlendFactor Material::CompareBlendString(const std::string &str)
	{
		if (str == "zero")
			return ZERO;
		else if (str == "one")
			return ONE;
		else if (str == "src_alpha")
			return SRC_ALPHA;
		else if (str == "dst_alpha")
			return DST_ALPHA;
		else if (str == "src_color")
			return SRC_COLOR;
		else if (str == "dst_color")
			return DST_COLOR;
		else if (str == "one_minus_src_alpha")
			return ONE_MINUS_SRC_ALPHA;
		else if (str == "one_minus_src_color")
			return ONE_MINUS_SRC_COLOR;

		return ZERO;
	}

	TextureWrap Material::TextureWrapFromString(const std::string &str)
	{
		if (str == "repeat")
			return TextureWrap::REPEAT;
		else if (str == "clamp")
			return TextureWrap::CLAMP;
		else if (str == "clamp_to_edge")
			return TextureWrap::CLAMP_TO_EDGE;

		return TextureWrap::REPEAT;
	}

	TextureInternalFormat Material::TextureInternalFormatFromString(const std::string &str)
	{
		if (str == "red")
			return TextureInternalFormat::RED8;
		else if (str == "rgb")
			return TextureInternalFormat::RGB8;
		else if (str == "r16")
			return TextureInternalFormat::R16F;
		/*else if (str == "rgba")
			return TextureFormat::RGBA;*/

		return TextureInternalFormat::RGBA8;
	}

	TextureFilter Material::TextureFilterFromString(const std::string &str)
	{
		if (str == "nearest")
			return TextureFilter::NEAREST;
		else if (str == "linear")
			return TextureFilter::LINEAR;

		return TextureFilter::LINEAR;
	}

	Topology Material::CompareTopologyString(const std::string &str)
	{
		if (str == "triangles")
			return TRIANGLES;
		else if (str == "triangle_strip")
			return TRIANGLE_STRIP;
		else if (str == "lines")
			return LINES;
		else if (str == "line_strip")
			return LINE_TRIP;

		return TRIANGLES;
	}

	const ShaderPass &Material::GetShaderPass(unsigned int index) const
	{
		assert(index >= 0 && index < shaderPasses.size());

		return shaderPasses[index];
	}

	unsigned int Material::GetShaderPassIndex(const std::string &passName) const
	{
		unsigned int passID = SID(passName);

		for (size_t i = 0; i < shaderPasses.size(); i++)
		{
			if (shaderPasses[i].id == passID)
				return static_cast<unsigned int>(i);
		}

		return 0;
	}

	void MaterialInstance::AddParameter(MaterialParameterType type)
	{
		MaterialParameter param = {};
		param.type = type;
		param.offset = lastParamOffset;

		switch (type)
		{
		case MaterialParameterType::FLOAT:
			lastParamOffset += sizeof(float);
			param.size = sizeof(float);
			break;
		case MaterialParameterType::INT:
			lastParamOffset += sizeof(int);
			param.size = sizeof(int);
			break;
		case MaterialParameterType::VEC2:
			lastParamOffset += sizeof(glm::vec2);
			param.size = sizeof(glm::vec2);
			break;
		case MaterialParameterType::VEC4:
			lastParamOffset += sizeof(glm::vec4);
			param.size = sizeof(glm::vec4);
			break;
		case MaterialParameterType::COLOR3:
			lastParamOffset += sizeof(glm::vec3);
			param.size = sizeof(glm::vec3);
			break;
		case MaterialParameterType::COLOR4:
			lastParamOffset += sizeof(glm::vec4);
			param.size = sizeof(glm::vec4);
			break;
		}

		parameters.push_back(param);
	}

	void MaterialInstance::SetParameterValue(const MaterialParameter &param, const void *value)
	{
		// We could check if the material has the param...
		void *ptr = materialData + param.offset;
		memcpy(ptr, value, param.size);
	}
}
