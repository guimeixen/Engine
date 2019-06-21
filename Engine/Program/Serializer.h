#pragma once

#include "include/glm/glm.hpp"
#include "include/glm/gtc/quaternion.hpp"

#include <string>

namespace Engine
{
	class FileManager;

	class Serializer
	{
	public:
		Serializer(FileManager *fileManager);

		void OpenForWriting();
		void OpenForReading(const std::string &filename);
		void Save(const std::string &filename);
		bool IsOpen();
		bool EndReached();
		void Close();

		char *GetData() const { return data; }
		size_t GetDataSize() const { return pos; }

		void Write(bool data);
		void Write(short data);
		void Write(unsigned short data);
		void Write(int data);
		void Write(unsigned int data);
		void Write(float data);
		void Write(const glm::vec2 &data);
		void Write(const glm::vec3 &data);
		void Write(const glm::vec4 &data);
		void Write(const glm::quat &data);
		void Write(const glm::mat4 &data);
		void Write(const glm::mat3 &data);
		void Write(const std::string &data);
		void Write(const char *data);
		void Write(const void *data, unsigned int size);

		void Read(bool &data);
		void Read(short &data);
		void Read(unsigned short &data);
		void Read(int &data);
		void Read(unsigned int &data);
		void Read(float &data);
		void Read(glm::vec2 &data);
		void Read(glm::vec3 &data);
		void Read(glm::vec4 &data);
		void Read(glm::quat &data);
		void Read(glm::mat4 &data);
		void Read(glm::mat3 &data);
		void Read(std::string &data);
		void Read(char *data);					// Assumes data as enough size to hold the serialized string
		void Read(void *data, unsigned int size);

	private:
		template<typename T>
		void write(const T &data, unsigned int count = 1)
		{
			size_t size = (size_t)(sizeof(data) * count);
			size_t newSize = pos + size;

			// Reallocate when newSize exceeds dataSize
			if (newSize > dataSize)
			{
				char *newData = new char[newSize * 2];
				std::memcpy(newData, this->data, dataSize);
				dataSize = newSize * 2;
				delete[] this->data;
				this->data = newData;
			}

			std::memcpy(static_cast<void*>(this->data + pos), &data, size);
			pos = newSize;
		}

		template<typename T>
		void read(T &data, unsigned int count = 1)
		{
			std::memcpy(&data, static_cast<void*>(this->data + pos), (size_t)(sizeof(data) * count));
			pos += (size_t)(sizeof(data) * count);
		}

	private:
		FileManager *fileManager;
		size_t dataSize;
		char *data;
		size_t pos;
	};
}
