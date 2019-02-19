#pragma once

#include "Graphics\Buffers.h"

namespace Engine
{
	class GLSSBO : public Buffer
	{
	public:
		GLSSBO(const void *data, unsigned int size);
		~GLSSBO();

		 void BindTo(unsigned int bindingIndex) ;
		 void Update(const void *data, unsigned int size, int offset) override;

		 unsigned int GetID() const { return id; }

	private:
		unsigned int id;
	};
}
