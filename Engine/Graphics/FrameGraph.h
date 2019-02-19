#pragma once

#include "Texture.h"
#include "Renderer.h"

#include <vector>
#include <unordered_set>
#include <functional>

namespace Engine
{
	class Texture;
	class Framebuffer;
	class FrameGraph;
	class Buffer;

	enum class InitialState : unsigned short
	{
		CLEAR,
		LOAD
	};

	struct AttachmentInfo
	{
		TextureParams params;
		unsigned int width;
		unsigned int height;
		InitialState initialState;
	};

	class TextureResource
	{
	public:
		TextureResource(const std::string &name, unsigned int index);

		const std::string &GetName() const { return name; }
		unsigned int GetNameID() const { return nameID; }

		void IsWrittenInPass(unsigned int passIndex) { writtenInPassIndices.insert(passIndex); }
		void IsReadInPass(unsigned int passIndex) { readInPassIndices.insert(passIndex); }

		void SetTexture(Texture *texture) { this->texture = texture; }
		Texture *GetTexture() const { return texture; }

		void SetReadWrite(bool readWrite) { this->readWrite = readWrite; }
		bool IsReadWrite() const { return readWrite; }

		void SetAttachmentInfo(const AttachmentInfo &info) { attachInfo = info; }
		const AttachmentInfo &GetAttachmentInfo() const { return attachInfo; }

		const std::unordered_set<unsigned int> &GetWritePasses() const { return writtenInPassIndices; }
		const std::unordered_set<unsigned int> &GetReadPasses() const { return readInPassIndices; }

	private:
		std::string name;
		unsigned int nameID;
		unsigned int index;
		AttachmentInfo attachInfo;
		std::unordered_set<unsigned int> writtenInPassIndices;
		std::unordered_set<unsigned int> readInPassIndices;
		Texture *texture;										// Used for texture used as storage
		bool readWrite;
	};

	class BufferResource
	{
	public:
		BufferResource(const std::string &name, unsigned int index);

		const std::string &GetName() const { return name; }
		unsigned int GetNameID() const { return nameID; }

		void IsWrittenInPass(unsigned int passIndex) { writtenInPassIndices.insert(passIndex); }
		void IsReadInPass(unsigned int passIndex) { readInPassIndices.insert(passIndex); }

		void SetBuffer(Buffer *buffer) { this->buffer = buffer; }
		Buffer *GetBuffer() const { return buffer; }

		const std::unordered_set<unsigned int> &GetWritePasses() const { return writtenInPassIndices; }
		const std::unordered_set<unsigned int> &GetReadPasses() const { return readInPassIndices; }

	private:
		std::string name;
		unsigned int nameID;
		unsigned int index;
		std::unordered_set<unsigned int> writtenInPassIndices;
		std::unordered_set<unsigned int> readInPassIndices;
		Buffer *buffer;
	};

	class Pass
	{
	protected: 
		friend FrameGraph;
	public:
		Pass(FrameGraph *fg, const std::string &name, unsigned int passIndex);

		const std::string &GetName() const { return name; }
		unsigned int GetNameID() const { return nameID; }
		unsigned int GetIndex() const { return passIndex; }
		Framebuffer *GetFramebuffer() const { return fb; }

		void SetIsCompute(bool isComp) { isCompute = isComp; }
		bool IsCompute() const { return isCompute; }
		
		void DisableWritesToFramebuffer() { writesToFramebuffer = false; }

		void Resize(unsigned int width, unsigned int height);

		void AddTextureOutput(const std::string &name, const AttachmentInfo &info);
		void AddDepthOutput(const std::string &name, const AttachmentInfo &info);

		void AddTextureInput(const std::string &name, Texture *texture = nullptr);
		void AddDepthInput(const std::string &name);

		void AddImageOutput(const std::string &name, Texture *texture, bool readWrite = false, bool overrideWriteFormat = false, TextureInternalFormat format = TextureInternalFormat::RGBA8);	// Default format doens't matter, because if override is false we will just use the image format
		void AddImageInput(const std::string &name, Texture *texture, bool sampled = false);

		void AddBufferOutput(const std::string &name, Buffer *buffer);
		void AddBufferInput(const std::string &name, Buffer *buffer);

		void SetOnBarriers(std::function<void()> func) { onBarriers = std::move(func); }
		void SetOnExecute(std::function<void()> func) { onExecute = std::move(func); }
		void SetOnResized(std::function<void(const Pass *thisPass)> func) { onResized = std::move(func); }
		void SetOnSetup(std::function<void(const Pass *thisPass)> func) { onSetup = std::move(func); }

		void OnBarriers() const { onBarriers(); }
		void Execute() const { onExecute(); }
		void OnResized() const { onResized(this); }
		void Setup() const { onSetup(this); }

		const std::vector<TextureResource*> &GetTextureOutputs()const { return textureOutputs; }
		const std::vector<TextureResource*> &GetTextureInputs()const { return textureInputs; }
		TextureResource *GetDepthOutput() const { return depthOutput; }
		const std::vector<TextureResource*> &GetDepthInputs() const { return depthInputs; }
		const std::vector<BufferResource*> &GetBufferOutputs() const { return bufferOutputs; }
		const std::vector<BufferResource*> &GetBufferInputs() const { return bufferInputs; }

	protected:
		Framebuffer *fb;

	private:
		FrameGraph *fg;
		std::string name;
		unsigned int nameID;
		unsigned int passIndex;
		unsigned int orderedIndex;
		bool isCompute;
		bool writesToFramebuffer;
		//std::vector<OutputTextureInfo> outputTexturesInfo;

		std::function<void()> onBarriers;
		std::function<void()> onExecute;
		std::function<void(const Pass *thisPass)> onResized;
		std::function<void(const Pass *thisPass)> onSetup;

		std::vector<TextureResource*> textureOutputs;
		std::vector<TextureResource*> textureInputs;
		std::vector<TextureResource*> imageOutputs;
		std::vector<TextureResource*> imageInputs;
		TextureResource *depthOutput;
		std::vector<TextureResource*> depthInputs;

		std::vector<BufferResource*> bufferOutputs;
		std::vector<BufferResource*> bufferInputs;

		Barrier barrier;
	};

	class FrameGraph
	{
	public:
		Pass &AddPass(const std::string &name);
		Pass &GetPass(const std::string &name);

		void SetBackbufferSource(const std::string &name) { backBufferSource = name; }

		void Bake(Renderer *renderer);
		void Setup();
		void Execute(Renderer *renderer);
		void Dispose();

		void ExportGraphVizFile();

		// Return the texture resource with name if found, otherwise creates a new one
		TextureResource *GetTextureResource(const std::string &name);
		BufferResource *GetBufferResource(const std::string &name);

	private:
		void FindDependencies(const Pass &lastPass);
		void FindDependenciesRecursive(const Pass &pass, const std::unordered_set<unsigned int> &writtenInPasses);
		void FilterPasses();

	private:
		std::vector<Pass> passes;
		std::vector<unsigned int> orderedPassesIndices;
		std::vector<TextureResource*> textureResources;
		std::vector<BufferResource*> bufferResources;
		//std::vector<Framebuffer*> fra
		std::string backBufferSource;
		unsigned int backBufferNameID;
	};
}
