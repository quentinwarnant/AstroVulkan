#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace FileHelpers
{
	static std::vector<char> ReadFile(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) 
		{
			throw std::runtime_error("failed to open file!");
		}

		// Find size of file, allocate buffer
		size_t fileSize = (size_t) file.tellg();
		std::vector<char> buffer(fileSize);

		// Go back to the start of the file & fill the buffer with the file data
		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

};