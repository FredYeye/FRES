#include <algorithm>
#include <iostream>
#include <array>
#include <vector>

#include "cart.hpp"
#include "file.hpp"
#include "sha1.hpp"


Cart::Cart(const std::string inFile, std::vector<uint8_t> &prgRom, std::vector<uint8_t> &prgRam) : prgRom(prgRom), prgRam(prgRam)
{
	fileContent = FileToU8Vec(inFile);
	if(fileContent.size() < 512) //just some number
	{
		std::cout << "File is too small to be a nes rom\n";
		exit(0);
	}

	//ines header
	std::copy_n(fileContent.begin(), 16, header.begin());
	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A)
	{
		std::cout << "Not a valid .nes file" << std::endl;
		exit(0);
	}
	fileContent.erase(fileContent.begin(), fileContent.begin() + 0x10);


	const std::array<uint32_t, 5> sha1 = SHA1(fileContent);
	if(cartInfo.count(sha1))
	{
		GameInfoSha(sha1);
	}
	else
	{
		mapper = (header[6] >> 4) | (header[7] & 0b11110000);

		switch(mapper)
		{
			case 0: type = NROM; break;
			case 1: type = SFROM; break;
			case 2: type = UNROM; break;
			case 3: type = CNROM; break;
			case 4: type = TGROM; break;
			case 7: type = AOROM; break;
			case 21: type = VRC_4; break;
		}


		if(mapper == 0 || mapper == 1 || mapper == 2 || mapper == 3 || mapper == 4 || mapper == 7)
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
}


void Cart::GameInfoSha(std::array<uint32_t, 5> sha1)
{
	std::cout << "Game recognized\n";
	const cartAttributes attr = cartInfo.at(sha1);
	type = attr.type;

	const auto prgRomEnd = fileContent.begin() + attr.prg * 1024;
	prgRom.assign(fileContent.begin(), prgRomEnd);
	SetDefaultPrgBanksSha(prgRom, attr);

	if(attr.wram)
	{
		prgRam.resize(attr.wram * 1024);

		for(int x = 0; x < 4; ++x)
		{
			pPrgRamBank[x] = prgRam.data() + (x & (attr.wram >> 1) - 1) * 2048;
		}
	}

	if(attr.chrType == ChrRom)
	{
		const auto chrRomEnd = prgRomEnd + attr.chr * 1024;
		chrMem.assign(prgRomEnd, chrRomEnd);
	}
	else
	{
		chrMem.resize(attr.chr * 1024);
	}
	chrType = attr.chrType;

	switch(attr.nametableLayout)
	{
		case vertical  : nametableOffsets = {A, A, B, B}; break;
		case horizontal: nametableOffsets = {A, B, A, B}; break;
		case single    : nametableOffsets = {A, A, A, A}; break;
	}
}


void Cart::SetDefaultPrgBanksSha(std::vector<uint8_t> &prgRom, cartAttributes attr)
{
	switch(attr.type)
	{
		case NROM: case SFROM: case UNROM: case CNROM: case TGROM: case VRC_4:
			pPrgBank[0] = prgRom.data();
			pPrgBank[1] = prgRom.data() + 0x2000;
			pPrgBank[2] = prgRom.data() + (attr.prg - 16) * 1024;
			pPrgBank[3] = prgRom.data() + (attr.prg - 8) * 1024;
		break;

		case AOROM:
			pPrgBank[0] = prgRom.data();
			pPrgBank[1] = prgRom.data() + 0x2000;
			pPrgBank[2] = prgRom.data() + 0x4000;
			pPrgBank[3] = prgRom.data() + 0x6000;
		break;
	}
}


void Cart::SetDefaultPrgBanks(std::vector<uint8_t> &prgRom)
{
	if(mapper == 0 || mapper == 1 || mapper == 2 || mapper == 3 || mapper == 4 || mapper == 21)
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
	if(mapper == 0 || mapper == 1 || mapper == 2 || mapper == 3 || mapper == 4)
	{
		if(header[5])
		{
			chrMem.assign(fileContent.begin() + header[4] * 0x4000, fileContent.begin() + header[4] * 0x4000 + header[5] * 0x2000);
			chrType = ChrRom;
		}
		else
		{
			chrMem.resize(0x2000);
			chrType = ChrRam;
		}
	}
	else if(mapper == 7)
	{
		chrMem.resize(0x2000);
		chrType = ChrRam;
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
			nametableOffsets = {A, A, B, B}; //vertical arrangement
		}
	}
	else if(mapper == 4)
	{
		nametableOffsets = {A, A, B, B}; //vertical arrangement
	}
	else if(mapper == 1 || mapper == 7)
	{
		nametableOffsets = {A, A, A, A}; // single screen
	}
}
