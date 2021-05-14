#pragma once

#include "include/glm/glm.hpp"

#include <array>
#include <vector>

namespace Engine
{
	enum class VertexAttributeFormat
	{
		FLOAT = 0,
		INT
	};

	struct VertexAttribute
	{
		VertexAttributeFormat vertexAttribFormat;
		unsigned int count;
		unsigned int offset;
	};

	struct VertexInputDesc
	{
		unsigned int stride;
		std::vector<VertexAttribute> attribs;
		bool instanced;
	};

	struct VertexPOS3D
	{
		glm::vec3 pos;
	};

	struct VertexPOS3D_UV_NORMAL_TANGENT
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
	};

	struct VertexPOS2D_UV
	{
		glm::vec4 posuv;
	};

	struct VertexPOS2D_UV_COLOR
	{
		glm::vec4 posuv;
		glm::vec4 color;
	};

	struct VertexPOS3D_UV_NORMAL_COLOR
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 color;
	};

	struct VertexPOS3D_UV_NORMAL_BONES
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::ivec4 boneIDs;
		glm::vec4 weights;
	};
}
