#include <algorithm>
#include <iostream>
#include <array>
#include <vector>

#include "cart.hpp"
#include "file.hpp"
#include "sha1.hpp"


Cart::Cart(const std::string inFile, std::vector<uint8_t> &prgRom, std::vector<uint8_t> &prgRam)
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


	const std::array<uint32_t, 5> sha1 = SHA1(fileContent);
	if(cartInfo.count(sha1))
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
				pPrgRamBank[x] = prgRam.data() + (x & (attr.wram >> 1) - 1);
			}
		}

		if(!attr.isChrRam)
		{
			const auto chrRomEnd = prgRomEnd + attr.chr * 1024;
			chrMem.assign(prgRomEnd, chrRomEnd);
		}
		else
		{
			chrMem.resize(attr.chr * 1024);
		}

		switch(attr.nametableLayout)
		{
			case vertical  : nametableOffsets = {A, A, B, B}; break;
			case horizontal: nametableOffsets = {A, B, A, B}; break;
			case single    : nametableOffsets = {A, A, A, A}; break;
		}
	}
	else
	{
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
}


void Cart::SetDefaultPrgBanksSha(std::vector<uint8_t> &prgRom, cartAttributes attr)
{
	switch(attr.type)
	{
		case NES_NROM: case NES_SGROM: case NES_SLROM: case NES_UNROM: case NES_CNROM: case KONAMI_VRC_4:
			pPrgBank[0] = prgRom.data();
			pPrgBank[1] = prgRom.data() + 0x2000;
			pPrgBank[2] = prgRom.data() + (attr.prg - 16) * 1024;
			pPrgBank[3] = prgRom.data() + (attr.prg - 8) * 1024;
		break;

		case NES_AOROM:
			pPrgBank[0] = prgRom.data();
			pPrgBank[1] = prgRom.data() + 0x2000;
			pPrgBank[2] = prgRom.data() + 0x4000;
			pPrgBank[3] = prgRom.data() + 0x6000;
		break;
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
			nametableOffsets = {A, A, B, B}; //vertical arrangement
		}
	}
	else if(mapper == 7)
	{
		nametableOffsets = {A, A, A, A}; // single screen
	}
}
