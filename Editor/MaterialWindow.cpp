#include "MaterialWindow.h"

#include "EditorManager.h"

#include "Graphics/ResourcesLoader.h"
#include "Graphics/Material.h"
#include "Graphics/Texture.h"
#include "Graphics/Shader.h"

#include "Program/FileManager.h"
#include "Program/StringID.h"

#include "imgui/imgui.h"
#include "imgui/imgui_dock.h"

#include <map>
#include <iostream>

MaterialWindow::MaterialWindow()
{
	currentMaterial = nullptr;
	currentMaterialInstance = nullptr;
	tempF = 0.0f;
	tempI = 0;
	tempv2 = glm::vec2();
	tempv4 = glm::vec4();
	tempColV3 = glm::vec3();
	tempColV4 = glm::vec4();

	displayCreateMaterialInstance = false;
	baseMaterialsInProjectLoaded = false;
	isNormalMap = false;

	memset(newMaterialInstanceName, 0, 64);

	basePassID = Engine::SID("base");
}

void MaterialWindow::Render()
{	
	if (BeginWindow("Material Editor"))
	{
		const std::map<unsigned int, Engine::MaterialRefInfo> materials = Engine::ResourcesLoader::GetMaterials();

		if (ImGui::Button("Create base material"))
		{

		}

		if (ImGui::CollapsingHeader("Loaded Base Materials"))
		{
			ImGui::Indent();
			for (auto &m : materials)
			{
				if (ImGui::TreeNode(m.second.mat->GetPath().c_str()))
				{
					std::vector<Engine::ShaderPass>& shaderPasses = m.second.mat->GetShaderPasses();
					const std::vector<Engine::TextureInfo>& texturesInfo = m.second.mat->GetTexturesInfo();

					for (size_t i = 0; i < shaderPasses.size(); i++)
					{
						Engine::ShaderPass& pass = shaderPasses[i];

						ImGui::Text("Pass ");
						ImGui::SameLine();
						ImGui::Text(std::to_string(i).c_str());

						if (pass.shader->GetComputeName().size() > 0)
						{
							ImGui::Text("Compute shader: ");
							ImGui::SameLine();
							ImGui::Text(pass.shader->GetComputeName().c_str());
						}
						else
						{
							ImGui::Text("Vertex shader: ");
							ImGui::SameLine();
							ImGui::Text(pass.shader->GetVertexName().c_str());

							if (pass.shader->GetGeometryName().size() > 0)
							{
								ImGui::Text("Geometry shader: ");
								ImGui::SameLine();
								ImGui::Text(pass.shader->GetGeometryName().c_str());
							}

							ImGui::Text("Fragment shader: ");
							ImGui::SameLine();
							ImGui::Text(pass.shader->GetFragmentName().c_str());
						}
						
					}

					for (size_t i = 0; i < texturesInfo.size(); i++)
					{
						ImGui::Text(texturesInfo[i].name.c_str());
					}


					ImGui::TreePop();
				}

				if (ImGui::BeginPopupContextItem())
				{
					std::string text = "Create material instance from base material: " + m.second.mat->GetName();

					if (ImGui::Button(text.c_str()))
					{
						selectedBaseMaterialPath = m.second.mat->GetPath();
						displayCreateMaterialInstance = true;
						
					}
					ImGui::EndPopup();
				}
				/*if(m.second.mat->ShowInEditor() && ImGui::Selectable(m.second.mat->GetPath().c_str()))
				{
					currentMaterial = m.second.mat;
				}*/
			}
			ImGui::Unindent();
		}

		if (ImGui::CollapsingHeader("Base materials in project folder"))
		{
			if (!baseMaterialsInProjectLoaded)
			{
				baseMaterialsInProjectStr.clear();
				Engine::utils::FindFilesInDirectory(baseMaterialsInProjectStr, editorManager->GetCurrentProjectDir() + "/*", "_mat.lua");

				baseMaterialsInProjectLoaded = true;
			}

			ImGui::Indent();
			for (const std::string& s : baseMaterialsInProjectStr)
			{
				ImGui::Text(s.c_str());

				if (ImGui::BeginPopupContextItem("NewMatContextPopup"))
				{
					std::string text = "Create material instance from base material: " + s;

					if (ImGui::Button(text.c_str()))
					{
						displayCreateMaterialInstance = true;
						selectedBaseMaterialPath = s;
					}
					ImGui::EndPopup();
				}
			}
			ImGui::Unindent();
		}

		if (displayCreateMaterialInstance)
		{
			ImGui::OpenPopup("Create material instance");
			displayCreateMaterialInstance = false;
		}

		if (ImGui::BeginPopup("Create material instance"))
		{
			if (ImGui::InputText("Name", newMaterialInstanceName, 64, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("Create"))
			{
				std::string path = editorManager->GetCurrentProjectDir() + '/' + newMaterialInstanceName + ".mat";

				if (std::filesystem::exists(path))
				{
					// File already exists
					// TODO show alert
				}
				else
				{
					std::ofstream file = game->GetFileManager()->OpenForWriting(path);
					file << "baseMat=" << selectedBaseMaterialPath << "\ndiffuse=Data/Textures/white.dds\nnormal=Data/Textures/normal.dds";
					file.close();
					memset(newMaterialInstanceName, 0, 64);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}


		if (ImGui::CollapsingHeader("Current material"))
		{
			ImGui::Text("Base Material");
			if (currentMaterial)
			{
				ImGui::Indent();

				const std::string str("Path: " + currentMaterial->GetPath());
				ImGui::Text(str.c_str());

				if (ImGui::TreeNode("Shader pass"))
				{
					std::vector<Engine::ShaderPass> &shaderPasses = currentMaterial->GetShaderPasses();

					for (size_t i = 0; i < shaderPasses.size(); i++)
					{
						Engine::ShaderPass &pass = shaderPasses[i];

						if (pass.id != basePassID)
							continue;

						ImGui::Text("Pass Base");
						
						if (ImGui::Selectable("Vertex shader:"))
						{
							ImGui::OpenPopup("Choose shader");
							const std::string dir = editorManager->GetCurrentProjectDir() + "/*";
							files.clear();
							Engine::utils::FindFilesInDirectory(files, dir, ".vert");
						}
						ImGui::SameLine();
						ImGui::Text(pass.shader->GetVertexName().c_str());
										
						if (ImGui::Selectable("Fragment shader: "))
						{
							ImGui::OpenPopup("Choose shader");
							const std::string dir = editorManager->GetCurrentProjectDir() + "/*";
							files.clear();
							Engine::utils::FindFilesInDirectory(files, dir, ".frag");
						}
						ImGui::SameLine();
						ImGui::Text(pass.shader->GetFragmentName().c_str());

						if (ImGui::BeginPopup("Choose shader"))
						{
							if (files.size() > 0)
								ChangeShader();
							else
								ImGui::Text("No shaders found on project folder.");

							ImGui::EndPopup();
						}
					}			

					ImGui::TreePop();
				}

				ImGui::Separator();
				ImGui::Indent();
			}

			ImGui::Separator();
			//ImGui::Unindent();
			ImGui::Text("Material Instance");

			if (currentMaterialInstance)
			{
				ImGui::Indent();

				ShowTextures();
				ShowMaterialParameters();

				ImGui::Unindent();
			}
		}
	}
	
	EndWindow();
}

void MaterialWindow::CreateMaterial(const std::string& name, const std::string& path)
{
	Engine::VertexAttribute attribs[4] = {};
	attribs[0].count = 3;						// Position
	attribs[1].count = 2;						// UV
	attribs[2].count = 3;						// Normal
	attribs[3].count = 3;						// Tangent

	attribs[0].offset = 0;
	attribs[1].offset = 3 * sizeof(float);
	attribs[2].offset = 5 * sizeof(float);
	attribs[3].offset = 8 * sizeof(float);

	Engine::VertexInputDesc desc = {};
	desc.stride = sizeof(Engine::VertexPOS3D_UV_NORMAL_TANGENT);
	desc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3] };

	std::ofstream file = game->GetFileManager()->OpenForWriting(path);
	file << name << " =\n{\n\tpasses =\n\t{\n\t\tbase =\n\t\t{\n\t\t\tqueue='opaque',\n\t\t\tshader='model',\n\t\t}\n\t},\n\tresources =\n\t{\n\t\tdiffuse =\n\t\t{\n\t\t\tresType=\"texture2D\"\n\t\t},\n\t\tnormal =\n\t\t{\n\t\t\tresType=\"texture2D\"\n\t\t}\n\t}\n}";
	file.close();

	Engine::MaterialInstance* m = game->GetRenderer()->CreateMaterialInstanceFromBaseMat(game->GetScriptManager(), path, { desc });
	//SetCurrentMaterialInstance(m);

	baseMaterialsInProjectLoaded = false;
}

void MaterialWindow::AddTexture()
{
	ImGui::Checkbox("Is Normal Map?", &isNormalMap);

	static ImGuiTextFilter filter;
	filter.Draw("Find");
	for (size_t i = 0; i < files.size(); i++)
	{
		if (filter.PassFilter(files[i].c_str()))
		{
			if (ImGui::Selectable(files[i].c_str()))
			{
				Engine::TextureParams params = {};
				params.enableCompare = false;
				params.filter = Engine::TextureFilter::LINEAR;
				params.format = Engine::TextureFormat::RGBA;				
				params.type = Engine::TextureDataType::UNSIGNED_BYTE;
				params.useMipmapping = true;
				params.wrap = Engine::TextureWrap::REPEAT;

				if (isNormalMap)
					params.internalFormat = Engine::TextureInternalFormat::RGBA8;
				else
					params.internalFormat = Engine::TextureInternalFormat::SRGB8_ALPHA8;

				currentMaterialInstance->textures[textureIndex]->RemoveReference();
				game->GetRenderer()->RemoveTexture(currentMaterialInstance->textures[textureIndex]);

				currentMaterialInstance->textures[textureIndex] = game->GetRenderer()->CreateTexture2D(files[i], params);

				game->GetRenderer()->UpdateMaterialInstance(currentMaterialInstance);
			}
		}
	}
}

void MaterialWindow::ChangeShader()
{
	static ImGuiTextFilter filter;
	filter.Draw("Find");
	for (size_t i = 0; i < files.size(); i++)
	{
		const char* str = files[i].c_str();
		if (filter.PassFilter(str))
		{
			if (ImGui::Selectable(str))
			{
				
			}
		}
	}
}

void MaterialWindow::ShowTextures()
{
	if (ImGui::TreeNode("Textures"))
	{
		const std::vector<Engine::TextureInfo>& texturesInfo = currentMaterial->GetTexturesInfo();

		for (size_t i = 0; i < currentMaterialInstance->textures.size(); i++)
		{
			if (ImGui::TreeNode(texturesInfo[i].name.c_str()))
			{
				ImGui::Text(currentMaterialInstance->textures[i]->GetPath().c_str());

				ShowTextureParameters();

				textureIndex = static_cast<unsigned int>(i);

				if (ImGui::Button("Change Texture"))
				{
					ImGui::OpenPopup("Choose texture");
					const std::string dir = editorManager->GetCurrentProjectDir() + "/*";
					files.clear();
					Engine::utils::FindFilesInDirectory(files, dir, ".png");
					Engine::utils::FindFilesInDirectory(files, dir, ".jpg");
					Engine::utils::FindFilesInDirectory(files, dir, ".dds");
					Engine::utils::FindFilesInDirectory(files, dir, ".ktx");

					Engine::utils::FindFilesInDirectory(files, "Data/Resources/Textures/*", ".png");
					Engine::utils::FindFilesInDirectory(files, "Data/Resources/Textures/*", ".jpg");
					Engine::utils::FindFilesInDirectory(files, "Data/Resources/Textures/*", ".dds");
					Engine::utils::FindFilesInDirectory(files, "Data/Resources/Textures/*", ".ktx");
				}

				if (ImGui::BeginPopup("Choose texture"))
				{
					if (files.size() > 0)
						AddTexture();
					else
						ImGui::Text("No textures found on project folder.");

					ImGui::EndPopup();
				}

				ImGui::TreePop();
			}
		}

		if (ImGui::Button("Add Texture"))
		{

		}

		ImGui::TreePop();
	}
}

void MaterialWindow::ShowTextureParameters()
{
	static const char* filters[] = { "Linear", "Nearest" };
	static const char* wraps[] = { "Repeat", "Clamp", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border" };

	if (ImGui::Combo("Filter", &textureFilterComboIndex, filters, 2))
	{
		switch (textureFilterComboIndex)
		{
		case 0:
			//obj->SetLayer(Engine::Layer::DEFAULT);
			break;
		case 1:
			//obj->SetLayer(Engine::Layer::OBSTACLE);
			break;
		}
	}

	if (ImGui::Combo("Wrap", &textureWrapComboIndex, wraps, 5))
	{
		switch (textureWrapComboIndex)
		{
		case 0:
			//obj->SetLayer(Engine::Layer::DEFAULT);
			break;
		case 1:
			//obj->SetLayer(Engine::Layer::OBSTACLE);
			break;
		}
	}
}

void MaterialWindow::ShowMaterialParameters()
{
	const std::vector<Engine::MaterialParameter> &parameters = currentMaterialInstance->GetMaterialParameters();

	ImGui::Separator();
	ImGui::Unindent();
	ImGui::Unindent();

	ImGui::Text("Parameters:");

	if (ImGui::Button("Add Parameter"))
	{
		ImGui::OpenPopup("Choose type");
	}

	if (ImGui::BeginPopup("Choose type"))
	{
		static const char* types[] = { "Float", "Int", "Vec2", "Vec4", "Color3", "Color4" };
		ImGui::Combo("Type", &matParamTypeComboIndex, types, 6);

		if (ImGui::Button("Add"))
		{
			switch (matParamTypeComboIndex)
			{
			case 0:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::FLOAT);
				break;
			case 1:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::INT);
				break;
			case 2:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::VEC2);
				break;
			case 3:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::VEC4);
				break;
			case 4:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::COLOR3);
				break;
			case 5:
				currentMaterialInstance->AddParameter(Engine::MaterialParameterType::COLOR4);
				break;
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();

	for (size_t i = 0; i < parameters.size(); i++)
	{
		const Engine::MaterialParameter &param = parameters[i];

		if (param.type == Engine::MaterialParameterType::FLOAT)
		{
			if (ImGui::DragFloat("Param f", &tempF, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempF);
			}
		}
		else if(param.type == Engine::MaterialParameterType::INT)
		{
			if (ImGui::DragInt("Param i", &tempI, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempI);
			}
		}
		else if (param.type == Engine::MaterialParameterType::VEC2)
		{
			if (ImGui::DragFloat2("Param v2", &tempv2.x, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempv2.x);
			}
		}
		else if (param.type == Engine::MaterialParameterType::VEC4)
		{
			if (ImGui::DragFloat4("Param v4", &tempv4.x, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempv4.x);
			}
		}
		else if (param.type == Engine::MaterialParameterType::COLOR3)
		{
			if (ImGui::DragFloat3("Param col v3", &tempColV3.x, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempColV3.x);
			}
		}
		else if (param.type == Engine::MaterialParameterType::COLOR4)
		{
			if (ImGui::DragFloat4("Param col v4", &tempColV4.x, 0.1f))
			{
				currentMaterialInstance->SetParameterValue(param, &tempColV4.x);
			}
		}
	}
}
