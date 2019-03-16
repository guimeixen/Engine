#include "MaterialWindow.h"

#include "EditorManager.h"

#include "Graphics\ResourcesLoader.h"
#include "Graphics\Material.h"
#include "Graphics\Texture.h"
#include "Graphics\Shader.h"

#include "imgui\imgui.h"
#include "imgui\imgui_dock.h"

#include <map>
#include <iostream>

#include <Windows.h>

MaterialWindow::MaterialWindow()
{
	currentMaterial = nullptr;
	tempF = 0.0f;
	tempI = 0;
	tempv2 = glm::vec2();
	tempv4 = glm::vec4();
	tempColV3 = glm::vec3();
	tempColV4 = glm::vec4();
}

void MaterialWindow::Render()
{	
	if (BeginWindow("Material Editor"))
	{
		const std::map<unsigned int, Engine::MaterialRefInfo> materials = Engine::ResourcesLoader::GetMaterials();

		if (ImGui::CollapsingHeader("Base materials list"))
		{
			ImGui::Indent();
			for (auto &m : materials)
			{
				ImGui::Selectable(m.second.mat->GetPath().c_str());
				/*if(m.second.mat->ShowInEditor() && ImGui::Selectable(m.second.mat->GetPath().c_str()))
				{
					currentMaterial = m.second.mat;
				}*/
			}
			ImGui::Unindent();
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
						//pass.shader->
					}

					ImGui::TreePop();
				}

				ImGui::Separator();
				ImGui::Indent();
			}

			ImGui::Separator();
			ImGui::Text("Material Instance");

			if (currentMaterialInstance)
			{
				ImGui::Indent();
				if (ImGui::TreeNode("Textures"))
				{
					const std::vector<Engine::TextureInfo> &texturesInfo = currentMaterial->GetTexturesInfo();

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

					ImGui::TreePop();
				}

				ShowMaterialParameters();

				ImGui::Unindent();
			}
		}
	}
	if (focus)
	{
		ImGui::SetDockActive();
		focus = false;
	}
	EndWindow();
}

void MaterialWindow::AddTexture()
{
	static ImGuiTextFilter filter;
	filter.Draw("Find");
	for (size_t i = 0; i < files.size(); i++)
		if (filter.PassFilter(files[i].c_str()))
		{
			if (ImGui::Selectable(files[i].c_str()))
			{
				Engine::TextureParams params = {};
				params.enableCompare = false;
				params.filter = Engine::TextureFilter::LINEAR;
				params.format = Engine::TextureFormat::RGBA;
				params.internalFormat = Engine::TextureInternalFormat::SRGB8_ALPHA8;
				params.type = Engine::TextureDataType::UNSIGNED_BYTE;
				params.useMipmapping = true;
				params.wrap = Engine::TextureWrap::REPEAT;
				currentMaterialInstance->textures[textureIndex] = game->GetRenderer()->CreateTexture2D(files[i], params);
			}
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
