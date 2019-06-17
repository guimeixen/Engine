#include "FrameGraph.h"

#include "Program/StringID.h"
#include "Framebuffer.h"
#include "Buffers.h"
#include "Program/Log.h"

#include <iostream>
#include <algorithm>
#include <ostream>

namespace Engine
{
	TextureResource::TextureResource(const std::string &name, unsigned int index) : index(index), name(name)
	{
		nameID = SID(name);
		texture = nullptr;
	}

	BufferResource::BufferResource(const std::string &name, unsigned int index) : index(index), name(name)
	{
		nameID = SID(name);
		buffer = nullptr;
	}

	Pass::Pass(FrameGraph *fg, const std::string &name, unsigned int passIndex) : fg(fg), name(name), passIndex(passIndex)
	{
		nameID = SID(name);
		depthOutput = nullptr;
		fb = nullptr;
		isCompute = false;
		isSetup = false;
	}

	void Pass::Resize(unsigned int width, unsigned int height)
	{
		for (size_t i = 0; i < textureOutputs.size(); i++)
		{
			TextureResource *tr = textureOutputs[i];
			const AttachmentInfo &oldInfo = tr->GetAttachmentInfo();
			AttachmentInfo newInfo = {};
			newInfo.initialState = oldInfo.initialState;
			newInfo.params = oldInfo.params;
			newInfo.width = width;
			newInfo.height = height;
			tr->SetAttachmentInfo(newInfo);
		}

		if (depthOutput)
		{
			const AttachmentInfo &oldInfo = depthOutput->GetAttachmentInfo();
			AttachmentInfo newInfo = {};
			newInfo.initialState = oldInfo.initialState;
			newInfo.params = oldInfo.params;
			newInfo.width = width;
			newInfo.height = height;
			depthOutput->SetAttachmentInfo(newInfo);
		}
	}

	void Pass::AddTextureOutput(const std::string &name, const AttachmentInfo &info)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsWrittenInPass(passIndex);
		tr->SetAttachmentInfo(info);

		textureOutputs.push_back(tr);
	}

	void Pass::AddDepthOutput(const std::string &name, const AttachmentInfo &info)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsWrittenInPass(passIndex);
		tr->SetAttachmentInfo(info);

		depthOutput = tr;
	}

	void Pass::AddTextureInput(const std::string &name, Texture *texture)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsReadInPass(passIndex);
		tr->SetTexture(texture);

		textureInputs.push_back(tr);
	}

	void Pass::AddDepthInput(const std::string &name)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsReadInPass(passIndex);

		depthInputs.push_back(tr);
	}

	void Pass::AddImageOutput(const std::string &name, Texture *texture, bool readWrite, bool overrideWriteFormat, TextureInternalFormat format)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsWrittenInPass(passIndex);
		tr->SetTexture(texture);
		tr->SetReadWrite(readWrite);
		//tr->SetAttachmentInfo(info);

		imageOutputs.push_back(tr);
	}

	void Pass::AddImageInput(const std::string &name, bool sampled)
	{
		TextureResource *tr = fg->GetTextureResource(name);
		tr->IsReadInPass(passIndex);
		//tr->SetTexture(texture);			// Always set the texture. Doesn't matter because the texture needs to be the same in output and input

		imageInputs.push_back(tr);
	}

	void Pass::AddBufferOutput(const std::string &name, Buffer *buffer)
	{
		BufferResource *br = fg->GetBufferResource(name);
		br->IsWrittenInPass(passIndex);
		br->SetBuffer(buffer);

		bufferOutputs.push_back(br);
	}

	void Pass::AddBufferInput(const std::string &name, Buffer *buffer)
	{
		BufferResource *br = fg->GetBufferResource(name);
		br->IsReadInPass(passIndex);
		br->SetBuffer(buffer);			// Always set the texture. Doesn't matter because the texture needs to be the same in output and input

		bufferInputs.push_back(br);
	}

	Pass &FrameGraph::AddPass(const std::string &name)
	{
		Pass p(this, name, static_cast<unsigned int>(passes.size()));
		p.writesToFramebuffer = true;
		passes.push_back(p);

		return passes[passes.size() - 1];
	}

	Pass &FrameGraph::GetPass(const std::string &name)
	{
		unsigned int nameID = SID(name);
		for (size_t i = 0; i < passes.size(); i++)
		{
			if (passes[i].GetNameID() == nameID)
				return passes[i];
		}

		return AddPass(name);
	}

	void FrameGraph::FindDependencies(const Pass &lastPass)
	{
		for (size_t i = 0; i < lastPass.depthInputs.size(); i++)
		{
			FindDependenciesRecursive(lastPass, lastPass.depthInputs[i]->GetWritePasses());
		}

		// Get the texture inputs this pass needs so we can find out which pass writes an input
		for (size_t i = 0; i < lastPass.textureInputs.size(); i++)
		{
			FindDependenciesRecursive(lastPass, lastPass.textureInputs[i]->GetWritePasses());
		}

		for (size_t i = 0; i < lastPass.imageInputs.size(); i++)
		{
			FindDependenciesRecursive(lastPass, lastPass.imageInputs[i]->GetWritePasses());
		}

		for (size_t i = 0; i < lastPass.bufferInputs.size(); i++)
		{
			FindDependenciesRecursive(lastPass, lastPass.bufferInputs[i]->GetWritePasses());
		}
	}

	void FrameGraph::FindDependenciesRecursive(const Pass &pass, const std::unordered_set<unsigned int> &writtenInPasses)
	{
		for (const auto &passIndex : writtenInPasses)
		{
			orderedPassesIndices.push_back(passIndex);
			FindDependencies(passes[passIndex]);
			//if (passIndex == pass.GetIndex())
		}
	}

	void FrameGraph::FilterPasses()
	{
		std::unordered_set<unsigned int> valuesChecked;

		auto outputIt = orderedPassesIndices.begin();
		for (auto it = orderedPassesIndices.begin(); it != orderedPassesIndices.end(); it++)
		{
			// Check if we have the value in  the set, (count returns 1 if it is there, 0 if it isn't
			if (!valuesChecked.count(*it))
			{
				valuesChecked.insert(*it);
				*outputIt = *it;		// Swap the values
				outputIt++;				// Advance the iterator where we will begin deleting
			}
		}

		orderedPassesIndices.erase(outputIt, orderedPassesIndices.end());
	}

	void FrameGraph::Bake(Renderer *renderer)
	{
		// Start at the back buffer and work our way up
		backBufferNameID = SID(backBufferSource);

		auto it = std::find_if(passes.begin(), passes.end(),
		[this](const Pass &pass)
		{
			return pass.GetNameID() == backBufferNameID; 
		});
		
		if (it == passes.end())
		{
			std::cout << "Back buffer source not set!\n";
			return;
		}

		orderedPassesIndices.clear();
		orderedPassesIndices.push_back((*it).GetIndex());

		FindDependencies((*it));

		std::reverse(orderedPassesIndices.begin(), orderedPassesIndices.end());
		FilterPasses();

		for (size_t i = 0; i < orderedPassesIndices.size(); i++)
		{
			// Create the resources for each pass
			Pass &pass = passes[orderedPassesIndices[i]];
			pass.orderedIndex = (unsigned int)i;

			// We don't want to create a framebuffer for the back buffer because it is handled seperately
			if (pass.GetNameID() == backBufferNameID)
				continue;

			const std::vector<TextureResource*> outputTextures = pass.GetTextureOutputs();
			TextureResource *depthOutput = pass.GetDepthOutput();

			FramebufferDesc desc = {};
			desc.passID = pass.GetNameID();
			desc.useDepth = false;
			desc.writesDisabled = !pass.writesToFramebuffer;

			bool hasTextureOutputs = outputTextures.size() > 0 || depthOutput != nullptr ? true : false;
			//bool usesCompute = false;

			for (size_t i = 0; i < outputTextures.size(); i++)
			{
				const TextureResource *tr = outputTextures[i];
				const AttachmentInfo &info = tr->GetAttachmentInfo();

				desc.colorTextures.push_back(info.params);
				desc.width = info.width;
				desc.height = info.height;

				// Check if the width and height match
				if (i > 0)
				{
					const TextureResource *prevTr = outputTextures[i - 1];
					const AttachmentInfo &prevInfo = prevTr->GetAttachmentInfo();
					// This could be check earlier so we avoid creating resources if something isn't right
					if (prevInfo.width != desc.width || prevInfo.height != desc.height)
					{
						std::cout << "For Pass: " << pass.GetName() << " the attachments dimensions don't match!\n";
						return;
					}
				}
			}

			if (pass.writesToFramebuffer == false)
			{
				for (size_t i = 0; i < pass.imageOutputs.size(); i++)
				{
					Texture *tex = pass.imageOutputs[i]->GetTexture();
					desc.width = tex->GetWidth();
					desc.height = tex->GetHeight();
				}
			}
		

			if (depthOutput)
			{
				const AttachmentInfo &info = depthOutput->GetAttachmentInfo();
				desc.depthTexture = info.params;
				desc.useDepth = true;
				desc.sampleDepth = depthOutput->GetReadPasses().size() > 0;


				// Check if the dimensions match with the color textures
				if (desc.colorTextures.size() > 0 && (desc.width != info.width || desc.height != info.height))
				{
					std::cout << "For Pass: " << pass.GetName() << " the depth attachment dimensions don't match the color attachments!\n";
					return;
				}
				else
				{
					desc.width = info.width;		// Else we only have a depth attachment
					desc.height = info.height;
				}
			}

			if ((hasTextureOutputs && !pass.isCompute) || pass.writesToFramebuffer == false)
			{
				// Because if writes are disabled then we use the window width/height for the framebuffer size
				/*if (desc.writesDisabled)
				{
					desc.width = windowWidth;
					desc.height = windowHeight;
				}*/

				if (pass.fb != nullptr)
				{
					if (desc.width != pass.fb->GetWidth() || desc.height != pass.fb->GetHeight())
					{
						pass.fb->Resize(desc);
						if (pass.onResized)
							pass.OnResized();
					}
				}
				else
				{
					pass.fb = renderer->CreateFramebuffer(desc);
					pass.fb->AddReference();
				}				
			}
		}

		std::cout << "Done baking frame graph\n";
	}

	void FrameGraph::Setup()
	{
		for (size_t i = 0; i < orderedPassesIndices.size(); i++)
		{
			unsigned int index = orderedPassesIndices[i];

			Pass &pass = passes[index];
			if (pass.onSetup && pass.isSetup == false)
			{
				pass.Setup();
				pass.isSetup = true;
			}
		}
	}

	void FrameGraph::Execute(Renderer *renderer)
	{
		for (size_t i = 0; i < orderedPassesIndices.size(); i++)
		{
			unsigned int index = orderedPassesIndices[i];

			Pass &pass = passes[index];

			renderer->ClearBoundImages();		// For D3D11

			if (pass.isCompute)
			{
				//bool needsBarrier = false;

				/*for (size_t i = 0; i < pass.imageInputs.size(); i++)
				{
					renderer->BindImage((unsigned int)i, 0, pass.imageInputs[i]->GetTexture(), ImageAccess::READ_ONLY);
				}
				for (size_t i = 0; i < pass.imageOutputs.size(); i++)
				{
					TextureResource *res = pass.imageOutputs[i];

					renderer->BindImage((unsigned int)i + pass.imageInputs.size(), 0, res->GetTexture(), res->IsReadWrite() ? ImageAccess::READ_WRITE : ImageAccess::WRITE_ONLY);
				}*/

				/*if (pass.barrier.images.size() > 0 || pass.barrier.buffers.size() > 0)
					renderer->PerformBarrier(pass.barrier);*/

				if (pass.onBarriers)
					pass.OnBarriers();

				pass.Execute();
			}
			else
			{
				/*for (size_t i = 0; i < pass.imageInputs.size(); i++)
				{
					TextureResource *res = pass.imageInputs[i];
					Texture *tex = res->GetTexture();

					renderer->BindImage((unsigned int)i, tex, ImageAccess::READ_ONLY);
				}*/

				/*for (size_t i = 0; i < pass.imageOutputs.size(); i++)
				{
					TextureResource *res = pass.imageOutputs[i];
					Texture *tex = res->GetTexture();

					renderer->BindImage((unsigned int)i + pass.imageInputs.size(), 0, tex, res->IsReadWrite() ? ImageAccess::READ_WRITE : ImageAccess::WRITE_ONLY);
				}*/


				/*if (pass.barrier.images.size() > 0 || pass.barrier.buffers.size() > 0)
					renderer->PerformBarrier(pass.barrier);*/

				if (pass.onBarriers)
					pass.OnBarriers();

				// Writes to backbuffer
				if (pass.GetNameID() == backBufferNameID)
				{
					renderer->SetDefaultRenderTarget();
					pass.Execute();
					renderer->EndDefaultRenderTarget();
				}
				else
				{
					renderer->SetRenderTargetAndClear(pass.fb);
					pass.Execute();
					renderer->EndRenderTarget(pass.fb);
				}
			}
		}
	}

	void FrameGraph::Dispose()
	{
		for (size_t i = 0; i < passes.size(); i++)
		{
			if (passes[i].fb)
				passes[i].fb->RemoveReference();
		}
		for (size_t i = 0; i < textureResources.size(); i++)
		{
			if (textureResources[i])
				delete textureResources[i];
		}
		for (size_t i = 0; i < bufferResources.size(); i++)
		{
			if (bufferResources[i])
				delete bufferResources[i];
		}
		Log::Print(LogLevel::LEVEL_INFO, "Disposing Framegraph\n");
	}

	void FrameGraph::ExportGraphVizFile()
	{
		std::ofstream file("Data/graph.dot");

		if (!file.is_open())
			return;

		file << "digraph framegraph{\nrankdir=\"LR\";\nnode[shape=rectangle];\n";

		for (size_t i = 0; i < orderedPassesIndices.size(); i++)
		{
			const Pass &pass = passes[orderedPassesIndices[i]];

			file << passes[orderedPassesIndices[i]].name << "[style=filled fillcolor=grey];\n";

			for (size_t j = 0; j < pass.textureOutputs.size(); j++)
			{
				file << pass.textureOutputs[j]->GetName() << "[style=filled fillcolor=darkorange];\n";
			}
			for (size_t j = 0; j < pass.imageOutputs.size(); j++)
			{
				file << pass.imageOutputs[j]->GetName() << "[style=filled fillcolor=darkorange];\n";
			}
			for (size_t j = 0; j < pass.bufferOutputs.size(); j++)
			{
				file << pass.bufferOutputs[j]->GetName() << "[style=filled fillcolor=darksalmon];\n";
			}

			if (pass.depthOutput)
				file << pass.depthOutput->GetName() << "[style=filled fillcolor=darkorange];\n";
		}
		file << '\n';

		for (size_t i = 0; i < orderedPassesIndices.size(); i++)
		{
			const Pass &pass = passes[orderedPassesIndices[i]];

			for (size_t j = 0; j < pass.textureOutputs.size(); j++)
			{
				file << pass.name << "->" << pass.textureOutputs[j]->GetName() << ";\n";
			}
			for (size_t j = 0; j < pass.imageOutputs.size(); j++)
			{
				file << pass.name << "->" << pass.imageOutputs[j]->GetName() << ";\n";
			}
			for (size_t j = 0; j < pass.bufferOutputs.size(); j++)
			{
				file << pass.name << "->" << pass.bufferOutputs[j]->GetName() << ";\n";
			}

			if (pass.depthOutput)
				file << pass.name << "->" << pass.depthOutput->GetName() << ";\n";
		}

		for (int i = orderedPassesIndices.size() - 1; i >= 0; i--)
		{
			const Pass &pass = passes[orderedPassesIndices[i]];

			for (size_t j = 0; j < pass.textureInputs.size(); j++)
			{
				file << pass.textureInputs[j]->GetName() << "->" << pass.name << ";\n";
			}
			for (size_t j = 0; j < pass.imageInputs.size(); j++)
			{
				/*bool computeToCompute = false;

				if (i > 0 && passes[orderedPassesIndices[i - 1]].isCompute)
				{
					if (pass.isCompute)
						computeToCompute = true;
				}*/

				/*if (computeToCompute)
				{
					//file << pass.imageInputs[j]->GetName() << "->";
				}
				else
				{*/
					file << pass.imageInputs[j]->GetName() << "->\"" << pass.imageInputs[j]->GetName() << " Barrier RAW\";\n";
					file << "\"" << pass.imageInputs[j]->GetName() << " Barrier RAW\"" << "->" << pass.name << ";\n";
				//}			
			}
			for (size_t j = 0; j < pass.bufferInputs.size(); j++)
			{
				//file << pass.bufferInputs[j]->GetName() << "->" << pass.name << "\n";
				file << pass.bufferInputs[j]->GetName() << "->\"" << pass.bufferInputs[j]->GetName() << " Barrier RAW\";\n";
				file << "\"" << pass.bufferInputs[j]->GetName() << " Barrier RAW\"" << "->" << pass.name << ";\n";
			}
			for (size_t j = 0; j < pass.depthInputs.size(); j++)
			{
				file << pass.depthInputs[j]->GetName() << "->" << pass.name << "\n";
			}
		}

		file << "}";
	}

	TextureResource *FrameGraph::GetTextureResource(const std::string &name)
	{
		unsigned int nameID = SID(name);

		for (size_t i = 0; i < textureResources.size(); i++)
		{
			if (textureResources[i]->GetNameID() == nameID)
				return textureResources[i];
		}

		TextureResource *tr = new TextureResource(name, static_cast<unsigned int>(textureResources.size()));
		
		textureResources.push_back(tr);

		return tr;
	}

	BufferResource *FrameGraph::GetBufferResource(const std::string &name)
	{
		unsigned int nameID = SID(name);

		for (size_t i = 0; i < bufferResources.size(); i++)
		{
			if (bufferResources[i]->GetNameID() == nameID)
				return bufferResources[i];
		}

		BufferResource *br = new BufferResource(name, static_cast<unsigned int>(bufferResources.size()));

		bufferResources.push_back(br);

		return br;
	}
}
