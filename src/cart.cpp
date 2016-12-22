#include <algorithm>
#include <iostream>
#include <array>
#include <vector>

#include "cart.hpp"
#include "file.hpp"
// #include "sha1.hpp"


Cart::Cart(const std::string inFile, std::vector<uint8_t> &prgRom)
{
	std::vector<uint8_t> fileContent = FileToU8Vec(inFile);

	//ines header stuff
	std::copy_n(fileContent.begin(), 16, header.begin());

	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A)
	{
		std::cout << "Not a valid .nes file" << std::endl;
		exit(0);
	}
	fileContent.erase(fileContent.begin(), fileContent.begin() + 0x10);

	mapper = (header[6] >> 4) | (header[7] & 0b11110000);
	if(mapper == 0 || mapper == 2 || mapper == 3 || mapper == 7)
	{
		prgRom.assign(fileContent.begin(), fileContent.begin() + header[4] * 0x4000);
		SetDefaultPrgBanks(prgRom);

		SetChrMem(fileContent);
		SetDefaultNametableLayout();
	}
	else
	{
		std::cout << "unsupported mapper\n" << +mapper;
		exit(0);
	}
}


void Cart::SetDefaultPrgBanks(std::vector<uint8_t> &prgRom)
{
	if(mapper == 0 || mapper == 3)
	{
		pPrgBank[0] = &prgRom[0];
		pPrgBank[1] = &prgRom[0x2000];
		if(header[4] > 1)
		{
			pPrgBank[2] = &prgRom[0x4000];
			pPrgBank[3] = &prgRom[0x6000];
		}
		else
		{
			pPrgBank[2] = &prgRom[0x0000];
			pPrgBank[3] = &prgRom[0x2000];
		}
	}
	else if(mapper == 2)
	{
		pPrgBank[0] = &prgRom[0];
		pPrgBank[1] = &prgRom[0x2000];
		pPrgBank[2] = &prgRom[(header[4] - 1) * 0x4000];
		pPrgBank[3] = pPrgBank[2] + 0x2000;
	}
	else if(mapper == 7)
	{
		pPrgBank[0] = &prgRom[0];
		pPrgBank[1] = &prgRom[0x2000];
		pPrgBank[2] = &prgRom[0x4000];
		pPrgBank[3] = &prgRom[0x6000];
	}
}


void Cart::SetChrMem(const std::vector<uint8_t> &fileContent)
{
	if(mapper == 0 || mapper == 2 || mapper == 3)
	{
		if(header[5])
		{
			chrMem.assign(fileContent.begin() + header[4] * 0x4000, fileContent.begin() + header[4] * 0x4000 + header[5] * 0x2000);
			isChrRam = false;
		}
		else
		{
			chrMem.resize(0x2000);
			isChrRam = true;
		}
	}
	else if(mapper == 7)
	{
		chrMem.resize(0x2000);
		isChrRam = true;
	}
}


void Cart::SetDefaultNametableLayout()
{
	if(mapper == 0 || mapper == 2 || mapper == 3)
	{
		if(header[6] & 1)
		{
			nametableOffsets = {A, B, A, B}; //horizontal arrangement
		}
		else
		{
			nametableOffsets = {A, A, C, C}; //vertical arrangement
		}
	}
	else if(mapper == 7)
	{
		nametableOffsets = {A, A, A, A}; // single screen
	}
}
