#include "MeshDefaults.h"

#include "Renderer.h"
#include "VertexArray.h"
#include "Buffers.h"
#include "VertexTypes.h"

namespace Engine
{
	namespace MeshDefaults
	{
		Mesh CreateCube(Renderer *renderer, float size, bool lines, bool instanced)
		{
			if (lines)
			{
				float vertices[] = {
					-size, -size, -size,
					-size, -size, size,
					size, -size, size,
					size, -size, -size,
					-size, size, -size,
					-size, size, size,
					size, size, size,
					size, size, -size,
				};

				unsigned short indices[] = {
					0, 1, 1, 2, 2, 3, 3, 0,
					0, 4, 4, 5, 5, 1, 1, 2,
					2, 6, 6, 5, 5, 4, 4, 7,
					7, 3, 3, 2, 2, 6, 6, 7
				};

				VertexAttribute position = {};
				position.count = 3;
				position.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				position.offset = 0;

				VertexInputDesc desc = {};
				desc.attribs = { position };
				desc.stride = 3 * sizeof(float);

				Buffer *vb = renderer->CreateVertexBuffer(vertices, sizeof(vertices), BufferUsage::STATIC);
				Buffer *ib = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

				Mesh m = {};
				m.indexCount = 32;

				if (instanced)
				{
					VertexAttribute attribs[5] = {};

					// Instance mat4 attrib
					attribs[0].count = 4;
					attribs[1].count = 4;
					attribs[2].count = 4;
					attribs[3].count = 4;

					attribs[0].offset = 0;
					attribs[1].offset = 4 * sizeof(float);
					attribs[2].offset = 8 * sizeof(float);
					attribs[3].offset = 12 * sizeof(float);

					// Instance color attrib
					attribs[4].count = 3;
					attribs[4].offset = 16 * sizeof(float);

					VertexInputDesc instDesc = {};
					instDesc.stride = 19 * sizeof(float);
					instDesc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3], attribs[4] };
					instDesc.instanced = true;

					VertexInputDesc descs[2] = {desc,instDesc };

					m.vao = renderer->CreateVertexArray(descs, 2, { vb }, ib);
				}
				else
				{
					m.vao = renderer->CreateVertexArray(desc, vb, ib);
				}			

				return m;
			}
			else
			{
				float vertices[] =
				{
					size,  size, -size, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
					size, -size, -size, 0.0f, 1.0f,  0.0f,  0.0f, -1.0f,
					-size, -size, -size, 1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
					-size,  size, -size, 1.0f, 0.0f,  0.0f,  0.0f, -1.0f,

					-size, -size,  size, 0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
					size, -size,  size, 1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
					size,  size,  size, 1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
					-size,  size,  size, 0.0f, 1.0f,  0.0f,  0.0f,  1.0f,

					-size,  size,  size, 1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
					-size,  size, -size, 1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
					-size, -size, -size, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
					-size, -size,  size, 0.0f, 1.0f, -1.0f,  0.0f,  0.0f,

					size, -size,  -size, 0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
					size, size, -size, 1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
					size,  size,  size, 1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
					size, -size,  size, 0.0f, 1.0f,  1.0f,  0.0f,  0.0f,

					-size, -size, -size, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
					size, -size, -size, 1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
					size, -size,  size, 1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
					-size, -size,  size, 0.0f, 1.0f,  0.0f, -1.0f,  0.0f,

					size,  size,  size, 1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
					size,  size, -size, 1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
					-size,  size, -size, 0.0f, 0.0f,  0.0f,  1.0f,  0.0f,		
					-size,  size,  size, 0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
				};


				// Needs to use indices because the Vita only supports indexed draws
				unsigned short indices[] =
				{
					0,1,2,2,3,0,
					4,5,6,6,7,4,
					8,9,10,10,11,8,
					12,13,14,14,15,12,
					16,17,18,18,19,16,
					20,21,22,22,23,20
				};

				VertexAttribute position = {};
				position.count = 3;
				position.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				position.offset = 0;
				VertexAttribute uv = {};
				uv.count = 2;
				uv.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				uv.offset = 3 * sizeof(float);
				VertexAttribute normal = {};
				normal.count = 3;
				normal.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				normal.offset = 5 * sizeof(float);

				VertexInputDesc desc = {};
				desc.attribs = { position, uv, normal };
				desc.stride = 8 * sizeof(float);

				Buffer *vb = renderer->CreateVertexBuffer(vertices, sizeof(vertices), BufferUsage::STATIC);
				Buffer *ib = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

				Mesh m = {};
				m.indexCount = 36;

				/*if (instanced)
				{
					VertexAttribute attribs[5] = {};

					// Instance mat4 attrib
					attribs[0].count = 4;
					attribs[1].count = 4;
					attribs[2].count = 4;
					attribs[3].count = 4;

					attribs[0].offset = 0;
					attribs[1].offset = 4 * sizeof(float);
					attribs[2].offset = 8 * sizeof(float);
					attribs[3].offset = 12 * sizeof(float);

					// Instance color attrib
					attribs[4].count = 3;
					attribs[4].offset = 16 * sizeof(float);

					VertexInputDesc instDesc = {};
					instDesc.stride = 19 * sizeof(float);
					instDesc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3], attribs[4] };
					instDesc.instanced = true;

					VertexInputDesc descs[2] = { desc,instDesc };

					m.vao = renderer->CreateVertexArray(descs, 2, { vb }, ib);
				}
				else
				{*/
					m.vao = renderer->CreateVertexArray(desc, vb, ib);
				//}

				return m;
			}
		}

		Mesh CreateLine(Renderer *renderer, float size)
		{
			/*float lineVertices[] = {
			0.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f
			};

			glGenVertexArrays(1, &debugLineVAO);
			glBindVertexArray(debugLineVAO);
			glGenBuffers(1, &debugLineVBO);
			glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
			glBindVertexArray(0);*/
			return Mesh();
		}

		Mesh CreateSphere(Renderer *renderer, float radius, bool instanced)
		{
			int latBands = 16;
			int longBands = 24;		

			if (instanced)
			{
				std::vector<glm::vec3> vertices;
				std::vector<unsigned short> indices;

				for (int y = 0; y <= latBands; y++)
				{
					float ySegment = (float)y / latBands;
					float theta = ySegment * 3.14159265f;
					float sinTheta = glm::sin(theta);
					float cosTheta = glm::cos(theta);

					for (int x = 0; x <= longBands; x++)
					{
						float xSegment = (float)x / longBands;
						float phi = xSegment * 2.0f * 3.14159365f;
						float sinPhi = glm::sin(phi);
						float cosPhi = glm::cos(phi);

						glm::vec3 v;
						v.x = radius * cosPhi * sinTheta;
						v.y = radius * cosTheta;
						v.z = radius * sinPhi * sinTheta;
						vertices.push_back(v);
					}
				}

				for (short y = 0; y < latBands; y++)
				{
					for (short x = 0; x < longBands; x++)
					{
						short first = (y * (longBands + 1)) + x;
						short second = first + longBands + 1;

						indices.push_back(first + 1);
						indices.push_back(second);
						indices.push_back(first);

						indices.push_back(first + 1);
						indices.push_back(second + 1);
						indices.push_back(second);
					}
				}

				VertexAttribute position = {};
				position.count = 3;
				position.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				position.offset = 0;

				VertexInputDesc desc = {};
				desc.attribs = { position };
				desc.stride = 3 * sizeof(float);

				Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(glm::vec3), BufferUsage::STATIC);
				Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

				VertexAttribute attribs[5] = {};

				// Instance mat4 attrib
				attribs[0].count = 4;
				attribs[1].count = 4;
				attribs[2].count = 4;
				attribs[3].count = 4;

				attribs[0].offset = 0;
				attribs[1].offset = 4 * sizeof(float);
				attribs[2].offset = 8 * sizeof(float);
				attribs[3].offset = 12 * sizeof(float);

				// Instance color attrib
				attribs[4].count = 3;
				attribs[4].offset = 16 * sizeof(float);

				VertexInputDesc instDesc = {};
				instDesc.stride = 19 * sizeof(float);
				instDesc.attribs = { attribs[0], attribs[1], attribs[2], attribs[3], attribs[4] };
				instDesc.instanced = true;

				VertexInputDesc descs[2] = { desc,instDesc };

				Mesh m = {};
				m.vao = renderer->CreateVertexArray(descs, 2, { vb }, ib);
				m.indexCount = indices.size();			

				return m;
			}
			else
			{
				std::vector<VertexPOS3D_UV_NORMAL> vertices;
				std::vector<unsigned short> indices;

				for (int y = 0; y <= latBands; y++)
				{
					float ySegment = (float)y / latBands;
					float theta = ySegment * 3.14159265f;
					float sinTheta = glm::sin(theta);
					float cosTheta = glm::cos(theta);

					for (int x = 0; x <= longBands; x++)
					{
						float xSegment = (float)x / longBands;
						float phi = xSegment * 2.0f * 3.14159365f;
						float sinPhi = glm::sin(phi);
						float cosPhi = glm::cos(phi);

						VertexPOS3D_UV_NORMAL v = {};
						v.pos.x = radius * cosPhi * sinTheta;
						v.pos.y = radius * cosTheta;
						v.pos.z = radius * sinPhi * sinTheta;
						v.uv.x = xSegment;
						v.uv.y = ySegment;
						v.normal.x = v.pos.x;
						v.normal.y = v.pos.y;
						v.normal.z = v.pos.z;
						vertices.push_back(v);
					}
				}

				for (short y = 0; y < latBands; y++)
				{
					for (short x = 0; x < longBands; x++)
					{
						short first = (y * (longBands + 1)) + x;
						short second = first + longBands + 1;

						indices.push_back(first + 1);
						indices.push_back(second);
						indices.push_back(first);

						indices.push_back(first + 1);
						indices.push_back(second + 1);
						indices.push_back(second);
					}
				}

				VertexAttribute position = {};
				position.count = 3;
				position.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				position.offset = 0;

				VertexAttribute uv = {};
				uv.count = 2;
				uv.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				uv.offset = 3 * sizeof(float);

				VertexAttribute normal = {};
				normal.count = 3;
				normal.vertexAttribFormat = VertexAttributeFormat::FLOAT;
				normal.offset = 5 * sizeof(float);

				VertexInputDesc desc = {};
				desc.attribs = { position, uv, normal };
				desc.stride = 8 * sizeof(float);

				Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(VertexPOS3D_UV_NORMAL), BufferUsage::STATIC);
				Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

				Mesh m = {};
				m.indexCount = indices.size();

				m.vao = renderer->CreateVertexArray(desc, vb, ib);

				return m;
			}
		}

		Mesh CreateQuad(Renderer *renderer, float size)
		{
			VertexAttribute posUv = {};
			posUv.count = 4;
			posUv.vertexAttribFormat = VertexAttributeFormat::FLOAT;
			posUv.offset = 0;

			VertexInputDesc desc = {};
			desc.attribs = { posUv };
			desc.stride = 4 * sizeof(float);

			float vertices[] = {
				-size, -size, 0.0f, 0.0f,
				size, -size, 1.0f, 0.0f,
				-size, size, 0.0f, 1.0f,
				size, size, 1.0f, 1.0f
			};
			unsigned short indices[] = {
				0,1,2, 2,1,3
			};

			Buffer *vb = renderer->CreateVertexBuffer(vertices, sizeof(vertices), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

			Mesh m = {};
			m.vao = renderer->CreateVertexArray(desc, vb, ib);
			m.indexCount = 6;

			return m;
		}

		Mesh CreatePlane(Renderer *renderer, float size)
		{
			VertexAttribute attribs[3] = {};
			// Position
			attribs[0].count = 3;
			attribs[0].vertexAttribFormat = VertexAttributeFormat::FLOAT;
			attribs[0].offset = 0;
			// Uv
			attribs[1].count = 2;
			attribs[1].vertexAttribFormat = VertexAttributeFormat::FLOAT;
			attribs[1].offset = 3 * sizeof(float);
			// Normal
			attribs[2].count = 3;
			attribs[2].vertexAttribFormat = VertexAttributeFormat::FLOAT;
			attribs[2].offset = 5 * sizeof(float);

			VertexInputDesc desc = {};
			desc.attribs = { attribs[0], attribs[1], attribs[2] };
			desc.stride = 8 * sizeof(float);

			float vertices[] = {
				-size, 0.0f, -size, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
				-size, 0.0f, size, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
				size, 0.0f, size, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
				size, 0.0f, -size, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f
			};
			unsigned short indices[] = {
				0,1,2, 0,2,3
			};

			Buffer *vb = renderer->CreateVertexBuffer(vertices, sizeof(vertices), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices, sizeof(indices), BufferUsage::STATIC);

			Mesh m = {};
			m.vao = renderer->CreateVertexArray(desc, vb, ib);
			m.indexCount = 6;

			return m;
		}

		Mesh CreateScreenSpaceGrid(Renderer *renderer, unsigned int resolution)
		{
			VertexAttribute attrib = {};
			// UV
			attrib.count = 2;
			attrib.vertexAttribFormat = VertexAttributeFormat::FLOAT;
			attrib.offset = 0;

			VertexInputDesc desc = {};
			desc.attribs = { attrib };
			desc.stride = 2 * sizeof(float);

			std::vector<glm::vec2> vertices(resolution * resolution);
			std::vector<unsigned short> indices((resolution - 1) * (resolution - 1) * 6);

			unsigned int v = 0;
			for (unsigned int z = 0; z < resolution; z++)
			{
				for (unsigned int x = 0; x < resolution; x++)
				{
					vertices[v++] = glm::vec2((float)x / (resolution - 1), (float)z / (resolution - 1));
				}
			}

			unsigned int index = 0;
			for (unsigned int z = 0; z < resolution - 1; z++)
			{
				for (unsigned int x = 0; x < resolution - 1; x++)
				{
					unsigned short topLeft = z * (unsigned short)resolution + x;
					unsigned short topRight = topLeft + 1;
					unsigned short bottomLeft = (z + 1) * (unsigned short)resolution + x;
					unsigned short bottomRight = bottomLeft + 1;

					indices[index++] = topLeft;
					indices[index++] = bottomLeft;
					indices[index++] = bottomRight;

					indices[index++] = topLeft;
					indices[index++] = bottomRight;
					indices[index++] = topRight;
				}
			}


			Buffer *vb = renderer->CreateVertexBuffer(vertices.data(), vertices.size() * sizeof(glm::vec2), BufferUsage::STATIC);
			Buffer *ib = renderer->CreateIndexBuffer(indices.data(), indices.size() * sizeof(unsigned short), BufferUsage::STATIC);

			Mesh m = {};
			m.vao = renderer->CreateVertexArray(desc, vb, ib);
			m.indexCount = indices.size();

			return m;
		}

		Mesh CreateFromVertices(Renderer *renderer, const VertexInputDesc &desc, unsigned int vertexCount, const void *vertices, unsigned int verticesSize, unsigned int indexCount, const unsigned short *indices, unsigned int indexSize)
		{
			Buffer *vb = renderer->CreateVertexBuffer(vertices, verticesSize, BufferUsage::STATIC);

			Buffer *ib = nullptr;

			if (indices)
				ib = renderer->CreateIndexBuffer(indices, indexSize, BufferUsage::STATIC);

			Mesh m = {};
			m.vao = renderer->CreateVertexArray(desc, vb, ib);
			
			if (indices)
				m.indexCount = indexCount;
			else
				m.vertexCount = vertexCount;

			return m;
		}
	}
}
