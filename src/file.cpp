#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "file.hpp"


const std::vector<uint8_t> FileToU8Vec(const std::string inFile)
{
	std::ifstream iFile(inFile.c_str(), std::ios::in | std::ios::ate | std::ios::binary);
	if(iFile.is_open() == false)
	{
		std::cout << "File not found" << std::endl;
		exit(0);
	}

	const uint32_t size = iFile.tellg();
	const std::vector<uint8_t> content(size);
	iFile.seekg(0, std::ios::beg);
	iFile.read((char*)content.data(), size);
	iFile.close();

	return content;
}
