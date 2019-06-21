#include "Serializer.h"

#include "Program/FileManager.h"

namespace Engine
{
	Serializer::Serializer(FileManager *fileManager)
	{
		this->fileManager = fileManager;
		dataSize = 0;
		data = nullptr;
		pos = 0;
	}

	void Serializer::OpenForWriting()
	{
		dataSize = 128;
		data = new char[dataSize];
	}

	void Serializer::OpenForReading(const std::string &filename)
	{
		std::ifstream file = fileManager->OpenForReading(filename, std::ios::ate | std::ios::binary);
		if (file.is_open())
		{
			dataSize = (size_t)file.tellg();
			file.seekg(0, file.beg);
			data = new char[dataSize];
			file.read(data, dataSize);
			file.close();
		}
	}

	void Serializer::Save(const std::string &filename)
	{
		if (pos <= 0)
			return;

		std::ofstream file = fileManager->OpenForWriting(filename, std::ios::binary);
		if (file.is_open())
		{
			file.write(data, (std::streamsize)pos);
			file.close();
		}
	}

	bool Serializer::IsOpen()
	{
		if (data)
			return true;

		return false;
	}

	bool Serializer::EndReached()
	{
		if (data && dataSize > 0 && pos > 0 && dataSize == pos)
			return true;

		return false;
	}

	void Serializer::Close()
	{
		if (data)
		{
			delete[] data;
			data = nullptr;
		}
	}

	void Serializer::Write(bool data)
	{
		write((uint32_t)(data ? 1 : 0));
	}

	void Serializer::Write(short data)
	{
		write(data);
	}

	void Serializer::Write(unsigned short data)
	{
		write(data);
	}

	void Serializer::Write(int data)
	{
		write(data);
	}

	void Serializer::Write(unsigned int data)
	{
		write(data);
	}

	void Serializer::Write(float data)
	{
		write(data);
	}

	void Serializer::Write(const glm::vec2 &data)
	{
		write(data);
	}

	void Serializer::Write(const glm::vec3 &data)
	{
		write(data);
	}

	void Serializer::Write(const glm::vec4 &data)
	{
		write(data);
	}

	void Serializer::Write(const glm::quat &data)
	{
		write(data);
	}

	void Serializer::Write(const glm::mat4 &data)
	{
		write(data);
	}

	void Serializer::Write(const glm::mat3 &data)
	{
		write(data);
	}

	void Serializer::Write(const std::string &data)
	{
		unsigned int length = (unsigned int)(data.length() + 1);
		write(length);
		write(*data.c_str(), length);
	}

	void Serializer::Write(const char *data)
	{
		unsigned int length = strlen(data);
		write(length);
		write(*data, length);
	}

	void Serializer::Write(const void *data, unsigned int size)
	{
		size_t newSize = pos + (size_t)size;

		// Reallocate when newSize exceeds dataSize
		if (newSize > dataSize)
		{
			char *newData = new char[newSize * 2];
			std::memcpy(newData, this->data, dataSize);
			dataSize = newSize * 2;
			delete[] this->data;
			this->data = newData;
		}

		std::memcpy(this->data + pos, data, size);
		pos = newSize;
	}

	void Serializer::Read(bool &data)
	{
		uint32_t temp;
		read(temp);
		data = temp == 1;
	}

	void Serializer::Read(short &data)
	{
		short temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(unsigned short &data)
	{
		unsigned short temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(int &data)
	{
		int temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(unsigned int &data)
	{
		unsigned int temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(float &data)
	{
		float temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::vec2 &data)
	{
		glm::vec2 temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::vec3 &data)
	{
		glm::vec3 temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::vec4 &data)
	{
		glm::vec4 temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::quat &data)
	{
		glm::quat temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::mat4 &data)
	{
		glm::mat4 temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(glm::mat3 &data)
	{
		glm::mat3 temp;
		read(temp);
		data = temp;
	}

	void Serializer::Read(std::string &data)
	{
		unsigned int length;
		read(length);
		char *str = new char[length];
		memset(str, '\0', (size_t)(sizeof(char) * length));
		read(*str, length);
		data = std::string(str);
		delete[] str;
	}

	void Serializer::Read(char *data)
	{
		unsigned int length;
		read(length);
		memset(data, '\0', (size_t)(sizeof(char) * length));
		read(*data, length);
	}

	void Serializer::Read(void *data, unsigned int size)
	{
		std::memcpy(data, static_cast<void*>(this->data + pos), (size_t)size);
		pos += size;
	}
}
