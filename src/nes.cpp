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


Nes::Nes(std::string inFile)
{
	LoadRom(inFile);

	addressBus = PC;
	dataBus = cpuMem[addressBus]; // CpuRead();

	++PC;
	++addressBus;
	dataBus = cpuMem[addressBus]; //CpuRead();

	++PC;
	addressBus = 0x0100 | rS;
	dataBus = PC >> 8;
	dataBus = cpuMem[addressBus]; //CpuRead();

	addressBus = 0x0100 | uint8_t(addressBus - 1);
	dataBus = PC;
	dataBus = cpuMem[addressBus]; //CpuRead();

	addressBus = 0x0100 | uint8_t(addressBus - 1);
	dataBus = rP.to_ulong() | 0x10;
	dataBus = cpuMem[addressBus]; //CpuRead();

	rS -= 3;
	addressBus = 0xFFFC;
	dataBus = cpuMem[addressBus]; //CpuRead();

	++addressBus;
	dataBus = cpuMem[addressBus]; //CpuRead();

	PC = cpuMem[0xFFFC] | (dataBus << 8);

	addressBus = PC;              //fetch next opcode
	dataBus = cpuMem[addressBus]; //CpuRead();

	rP = 0x36;
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
	std::ifstream iFile(inFile.c_str(), std::ios::in | std::ios::binary);
	if(iFile.is_open() == false)
	{
		std::cout << "File not found" << std::endl;
		exit(0);
	}

	std::ostringstream contents;
    contents << iFile.rdbuf();
	iFile.close();

	const std::string contentStr = contents.str();
	const std::vector<uint8_t> contentVec(contentStr.begin(), contentStr.end());
	kass = contentVec;


	//ines header stuff
	std::array<uint8_t, 16> header;
	std::copy_n(contentVec.begin(), 16, header.begin());

	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A)
	{
		std::cout << "Not a valid .nes file" << std::endl;
		exit(0);
	}

	mapper = (header[6] >> 4) | (header[7] & 0b11110000);
	if(mapper == 0)
	{
		std::copy_n(contentVec.begin() + 0x10, 0x4000, cpuMem.begin() + 0x8000);
		if(header[4] == 1)
		{
			std::copy_n(cpuMem.begin() + 0x8000, 0x4000, cpuMem.begin() + 0xC000);
		}
		else if(header[4] == 2)
		{
			std::copy_n(contentVec.begin() + 0x4010, 0x4000, cpuMem.begin() + 0xC000);
		}

		if(header[5])
		{
			std::copy_n(contentVec.begin() + 0x4000 * header[4] + 0x10, 0x2000, ppu.VramIterator());
		}

		if(header[6] & 1)
		{
			ppu.SetNametableMirroring(0x800);
			std::cout << "X layout\n";
		}
		else
		{
			ppu.SetNametableMirroring(0x400);
			std::cout << "Y layout\n";
		}
	}
	else if(mapper == 2)
	{
		std::copy_n(contentVec.begin() + 0x10, 0x4000, cpuMem.begin() + 0x8000);
		std::copy_n(contentVec.begin() + 0x10 + 0x4000 * (header[4] - 1), 0x4000, cpuMem.begin() + 0xC000);

		if(header[5])
		{
			std::copy_n(contentVec.begin() + 0x4000 * header[4] + 0x10, 0x2000, ppu.VramIterator());
		}

		if(header[6] & 1)
		{
			ppu.SetNametableMirroring(0x800);
			std::cout << "X layout\n";
		}
		else
		{
			ppu.SetNametableMirroring(0x400);
			std::cout << "Y layout\n";
		}
	}
	else
	{
		std::cout << "unsupported mapper\n";
		exit(0);
	}
}


void Nes::RunOpcode()
{
	const uint8_t opcode = cpuMem[PC];
	const uint8_t op1 = cpuMem[PC+1];
	const uint8_t op2 = cpuMem[PC+2];


	#ifdef DEBUG
		DebugCpu();
	#endif
	#ifdef DUMP_VRAM
		if(ppu.GetScanlineV() == 261)
		{
			std::string outfile = "vram.txt";
			std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
			result.write((char*)&ppu.GetVram()[0],0x4000);
			result.close();
		}
	#endif


	++PC;                    //fetch op1
	CpuRead(addressBus + 1); //

	switch(opcode)
	{
		// opcodes left
		// 0B ANC
		// 2B ANC
		// 4B ALR
		// 6B ARR
		// 8B XAA
		// 93 AHX
		// 9B TAS
		// 9C SHY
		// 9E SHX
		// 9F AHX
		// AB LAX
		// BB LAS
		// CB AXS

		case 0x00: //BRK
			++PC;
			CpuWrite(0x0100 | rS, PC >> 8);
			CpuWrite(0x0100 | uint8_t(addressBus - 1), PC);
			CpuWrite(0x0100 | uint8_t(addressBus - 1), rP.to_ulong() | 0b00010000);

			rS -= 3;
			rP.set(2);
			CpuRead(0xFFFE);
			CpuRead(0xFFFF);

			PC = cpuMem[0xFFFE] | (dataBus << 8);
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			++addressBus;
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
			CpuRead(0x0100 | rS);
			CpuWrite(addressBus, PC >> 8);
			CpuWrite(0x0100 | uint8_t(addressBus - 1), PC);
			CpuRead(PC);

			rS -= 2;
			PC = op1 + (op2 << 8);
			break;
		case 0x40: //RTI
			++PC;
			CpuRead(0x0100 | rS);
			CpuRead(0x0100 | uint8_t(addressBus + 1));

			rP = dataBus | 0x20;
			CpuRead(0x0100 | uint8_t(addressBus + 1));

			rS += 3;
			CpuRead(0x0100 | uint8_t(addressBus + 1));

			PC = cpuMem[0x0100 | uint8_t(addressBus - 1)] | (dataBus << 8);
			break;
		case 0x60: //RTS
			++PC;
			CpuRead(0x0100 | rS);
			CpuRead(0x0100 | uint8_t(addressBus + 1));

			rS += 2;
			CpuRead(0x0100 | uint8_t(addressBus + 1));

			PC = cpuMem[0x0100 | addressBus - 1] | (dataBus << 8);
			CpuRead(PC++);
			break;

		case 0x4C: //JMP abs
			++PC;
			CpuRead(addressBus + 1);
			PC = op1 | (op2 << 8);
			break;
		case 0x6C: //JMP ind
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(op1 | (op2 << 8));
			CpuRead(uint8_t(op1 + 1) | (op2 << 8));

			PC = cpuMem[op1 | (op2 << 8)] | (dataBus << 8);
			break;

		case 0x88: //DEY
			--rY; //actually set one cycle later
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

		case 0x81: //STA (ind,x)
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			CpuRead(uint8_t(addressBus + 1));
			CpuWrite(cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8), rA);
			break;
		case 0x85: //STA zp
			++PC;
			CpuWrite(op1, rA);
			break;
		case 0x8D: //STA abs
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuWrite(op1 | (op2 << 8), rA);
			break;
		case 0x91: //STA (ind),y
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(uint8_t(cpuMem[uint8_t(addressBus - 1)] + rY) | (cpuMem[addressBus] << 8));
			CpuWrite(addressBus + (cpuMem[op1] + rY & 0x0100), rA);
			break;
		case 0x95: //STA zp,x
			++PC;
			CpuRead(op1);
			CpuWrite(uint8_t(addressBus + rX), rA);
			break;
		case 0x99: //STA abs,y
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuRead(uint8_t(op1 + rY) | (op2 << 8));
			CpuWrite(addressBus + (op1 + rY & 0x0100), rA);
			break;
		case 0x9D: //STA abs,x
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuRead(uint8_t(op1 + rX) | (op2 << 8));
			CpuWrite(addressBus + (op1 + rX & 0x0100), rA);
			break;

		case 0x84: //STY zp
			++PC;
			CpuWrite(op1, rY);
			break;
		case 0x8C: //STY abs
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuWrite(op1 | (op2 << 8), rY);
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
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuWrite(op1 | (op2 << 8), rX);
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
			CpuRead(uint8_t(addressBus + 1));
			CpuWrite(cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8), rA & rX);
			break;
		case 0x87: //SAX zp
			++PC;
			CpuWrite(op1, rA & rX);
			break;
		case 0x8F: //SAX abs
			++PC;
			CpuRead(addressBus + 1);
			++PC;
			CpuWrite(op1 | (op2 << 8), rA & rX);
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
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(op1 | (op2 << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x16: case 0x17: case 0x36: case 0x37: case 0x56: case 0x57: //z,x RW
		case 0x76: case 0x77: case 0xD6: case 0xD7: case 0xF6: case 0xF7:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x1E: case 0x1F: case 0x3E: case 0x3F: case 0x5E: case 0x5F: //abs,x RW
		case 0x7E: case 0x7F: case 0xDE: case 0xDF: case 0xFE: case 0xFF:
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(uint8_t(op1 + rX) | (op2 << 8));
			CpuRead(op1 + rX + (op2 << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x1B: case 0x3B: case 0x5B: case 0x7B: case 0xDB: case 0xFB: //abs,y RW
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(uint8_t(op1 + rY) | (op2 << 8));
			CpuRead(op1 + rY + (op2 << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x03: case 0x23: case 0x43: case 0x63: case 0xC3: case 0xE3: //(indir,x) RW
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8));
			CpuWrite(addressBus, dataBus);
			break;
		case 0x13: case 0x33: case 0x53: case 0x73: case 0xD3: case 0xF3: //(ind),y RW
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(uint8_t(cpuMem[uint8_t(addressBus - 1)] + rY) | (cpuMem[addressBus] << 8));
			CpuRead(addressBus + (cpuMem[op1] + rY & 0x0100));
			CpuWrite(addressBus, dataBus);
			break;

		case 0x01: case 0x21: case 0x41: case 0x61: //(indirect,x) / indexed indirect R
		case 0xA1: case 0xA3: case 0xC1: case 0xE1:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8));
			break;
		case 0x04: case 0x05: case 0x24: case 0x25: case 0x44: case 0x45: case 0x64: //zero page R
		case 0x65: case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xC4: case 0xC5:
		case 0xE4: case 0xE5:
			++PC;
			CpuRead(op1);
			break;
		case 0x09: case 0x29: case 0x49: case 0x69: case 0x80: case 0x82: case 0x89: //immediate R
		case 0xA0: case 0xA2: case 0xA9: case 0xC0: case 0xC2: case 0xC9: case 0xE0:
		case 0xE2: case 0xE9: case 0xEB:
			++PC;
			break;
		case 0x0C: case 0x0D: case 0x2C: case 0x2D: case 0x4D: case 0x6D: case 0xAC: //absolute R
		case 0xAD: case 0xAE: case 0xAF: case 0xCC: case 0xCD: case 0xEC: case 0xED:
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(op1 | (op2 << 8));
			break;
		case 0x11: case 0x31: case 0x51: case 0x71: //(indirect),y / indirect indexed R
		case 0xB1: case 0xD1: case 0xF1: case 0xB3:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + 1));
			CpuRead(uint8_t(cpuMem[uint8_t(addressBus - 1)] + rY) | (cpuMem[addressBus] << 8));

			if(cpuMem[op1] + rY > 0xFF)
			{
				CpuRead(addressBus + 0x0100);
			}
			break;
		case 0x14: case 0x15: case 0x34: case 0x35: case 0x54: case 0x55: case 0x74: //zero page,X R
		case 0x75: case 0xB4: case 0xB5: case 0xD4: case 0xD5: case 0xF4: case 0xF5:
			++PC;
			CpuRead(op1);
			CpuRead(uint8_t(addressBus + rX));
			break;
		case 0x19: case 0x39: case 0x59: case 0x79: case 0xB9: //absolute,Y R
		case 0xBE: case 0xBF: case 0xD9: case 0xF9:
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(uint8_t(op1 + rY) + (op2 << 8));

			if(op1 + rY > 0xFF)
			{
				CpuRead(addressBus + 0x0100);
			}
			break;
		case 0x1C: case 0x1D: case 0x3C: case 0x3D: case 0x5C: case 0x5D: case 0x7C: //absolute,X R
		case 0x7D: case 0xBC: case 0xBD: case 0xDC: case 0xDD: case 0xFC: case 0xFD:
			++PC;
			CpuRead(addressBus + 1);

			++PC;
			CpuRead(uint8_t(op1 + rX) + (op2 << 8));

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

		case 0xE1: case 0xE5: case 0xE9: case 0xEB: case 0xED: //sbc
		case 0xF1: case 0xF5: case 0xF9: case 0xFD:
			{
			const uint8_t prevrA = rA;
			rA = (rA - dataBus) - !rP[0];
			rP[0] = !((prevrA - dataBus) - !rP[0] & 0x0100);
			rP[1] = !rA;
			rP[6] = (prevrA ^ rA) & (~dataBus ^ rA) & 0x80;
			}
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

		case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xBF: //lax
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

	//interrupt checking here?
	nmiLine = ppu.PollNmi();

	CpuRead(PC); //fetch next opcode
	CpuOpDone();
}


void Nes::CpuRead(uint16_t address)
{
	addressBus = address;
	dataBus = cpuMem[addressBus];

	if((addressBus & 0xE000) == 0x2000) //ppu register mirrors
	{
		addressBus &= 0x2007;
	}

	switch(addressBus)
	{
		case 0x2002:
			dataBus = ppu.StatusRead();
			break;
		case 0x2007: //ppuData
			dataBus = ppu.DataRead();
			break;

		case 0x4015:
			dataBus = apu.StatusRead();
			break;
		case 0x4016:
			dataBus = (addressBus & 0xE0) | (controller_reg & 1); //addressBus = open bus?
			controller_reg >>= 1; //if we have read 4016 8 times, start returning 1s
			break;
	}

	CpuTick();
}


void Nes::CpuWrite(uint16_t address, uint8_t data) // block writes to ppu on the first screen
{
	addressBus = address;
	dataBus = data;

	if(addressBus & 0x8000)
	{
		if(mapper == 2) //mapper 2, unrom
		{
			std::memcpy(&cpuMem[0x8000], &kass[0x10 + (dataBus & 0b0111)], 0x4000);
			CpuTick();
			return;
		}
		else
		{
			CpuTick();
			return;
		}
	}

	cpuMem[addressBus] = dataBus;

	if((addressBus & 0xE000) == 0x2000)
	{
		addressBus &= 0x2007;
	}

	switch(addressBus)
	{
		case 0x2000:
			ppu.CtrlWrite(dataBus);
			break;
		case 0x2001:
			ppu.MaskWrite(dataBus);
			break;
		case 0x2003: //oamAddr
			ppu.OamAddrWrite(dataBus);
			break;
		case 0x2004: //oamData
			ppu.OamDataWrite(dataBus);
			break;
		case 0x2005: //ppuScroll
			ppu.ScrollWrite(dataBus);
			break;
		case 0x2006:
			ppu.AddrWrite(dataBus);
			break;
		case 0x2007: //ppuData
			ppu.DataWrite(dataBus);
			break;

		case 0x4000:
			apu.Pulse0Write(dataBus, 0);
			break;
		case 0x4001:
			apu.Pulse1Write(dataBus, 0);
			break;
		case 0x4002:
			apu.Pulse2Write(dataBus, 0);
			break;
		case 0x4003:
			apu.Pulse3Write(dataBus, 0);
			break;
		case 0x4004:
			apu.Pulse0Write(dataBus, 1);
			break;
		case 0x4005:
			apu.Pulse1Write(dataBus, 1);
			break;
		case 0x4006:
			apu.Pulse2Write(dataBus, 1);
			break;
		case 0x4007:
			apu.Pulse3Write(dataBus, 1);
			break;
		case 0x4008:
			apu.Triangle0Write(dataBus);
			break;
		case 0x400A:
			apu.Triangle2Write(dataBus);
			break;
		case 0x400B:
			apu.Triangle3Write(dataBus);
			break;
		case 0x400C:
			apu.Noise0Write(dataBus);
			break;
		case 0x400E:
			apu.Noise2Write(dataBus);
			break;
		case 0x400F:
			apu.Noise3Write(dataBus);
			break;

		case 0x4014:
			// if(cycles & 1) cpu_tick(); //+1 cycle if dma started on an odd cycle
			CpuTick(); //tick or read?
			{
			uint16_t dmaOffset = dataBus << 8;

			for(uint16_t x=0; x<=255; ++x) //only toggle bool here, move this to CpuOpDone?
			{
				CpuRead(dmaOffset++);
				CpuWrite(0x2004, dataBus);
			}
			}
			break;
		case 0x4015:
			apu.StatusWrite(dataBus);
			break;
		case 0x4016:
			if(dataBus & 1)
			{
				readJoy1 = true;
			}
			else
			{
				readJoy1 = false;
			}
			break;
		case 0x4017:
			apu.FrameCounterWrite(dataBus);
			break;
	}

	CpuTick();
}


void Nes::CpuTick()
{
	apu.Tick();
	ppu.Tick();
	ppu.Tick();
	ppu.Tick();
}


void Nes::CpuOpDone()
{
	if(nmiLine)
	{
		// ++PC;             //fetch op1, increment suppressed
		CpuRead(addressBus); //
		CpuWrite(0x100 | rS, PC >> 8); //push PC high on stack
		CpuWrite(0x100 | addressBus - 1, PC); //push PC low on stack
		CpuWrite(0x100 | addressBus - 1, rP.to_ulong() & 0b11101111); //push flags on stack with B flag clear

		rS -= 3;         //read nmi vector low, set I flag
		rP.set(2);       //
		CpuRead(0xFFFA); //
		CpuRead(0xFFFB); //read nmi vector high

		PC = cpuMem[0xFFFA] | (dataBus << 8); //fetch next opcode
		CpuRead(PC);                          //

		nmiLine = false;
	}
}


void Nes::DebugCpu()
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

	const uint8_t opcode = cpuMem[PC];
	const uint8_t op1 = cpuMem[PC+1];
	const uint8_t op2 = cpuMem[PC+2];

	std::cout << std::uppercase << std::hex << std::setfill('0')
			  << std::setw(4) << PC
			  << "  " << std::setw(2) << +opcode
			  << " " << std::setw(4) << op1 + (op2 << 8)
			  << "  " << opName[opcode]
			  << "       A:" << std::setw(2) << +rA
			  << " X:" << std::setw(2) << +rX
			  << " Y:" << std::setw(2) << +rY
			  << " P:" << std::setw(2) << (rP.to_ulong() & ~0x10)
			  << " SP:" << std::setw(2) << +rS
			  << " PPU:" << std::setw(3) << std::dec << ppu.GetScanlineH()
			  << " SL:" << std::setw(3) << ppu.GetScanlineV() << std::endl;
}
