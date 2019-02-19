#pragma once

#include "Graphics\VertexArray.h"

#include <d3d11_1.h>

namespace Engine
{
	class D3D11VertexArray : public VertexArray
	{
	public:
		D3D11VertexArray(ID3D11Device *device, const VertexInputDesc &desc, Buffer *vertexBuffer, Buffer *indexBuffer);
		D3D11VertexArray(ID3D11Device *device, const VertexInputDesc *descs, unsigned int descCount, const std::vector<Buffer*> &vertexBuffers, Buffer *indexBuffer);
		~D3D11VertexArray();

		void AddVertexBuffer(Buffer *vertexBuffer) override;

	private:
		
	};
}
