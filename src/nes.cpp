// #define DEBUG
// #define DUMP_VRAM

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>

#include "nes.hpp"
#include "apu.hpp"
#include "ppu.hpp"
#include "file.hpp"


Nes::Nes(std::string inFile)
{
	LoadRom(inFile);

	CpuRead(PC); //PC?
	CpuRead(PC);
	CpuRead(PC);
	CpuRead(0x100 | rS--);
	CpuRead(0x100 | rS--);
	CpuRead(0x100 | rS--);
	CpuRead(0xFFFC);
	tempData = dataBus;

	rP = 0x36;
	CpuRead(0xFFFD);

	PC = tempData | (dataBus << 8);
	CpuRead(PC);
}


void Nes::AdvanceFrame(uint8_t input)
{
	while(!ppu.RenderFrame())
	{
		RunOpcode();

		if(readJoy1)
		{
			controller_reg = input;
		}
	}
}


void Nes::LoadRom(std::string inFile)
{
	std::vector<uint8_t> fileContent = FileToU8Vec(inFile);

	//ines header stuff
	std::array<uint8_t, 16> header;
	std::copy_n(fileContent.begin(), 16, header.begin());

	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A)
	{
		std::cout << "Not a valid .nes file" << std::endl;
		exit(0);
	}
	fileContent.erase(fileContent.begin(), fileContent.begin() + 0x10);

	mapper = (header[6] >> 4) | (header[7] & 0b11110000);
	if(mapper == 0)
	{
		prgRom.assign(fileContent.begin(), fileContent.begin() + header[4] * 0x4000);

		bankPtr[0] = &prgRom[0];
		bankPtr[1] = &prgRom[0x2000];
		if(header[4] > 1)
		{
			bankPtr[2] = &prgRom[0x4000];
			bankPtr[3] = &prgRom[0x6000];
		}
		else
		{
			bankPtr[2] = &prgRom[0x0000];
			bankPtr[3] = &prgRom[0x2000];
		}

		if(header[5])
		{
			chrRom.assign(fileContent.begin() + header[4] * 0x4000, fileContent.begin() + header[4] * 0x4000 + header[5] * 0x2000);
			std::memcpy(ppu.GetVramPtr(), &chrRom[0], 0x2000);
		}

		if(header[6] & 1)
		{
			ppu.SetNametableMirroring(0x800, 0);
		}
		else
		{
			ppu.SetNametableMirroring(0x400, 0);
		}
	}
	else if(mapper == 2)
	{
		prgRom.assign(fileContent.begin(), fileContent.begin() + header[4] * 0x4000);

		bankPtr[0] = &prgRom[0];
		bankPtr[1] = &prgRom[0x2000];
		bankPtr[2] = &prgRom[(header[4] - 1) * 0x4000];
		bankPtr[3] = bankPtr[2] + 0x2000;

		if(header[5])
		{
			chrRom.assign(fileContent.begin() + header[4] * 0x4000, fileContent.begin() + header[4] * 0x4000 + header[5] * 0x2000);
			std::memcpy(ppu.GetVramPtr(), &chrRom[0], 0x2000);
		}

		if(header[6] & 1)
		{
			ppu.SetNametableMirroring(0x800, 0);
		}
		else
		{
			ppu.SetNametableMirroring(0x400, 0);
		}
	}
	else if(mapper == 7)
	{
		prgRom.assign(fileContent.begin(), fileContent.begin() + header[4] * 0x4000);

		bankPtr[0] = &prgRom[0];
		bankPtr[1] = &prgRom[0x2000];
		bankPtr[2] = &prgRom[0x4000];
		bankPtr[3] = &prgRom[0x6000];

		ppu.SetNametableMirroring(0xC00, 0);
	}
	else
	{
		std::cout << "unsupported mapper\n";
		exit(0);
	}
}


void Nes::RunOpcode()
{
	const uint8_t opcode = dataBus;


	#ifdef DEBUG
		DebugCpu(opcode);
	#endif
	#ifdef DUMP_VRAM
		if(ppu.GetScanlineV() == 261 && ppu.GetScanlineH() == 0)
		{
			std::string outfile = "vram.txt";
			std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
			result.write((char*)&ppu.GetVram()[0],0x4000);
			result.close();
		}
	#endif


	CpuRead(++PC); //fetch op1

	const uint8_t op1 = dataBus;

	switch(opcode)
	{
		// opcodes left
		// 4B ALR
		// 6B ARR
		// 8B XAA
		// 93 AHX
		// 9B TAS
		// 9C SHY
		// 9E SHX
		// 9F AHX
		// BB LAS

		case 0x00: //BRK
			++PC;
			CpuWrite(0x0100 | rS--, PC >> 8);
			CpuWrite(0x0100 | rS--, PC);
			CpuWrite(0x0100 | rS--, rP.to_ulong() | 0b00010000);

			rP.set(2);
			CpuRead(0xFFFE);
			tempData = dataBus;
			CpuRead(0xFFFF);

			PC = tempData | (dataBus << 8);
			break;

		case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52: //KIL
		case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
			std::cout << "CRASH" << std::endl;
			exit(0);
			break;

		case 0x08: //PHP
			CpuWrite(0x0100 | rS--, rP.to_ulong() | 0b00010000);
			break;
		case 0x28: //PLP
			CpuRead(0x0100 | rS++);
			CpuRead(0x0100 | rS);
			rP = dataBus | 0x20;
			break;
		case 0x48: //PHA
			CpuWrite(0x0100 | rS--, rA);
			break;
		case 0x68: //PLA
			CpuRead(0x0100 | rS++);
			CpuRead(0x0100 | rS);

			rA = dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x0A: //ASL acc
			rP[0] = rA & 0x80;
			rA <<= 1;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x2A: //ROL acc
			{
			const bool newCarry = rA & 0x80;
			rA = (rA << 1) | rP[0];
			rP[0] = newCarry;
			}
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x4A: //LSR acc
			rP[0] = rA & 0x01;
			rA >>= 1;
			rP[1] = !rA;
			rP.reset(7);
			break;
		case 0x6A: //ROR acc
			{
			const bool newCarry = rA & 0x01;
			rA = (rA >> 1) | (rP[0] << 7);
			rP[0] = newCarry;
			}
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x10: //BPL
			++PC;
			if(!rP.test(7))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0x30: //BMI
			++PC;
			if(rP.test(7))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0x50: //BVC
			++PC;
			if(!rP.test(6))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0x70: //BVS
			++PC;
			if(rP.test(6))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0x90: //BCC
			++PC;
			if(!rP.test(0))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0xB0: //BCS
			++PC;
			if(rP.test(0))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0xD0: //BNE
			++PC;
			if(!rP.test(1))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;
		case 0xF0: //BEQ
			++PC;
			if(rP.test(1))
			{
				const uint16_t pagePC = PC + int8_t(op1);
				PC = (PC & 0xFF00) | (pagePC & 0x00FF);
				CpuRead(PC);
				if(PC != pagePC)
				{
					PC = pagePC;
					CpuRead(PC);
				}
			}
			break;

		case 0x18: //CLC
			rP.reset(0);
			break;
		case 0x38: //SEC
			rP.set(0);
			break;
		case 0x58: //CLI
			rP.reset(2);
			break;
		case 0x78: //SEI
			rP.set(2);
			break;
		case 0xB8: //CLV
			rP.reset(6);
			break;
		case 0xD8: //CLD
			rP.reset(3);
			break;
		case 0xF8: //SED
			rP.set(3);
			break;

		case 0x20: //JSR
			++PC;
			// rS = op1; //wtf
			CpuRead(0x0100 | rS--);
			CpuWrite(addressBus, PC >> 8);
			CpuWrite(0x0100 | rS--, PC);
			CpuRead(PC);

			PC = op1 + (dataBus << 8);
			break;
		case 0x40: //RTI
			++PC;
			CpuRead(0x0100 | rS++);
			CpuRead(0x0100 | rS++);

			rP = dataBus | 0x20;
			CpuRead(0x0100 | rS++);
			tempData = dataBus;

			CpuRead(0x0100 | rS);

			PC = tempData | (dataBus << 8);
			break;
		case 0x60: //RTS
			++PC;
			CpuRead(0x0100 | rS++);
			CpuRead(0x0100 | rS++);
			tempData = dataBus;

			CpuRead(0x0100 | rS);

			PC = tempData | (dataBus << 8);
			CpuRead(PC++);
			break;

		case 0x4C: //JMP abs
			CpuRead(++PC);
			PC = op1 | (dataBus << 8);
			break;
		case 0x6C: //JMP ind
			CpuRead(++PC);

			++PC;
			CpuRead(op1 | (dataBus << 8));
			tempData = dataBus;
			CpuRead((addressBus & 0xFF00) + uint8_t(addressBus + 1));

			PC = tempData | (dataBus << 8);
			break;

		case 0x88: //DEY
			--rY; //set one cycle later? doesn't matter
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0xC8: //INY
			++rY;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0xCA: //DEX
			--rX;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0xE8: //INX
			++rX;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;

		case 0x81: //STA (ind,x) indexed indirect
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			tempData = dataBus;
			CpuRead(uint8_t(addressBus + 1));
			CpuWrite(tempData | (dataBus << 8), rA);
			break;
		case 0x85: //STA zp
			++PC;
			CpuWrite(op1, rA);
			break;
		case 0x8D: //STA abs
			CpuRead(++PC);
			++PC;
			CpuWrite(op1 | (dataBus << 8), rA);
			break;
		case 0x91: //STA (ind),y indirect indexed
			++PC;
			CpuRead(op1);
			tempData = dataBus;
			CpuRead(uint8_t(op1 + 1));
			CpuRead(uint8_t(tempData + rY) | (dataBus << 8));
			CpuWrite(addressBus + (tempData + rY & 0x0100), rA);
			break;
		case 0x95: //STA zp,x
			++PC;
			CpuRead(op1);
			CpuWrite(uint8_t(addressBus + rX), rA);
			break;
		case 0x99: //STA abs,y
			CpuRead(++PC);
			++PC;
			CpuRead(uint8_t(op1 + rY) | (dataBus << 8));
			CpuWrite(addressBus + (op1 + rY & 0x0100), rA);
			break;
		case 0x9D: //STA abs,x
			CpuRead(++PC);
			++PC;
			CpuRead(uint8_t(op1 + rX) | (dataBus << 8));
			CpuWrite(addressBus + (op1 + rX & 0x0100), rA);
			break;

		case 0x84: //STY zp
			++PC;
			CpuWrite(op1, rY);
			break;
		case 0x8C: //STY abs
			CpuRead(++PC);
			++PC;
			CpuWrite(op1 | (dataBus << 8), rY);
			break;
		case 0x94: //STY zp,x
			++PC;
			CpuRead(op1);
			CpuWrite(uint8_t(addressBus + rX), rY);
			break;

		case 0x86: //STX zp
			++PC;
			CpuWrite(op1, rX);
			break;
		case 0x8E: //STX abs
			CpuRead(++PC);
			++PC;
			CpuWrite(op1 | (dataBus << 8), rX);
			break;
		case 0x96: //STX zp,y
			++PC;
			CpuRead(op1);
			CpuWrite(uint8_t(addressBus + rY), rX);
			break;

		case 0x83: //SAX (ind,x)
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			tempData = dataBus;
			CpuRead(uint8_t(addressBus + 1));
			CpuWrite(tempData | (dataBus << 8), rA & rX);
			break;
		case 0x87: //SAX zp
			++PC;
			CpuWrite(op1, rA & rX);
			break;
		case 0x8F: //SAX abs
			CpuRead(++PC);
			++PC;
			CpuWrite(op1 | (dataBus << 8), rA & rX);
			break;
		case 0x97: //SAX zp,y
			++PC;
			CpuRead(op1);
			CpuWrite(uint8_t(addressBus + rY), rA & rX);
			break;

		case 0x8A: //TXA
			rA = rX;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x98: //TYA
			rA = rY;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x9A: //TXS
			rS = rX;
			break;
		case 0xA8: //TAY
			rY = rA;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0xAA: //TAX
			rX = rA;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0xBA: //TSX
			rX = rS;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;

		case 0x0B: case 0x2B: //ANC
			++PC;
			rA &= op1;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			rP[0] = rA & 0x80;
			break;

		case 0xCB: //AXS
			++PC;
			rP[0] = !((rA & rX) - dataBus & 0x0100);
			rP[1] = !((rA & rX) - dataBus);
			rP[7] = (rA & rX) - dataBus & 0x80;
			rX = (rA & rX) - op1;
			break;

		// case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA: //NOP (1 byte)
		// case 0x04: case 0x14: case 0x34: case 0x44: case 0x54: case 0x64: case 0x74: //NOP (2 bytes)
		// case 0x80: case 0x82: case 0x89: case 0xC2: case 0xD4: case 0xE2: case 0xF4:
		// case 0x0C: case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: //NOP (3 bytes)
			// break;

		case 0x06: case 0x07: case 0x26: case 0x27: case 0x46: case 0x47: //z RW
		case 0x66: case 0x67: case 0xC6: case 0xC7: case 0xE6: case 0xE7:
			++PC;
			CpuRead(op1);
			CpuWrite(addressBus, dataBus);
			break;
		case 0x0E: case 0x0F: case 0x2E: case 0x2F: case 0x4E: case 0x4F: //abs RW
		case 0x6E: case 0x6F: case 0xCE: case 0xCF: case 0xEE: case 0xEF:
			CpuRead(++PC);

			++PC;
			CpuRead(op1 | (dataBus << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x16: case 0x17: case 0x36: case 0x37: case 0x56: case 0x57: //z,x RW
		case 0x76: case 0x77: case 0xD6: case 0xD7: case 0xF6: case 0xF7:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(op1 + rX));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x1E: case 0x1F: case 0x3E: case 0x3F: case 0x5E: case 0x5F: //abs,x RW
		case 0x7E: case 0x7F: case 0xDE: case 0xDF: case 0xFE: case 0xFF:
			CpuRead(++PC);
			tempData = dataBus;
			++PC;
			CpuRead(uint8_t(op1 + rX) | (tempData << 8));
			CpuRead(op1 + rX + (tempData << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x1B: case 0x3B: case 0x5B: case 0x7B: case 0xDB: case 0xFB: //abs,y RW
			CpuRead(++PC);
			tempData = dataBus;
			++PC;
			CpuRead(uint8_t(op1 + rY) | (tempData << 8));
			CpuRead(op1 + rY + (tempData << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x03: case 0x23: case 0x43: case 0x63: case 0xC3: case 0xE3: //(indir,x) RW
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			tempData = dataBus;
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(tempData | (dataBus << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x13: case 0x33: case 0x53: case 0x73: case 0xD3: case 0xF3: //(ind),y RW
			++PC;
			CpuRead(op1);
			tempData = dataBus;
			CpuRead(uint8_t(op1 + 1));
			CpuRead(uint8_t(tempData + rY) | (dataBus << 8));
			CpuRead(addressBus + (tempData + rY & 0x0100));
			CpuWrite(addressBus, dataBus);
			break;

		case 0x01: case 0x21: case 0x41: case 0x61: //(indirect,x) / indexed indirect R
		case 0xA1: case 0xA3: case 0xC1: case 0xE1:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			tempData = dataBus;
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(tempData | (dataBus << 8));
			break;
		case 0x04: case 0x05: case 0x24: case 0x25: case 0x44: case 0x45: case 0x64: //zero page R
		case 0x65: case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xC4: case 0xC5:
		case 0xE4: case 0xE5:
			++PC;
			CpuRead(op1);
			break;
		case 0x09: case 0x29: case 0x49: case 0x69: case 0x80: case 0x82: case 0x89: //immediate R
		case 0xA0: case 0xA2: case 0xA9: case 0xAB: case 0xC0: case 0xC2: case 0xC9:
		case 0xE0: case 0xE2: case 0xE9: case 0xEB:
			++PC;
			break;
		case 0x0C: case 0x0D: case 0x2C: case 0x2D: case 0x4D: case 0x6D: case 0xAC: //absolute R
		case 0xAD: case 0xAE: case 0xAF: case 0xCC: case 0xCD: case 0xEC: case 0xED:
			CpuRead(++PC);
			++PC;
			CpuRead(op1 | (dataBus << 8));
			break;
		case 0x11: case 0x31: case 0x51: case 0x71: //(indirect),y / indirect indexed R
		case 0xB1: case 0xD1: case 0xF1: case 0xB3:
			++PC;
			CpuRead(op1);
			tempData = dataBus;
			CpuRead(uint8_t(op1 + 1));
			CpuRead(uint8_t(tempData + rY) | (dataBus << 8));

			if(tempData + rY > 0xFF)
			{
				CpuRead(addressBus + 0x0100);
			}
			break;
		case 0x14: case 0x15: case 0x34: case 0x35: case 0x54: case 0x55: case 0x74: //zero page,X R
		case 0x75: case 0xB4: case 0xB5: case 0xD4: case 0xD5: case 0xF4: case 0xF5:	
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(op1 + rX));
			break;
		case 0x19: case 0x39: case 0x59: case 0x79: case 0xB9: //absolute,Y R
		case 0xBE: case 0xBF: case 0xD9: case 0xF9:
			CpuRead(++PC);

			++PC;
			CpuRead(uint8_t(op1 + rY) + (dataBus << 8));

			if(op1 + rY > 0xFF)
			{
				CpuRead(addressBus + 0x0100);
			}
			break;
		case 0x1C: case 0x1D: case 0x3C: case 0x3D: case 0x5C: case 0x5D: case 0x7C: //absolute,X R
		case 0x7D: case 0xBC: case 0xBD: case 0xDC: case 0xDD: case 0xFC: case 0xFD:
			CpuRead(++PC);

			++PC;
			CpuRead(uint8_t(op1 + rX) + (dataBus << 8));

			if(op1 + rX > 0xFF)
			{
				CpuRead(addressBus + 0x0100);
			}
			break;
		case 0xB6: case 0xB7: //zero page,Y R
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rY));
			break;
	}

	switch(opcode)
	{
		//R
		case 0xE1: case 0xE5: case 0xE9: case 0xEB: case 0xED: //sbc
		case 0xF1: case 0xF5: case 0xF9: case 0xFD:
			dataBus ^= 0xFF; // sbc(value) = adc(~value)

		case 0x61: case 0x65: case 0x69: case 0x6D: //adc
		case 0x71: case 0x75: case 0x79: case 0x7D:
			{
			const uint8_t prevrA = rA;
			rA += dataBus + rP[0];
			rP[0] = (prevrA + dataBus + rP[0]) & 0x100;
			rP[1] = !rA;
			rP[6] = (prevrA ^ rA) & (dataBus ^ rA) & 0x80;
			}
			rP[7] = rA & 0x80;
			break;

		case 0x21: case 0x25: case 0x29: case 0x2D: //and
		case 0x31: case 0x35: case 0x39: case 0x3D:
			rA &= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0xC1: case 0xC5: case 0xC9: case 0xCD: //cmp
		case 0xD1: case 0xD5: case 0xD9: case 0xDD:
			rP[0] = !(rA - dataBus & 0x0100);
			rP[1] = !(rA - dataBus);
			rP[7] = rA - dataBus & 0x80;
			break;

		case 0x41: case 0x45: case 0x49: case 0x4D: //eor
		case 0x51: case 0x55: case 0x59: case 0x5D:
			rA ^= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0xA1: case 0xA5: case 0xA9: case 0xAD: //lda
		case 0xB1: case 0xB5: case 0xB9: case 0xBD:
			rA = dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x01: case 0x05: case 0x09: case 0x0D: //ora
		case 0x11: case 0x15: case 0x19: case 0x1D:
			rA |= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0xE0: case 0xE4: case 0xEC: //cpx
			rP[0] = !(rX - dataBus & 0x0100);
			rP[1] = !(rX - dataBus);
			rP[7] = rX - dataBus & 0x80;
			break;

		case 0xC0: case 0xC4: case 0xCC: //cpy
			rP[0] = !(rY - dataBus & 0x0100);
			rP[1] = !(rY - dataBus);
			rP[7] = rY - dataBus & 0x80;
			break;

		case 0xA2: case 0xA6: case 0xAE: case 0xB6: case 0xBE: //ldx
			rX = dataBus;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;

		case 0xA0: case 0xA4: case 0xAC: case 0xB4: case 0xBC: //ldy
			rY = dataBus;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;

		case 0x24: case 0x2C: //bit
			rP[1] = !(dataBus & rA);
			rP[6] = dataBus & 0x40;
			rP[7] = dataBus & 0x80;
			break;

		case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xAB: case 0xBF: //lax
			rA = dataBus;
			rX = dataBus;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;

		//RW
		case 0x46: case 0x4E: case 0x56: case 0x5E: //lsr
			rP[0] = dataBus & 0x01;
			dataBus >>= 1;
			rP[1] = !dataBus;
			rP.reset(7);
			CpuWrite(addressBus, dataBus);
			break;

		case 0x06: case 0x0E: case 0x16: case 0x1E: //asl
			rP[0] = dataBus & 0x80;
			dataBus <<= 1;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);
			break;

		case 0x26: case 0x2E: case 0x36: case 0x3E: //rol
			{
			const bool newCarry = dataBus & 0x80;
			dataBus = (dataBus << 1) | rP[0];
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);
			break;

		case 0x66: case 0x6E: case 0x76: case 0x7E: //ror
			{
			const bool newCarry = dataBus & 0x01;
			dataBus = (dataBus >> 1) | (rP[0] << 7);
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);
			break;

		case 0xC6: case 0xCE: case 0xD6: case 0xDE: //dec
			--dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);
			break;

		case 0xE6: case 0xEE: case 0xF6: case 0xFE: //inc
			++dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);
			break;

		case 0xC3: case 0xC7: case 0xCF: case 0xD3: case 0xD7: case 0xDB: case 0xDF: //dcp
			--dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

			rP[0] = !(rA - dataBus & 0x0100);
			rP[1] = !(rA - dataBus);
			rP[7] = rA - dataBus & 0x80;
			break;

		case 0xE3: case 0xE7: case 0xEF: case 0xF3: case 0xF7: case 0xFB: case 0xFF: //isc
			++dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

			{
			const uint8_t prevrA = rA;
			rA = (rA - dataBus) - !rP[0];
			rP[0] = !((prevrA - dataBus) - !rP[0] & 0x0100);
			rP[1] = !rA;
			rP[6] = (prevrA ^ rA) & (~dataBus ^ rA) & 0x80;
			}
			rP[7] = rA & 0x80;
			break;

		case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: //slo
			rP[0] = dataBus & 0x80;
			dataBus <<= 1;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

			rA |= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: //rla
			{
			const bool newCarry = dataBus & 0x80;
			dataBus = (dataBus << 1) | rP[0];
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

			rA &= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: //sre
			rP[0] = dataBus & 0x01;
			dataBus >>= 1;
			rP[1] = !dataBus;
			rP.reset(7);
			CpuWrite(addressBus, dataBus);

			rA ^= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;

		case 0x63: case 0x67: case 0x6F: case 0x73: case 0x77: case 0x7B: case 0x7F: //rra
			{
			const bool newCarry = dataBus & 0x01;
			dataBus = (dataBus >> 1) | (rP[0] << 7);
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

			{
			const uint8_t prevrA = rA;
			rA += dataBus + rP[0];
			rP[0] = (prevrA + dataBus + rP[0]) & 0x100;
			rP[1] = !rA;
			rP[6] = (prevrA ^ rA) & (dataBus ^ rA) & 0x80;
			}
			rP[7] = rA & 0x80;
			break;
	}

	CpuRead(PC); //fetch next opcode
	CpuOpDone();
}


void Nes::CpuRead(uint16_t address)
{
	addressBus = address;

	switch(addressBus & 0xE000)
	{
		case 0x0000: dataBus = cpuRam[addressBus & 0x07FF]; break;

		case 0x2000:
			switch(addressBus & 7)
			{
				case 2: dataBus = ppu.StatusRead();  break;
				case 4: dataBus = ppu.OamDataRead(); break;
				case 7: dataBus = ppu.DataRead();    break;
			}
		break;

		case 0x4000:
			switch(addressBus)
			{
				case 0x4014: dataBus = 0x40;             break; //open bus, maybe hax something better later
				case 0x4015: dataBus = apu.StatusRead(); break;

				case 0x4016:
					dataBus = (addressBus & 0xE0) | (controller_reg & 1); //addressBus = open bus?
					controller_reg >>= 1; //todo: if we have read 4016 8 times, start returning 1s
				break;
			}
		break;

		case 0x6000: break;
		case 0x8000: dataBus = *(bankPtr[0] + (addressBus & 0x1FFF)); break;
		case 0xA000: dataBus = *(bankPtr[1] + (addressBus & 0x1FFF)); break;
		case 0xC000: dataBus = *(bankPtr[2] + (addressBus & 0x1FFF)); break;
		case 0xE000: dataBus = *(bankPtr[3] + (addressBus & 0x1FFF)); break;
	}

	if(apu.dmcDma & !dmcDmaActive) //where to do this?
	{
		const uint16_t tempAdr = addressBus; //better solution?

		dmcDmaActive = true;

		CpuRead(addressBus); //current read becomes the halt/1st dummy, this is the 2nd dummy read
		if(cycleCount & 1)
		{
			CpuRead(addressBus); //alignment
		}
		CpuRead(apu.GetDmcAddr()); //dma fetch
		apu.DmcDma(dataBus);
		CpuRead(tempAdr); //resume (addressBus)

		dmcDmaActive = false;
		apu.dmcDma = false;
	}

	CpuTick();
}


void Nes::CpuWrite(uint16_t address, uint8_t data)
{
	addressBus = address;
	dataBus = data;

	switch(addressBus & 0xE000)
	{
		case 0x0000: cpuRam[addressBus & 0x07FF] = dataBus; break;

		case 0x2000:
			switch(addressBus & 7)
			{
				case 0: ppu.CtrlWrite(dataBus);    break;
				case 1: ppu.MaskWrite(dataBus);    break;
				case 3: ppu.OamAddrWrite(dataBus); break;
				case 4: ppu.OamDataWrite(dataBus); break;
				case 5: ppu.ScrollWrite(dataBus);  break;
				case 6: ppu.AddrWrite(dataBus);    break;
				case 7: ppu.DataWrite(dataBus);    break;
			}
		break;

		case 0x4000:
			switch(addressBus)
			{
				case 0x4000: apu.Pulse0Write(dataBus, 0); break;
				case 0x4001: apu.Pulse1Write(dataBus, 0); break;
				case 0x4002: apu.Pulse2Write(dataBus, 0); break;
				case 0x4003: apu.Pulse3Write(dataBus, 0); break;

				case 0x4004: apu.Pulse0Write(dataBus, 1); break;
				case 0x4005: apu.Pulse1Write(dataBus, 1); break;
				case 0x4006: apu.Pulse2Write(dataBus, 1); break;
				case 0x4007: apu.Pulse3Write(dataBus, 1); break;

				case 0x4008: apu.Triangle0Write(dataBus); break;
				case 0x400A: apu.Triangle2Write(dataBus); break;
				case 0x400B: apu.Triangle3Write(dataBus); break;

				case 0x400C: apu.Noise0Write(dataBus);    break;
				case 0x400E: apu.Noise2Write(dataBus);    break;
				case 0x400F: apu.Noise3Write(dataBus);    break;

				case 0x4010: apu.Dmc0Write(dataBus);      break;
				case 0x4011: apu.Dmc1Write(dataBus);      break;
				case 0x4012: apu.Dmc2Write(dataBus);      break;
				case 0x4013: apu.Dmc3Write(dataBus);      break;

				case 0x4014:
					dmaPending = true;
					dmaAddress = dataBus << 8;
				break;

				case 0x4015: apu.StatusWrite(dataBus);       break;
				case 0x4016: readJoy1 = dataBus & 1;         break;
				case 0x4017: apu.FrameCounterWrite(dataBus); break;
			}
		break;

		case 0x6000: break;

		case 0x8000: case 0xA000: case 0xC000: case 0xE000:
			if(mapper == 2)
			{
				bankPtr[0] = &prgRom[(dataBus & 0b1111) * 0x4000];
				bankPtr[1] = bankPtr[0] + 0x2000;
			}
			else if(mapper == 7)
			{
				bankPtr[0] = &prgRom[(dataBus & 0b0111) * 0x8000];
				bankPtr[1] = bankPtr[0] + 0x2000;
				bankPtr[2] = bankPtr[0] + 0x4000;
				bankPtr[3] = bankPtr[0] + 0x6000;

				if(dataBus & 0b00010000)
				{
					ppu.SetNametableMirroring(0xC00, 0);
				}
				else
				{
					ppu.SetNametableMirroring(0x400, 0x400); //fix: only use 0x2400
				}
			}
		break;
	}

	CpuTick();
}


void Nes::CpuTick()
{
	apu.Tick();
	ppu.Tick();
	PollInterrupts();
	ppu.Tick();
	ppu.Tick();

	++cycleCount;
}


void Nes::CpuOpDone()
{
	if(dmaPending)
	{
		// DMA actually waits for writes to finish, not op to finish
		// however, this makes no difference for OAM DMA
		if(cycleCount & 1)
		{
			CpuRead(PC); //read what? doesn't really matter, maybe addressBus
		}

		for(uint16_t x=0; x<=255; ++x)
		{
			CpuRead(dmaAddress + x); //should dmaAddress be 1st or 2nd write from R&W ops (2nd currently)?
			CpuWrite(0x2004, dataBus);
		}

		CpuRead(PC);
		dmaPending = false;
	}

	if(nmiLine)
	{
		CpuRead(addressBus);                                //fetch op1, increment suppressed
		CpuWrite(0x100 | rS--, PC >> 8);                    //push PC high on stack
		CpuWrite(0x100 | rS--, PC);                         //push PC low on stack
		CpuWrite(0x100 | rS--, rP.to_ulong() & 0b11101111); //push flags on stack with B clear

		rP.set(2);                                          //read nmi vector low, set I flag
		CpuRead(0xFFFA);                                    //
		tempData = dataBus;
		CpuRead(0xFFFB);                                    //read nmi vector high

		PC = tempData | (dataBus << 8);                     //fetch next opcode
		CpuRead(PC);                                        //

		nmiPending = false;
		irqPending = false;
	}
	else if(irqLine)
	{
		CpuRead(addressBus);                                //fetch op1, increment suppressed
		CpuWrite(0x100 | rS--, PC >> 8);                    //push PC high on stack
		CpuWrite(0x100 | rS--, PC);                         //push PC low on stack
		CpuWrite(0x100 | rS--, rP.to_ulong() & 0b11101111); //push flags on stack with B clear

		rP.set(2);                                          //read irq vector low, set I flag
		CpuRead(0xFFFE);                                    //
		tempData = dataBus;
		CpuRead(0xFFFF);                                    //read irq vector high

		PC = tempData | (dataBus << 8);                     //fetch next opcode
		CpuRead(PC);                                        //

		irqPending = false;
	}
	nmiLine = nmiPending;
	irqLine = irqPending;
}


void Nes::PollInterrupts()
{
	bool oldNmi = nmi;
	nmi = ppu.PollNmi();
	nmiPending |= !oldNmi & nmi;

	irqPending = !rP.test(2) & apu.PollFrameInterrupt();
}


void Nes::DebugCpu(uint8_t opcode)
{
	const std::array<std::string, 256> opName
	{
		"BRK","ORA","KIL","SLO","NOP","ORA","ASL","SLO","PHP","ORA","ASL","ANC","NOP","ORA","ASL","SLO",
		"BPL","ORA","KIL","SLO","NOP","ORA","ASL","SLO","CLC","ORA","NOP","SLO","NOP","ORA","ASL","SLO",
		"JSR","AND","KIL","RLA","BIT","AND","ROL","RLA","PLP","AND","ROL","ANC","BIT","AND","ROL","RLA",
		"BMI","AND","KIL","RLA","NOP","AND","ROL","RLA","SEC","AND","NOP","RLA","NOP","AND","ROL","RLA",
		"RTI","EOR","KIL","SRE","NOP","EOR","LSR","SRE","PHA","EOR","LSR","ALR","JMP","EOR","LSR","SRE",
		"BVC","EOR","KIL","SRE","NOP","EOR","LSR","SRE","CLI","EOR","NOP","SRE","NOP","EOR","LSR","SRE",
		"RTS","ADC","KIL","RRA","NOP","ADC","ROR","RRA","PLA","ADC","ROR","ARR","JMP","ADC","ROR","RRA",
		"BVS","ADC","KIL","RRA","NOP","ADC","ROR","RRA","SEI","ADC","NOP","RRA","NOP","ADC","ROR","RRA",
		"NOP","STA","NOP","SAX","STY","STA","STX","SAX","DEY","NOP","TXA","XAA","STY","STA","STX","SAX",
		"BCC","STA","KIL","AHX","STY","STA","STX","SAX","TYA","STA","TXS","TAS","SHY","STA","SHX","AHX",
		"LDY","LDA","LDX","LAX","LDY","LDA","LDX","LAX","TAY","LDA","TAX","LAX","LDY","LDA","LDX","LAX",
		"BCS","LDA","KIL","LAX","LDY","LDA","LDX","LAX","CLV","LDA","TSX","LAS","LDY","LDA","LDX","LAX",
		"CPY","CMP","NOP","DCP","CPY","CMP","DEC","DCP","INY","CMP","DEX","AXS","CPY","CMP","DEC","DCP",
		"BNE","CMP","KIL","DCP","NOP","CMP","DEC","DCP","CLD","CMP","NOP","DCP","NOP","CMP","DEC","DCP",
		"CPX","SBC","NOP","ISC","CPX","SBC","INC","ISC","INX","SBC","NOP","SBC","CPX","SBC","INC","ISC",
		"BEQ","SBC","KIL","ISC","NOP","SBC","INC","ISC","SED","SBC","NOP","ISC","NOP","SBC","INC","ISC"
	};

	// const uint8_t opcode = cpuMem[PC];
	// const uint8_t op1 = cpuMem[PC+1];
	// const uint8_t op2 = cpuMem[PC+2];

	std::cout << std::uppercase << std::hex << std::setfill('0')
			  << std::setw(4) << PC
			  << "  " << std::setw(2) << +opcode
			  // << " " << std::setw(4) << op1 + (op2 << 8)
			  << " " << std::setw(4) << (DebugRead(PC+1) + (DebugRead(PC+2) << 8))
			  << "  " << opName[opcode]
			  << "    A:" << std::setw(2) << +rA
			  << " X:" << std::setw(2) << +rX
			  << " Y:" << std::setw(2) << +rY
			  << " P:" << std::setw(2) << (rP.to_ulong() & ~0x10)
			  << " SP:" << std::setw(2) << +rS
			  << " PPU:" << std::setw(3) << std::dec << ppu.GetScanlineH()
			  << " SL:" << std::setw(3) << ppu.GetScanlineV() << std::endl;
}


uint8_t Nes::DebugRead(uint16_t address)
{
	if(address < 0x8000)
	{
		std::cout << "running code below 0x8000, at 0x"
		<< std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address << "\n";
	}

	uint8_t a;
	switch(address & 0xE000)
	{
		case 0x8000: a = *(bankPtr[0] + (address & 0x1FFF)); break;
		case 0xA000: a = *(bankPtr[1] + (address & 0x1FFF)); break;
		case 0xC000: a = *(bankPtr[2] + (address & 0x1FFF)); break;
		case 0xE000: a = *(bankPtr[3] + (address & 0x1FFF)); break;
	}

	return a;
}