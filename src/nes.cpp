// #define DEBUG
// #define DUMP_VRAM

#include "nes.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>


nes::nes(std::string inFile)
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

	rP = 0x36;
}


void nes::AdvanceFrame(uint8_t input)
{
	renderFrame = false;
	while(!renderFrame)
	{
		RunOpcode();

		if(readJoy1) //doesn't work outside while
		{
			controller_reg = input;
		}
	}
}


void nes::LoadRom(std::string inFile)
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


//----- iNes stuff
	std::array<uint8_t, 16> header;
	std::copy_n(contentVec.begin(), 16, header.begin());

	if(header[0] != 0x4E && header[1] != 0x45 && header[2] != 0x53 && header[3] != 0x1A)
	{
		std::cout << "Not a valid .nes file" << std::endl;
		exit(0);
	}

	ppu_nt_mirroring = (header[6] & 1) ? ~0x400 : ~0x800; //false=v, true=h
	//where to do this? cpu r/w to ppu, everything that does stuff between ppu:0x2000-0x2FFF
//-----


	std::copy_n(contentVec.begin() + 0x10, 0x4000, cpuMem.begin() + 0x8000);
	std::copy_n(contentVec.begin() + 0x4010, 0x2000, vram.begin());
	std::copy(cpuMem.begin()+0x8000, cpuMem.begin()+0xC000, cpuMem.begin()+0xC000);
}


const std::array<uint8_t, 256*240*3>* const nes::GetPixelPtr() const
{
	return &render;
}


void nes::RunOpcode()
{
	const uint8_t opcode = cpuMem[PC];
	const uint8_t op1 = cpuMem[PC+1];
	const uint8_t op2 = cpuMem[PC+2];


	#ifdef DEBUG
	std::cout << std::uppercase << std::hex << std::setfill('0')
			  << std::setw(4) << PC
			  << "  " << std::setw(2) << int(opcode)
			  << " " << std::setw(2) << int(op1)
			  << " " << std::setw(2) << int(op2)
			  << "        A:" << std::setw(2) << int(rA)
			  << " X:" << std::setw(2) << int(rX)
			  << " Y:" << std::setw(2) << int(rY)
			  << " P:" << std::setw(2) << int(rP.to_ulong() & ~0x10)
			  << " SP:" << std::setw(2) << int(rS)
			  << " PPU:" << std::setw(3) << std::dec << scanlineH
			  << " SL:" << std::setw(3) << scanlineV << std::endl;
	#endif
	#ifdef DUMP_VRAM
	if(scanlineV == 261)
	{
		std::string outfile = "vram.txt";
		std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
		result.write((char*)&vram[0],0x4000);
		result.close();
	}
	#endif


	addressBus = PC;
	CpuRead();

	++PC;
	++addressBus;
	CpuRead();

	switch(opcode)
	{
		case 0x00: //BRK
			++PC;
			addressBus = 0x0100 | rS;
			dataBus = PC >> 8;
			CpuWrite();

			addressBus = 0x0100 | uint8_t(addressBus - 1);
			dataBus = PC;
			CpuWrite();

			addressBus = 0x0100 | uint8_t(addressBus - 1);
			dataBus = rP.to_ulong() | 0x10;
			CpuWrite();

			rS -= 3;
			addressBus = 0xFFFE;
			CpuRead();

			++addressBus;
			CpuRead();

			PC = cpuMem[0xFFFE] | (dataBus << 8);
			break;

		case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52: //KIL
		case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
			std::cout << "CRASH" << std::endl;
			exit(0);
			break;

		case 0x08: //PHP
			addressBus = 0x0100 | rS;
			dataBus = rP.to_ulong();
			CpuWrite();

			--rS;
			break;
		case 0x28: //PLP
			addressBus = 0x0100 | rS;
			CpuRead();

			++rS;
			addressBus = 0x0100 | rS;
			CpuRead();

			rP = dataBus | 0x20;
			break;
		case 0x48: //PHA
			addressBus = 0x0100 | rS;
			dataBus = rA;
			CpuWrite();

			--rS;
			break;
		case 0x68: //PLA
			addressBus = 0x0100 | rS;
			CpuRead();

			++rS;
			addressBus = 0x0100 | rS;
			CpuRead();

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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
				addressBus = PC;
				CpuRead();
				if(PC != pagePC)
				{
					PC = pagePC;
					addressBus = PC;
					CpuRead();
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
			addressBus = 0x0100 | rS;
			// rS = op1; //wtf
			CpuRead();

			dataBus = PC >> 8;
			CpuWrite();

			addressBus = 0x0100 | uint8_t(addressBus - 1);
			dataBus = PC;
			CpuWrite();

			addressBus = PC;
			CpuRead();

			rS -= 2;
			PC = op1 + (op2 << 8);
			break;
		case 0x40: //RTI
			++PC;
			addressBus = 0x0100 | rS;
			CpuRead();

			addressBus = 0x0100 | uint8_t(addressBus + 1);
			CpuRead();

			rP = dataBus | 0x20;
			addressBus = 0x0100 | uint8_t(addressBus + 1);
			CpuRead();

			rS += 3;
			addressBus = 0x0100 | uint8_t(addressBus + 1);
			CpuRead();

			PC = cpuMem[0x0100 | uint8_t(addressBus - 1)] | (dataBus << 8);
			break;
		case 0x60: //RTS
			++PC;
			addressBus = 0x0100 | rS;
			CpuRead();

			addressBus = 0x0100 | uint8_t(addressBus + 1);
			CpuRead();

			rS += 2;
			addressBus = 0x0100 | uint8_t(addressBus + 1);
			CpuRead();

			PC = cpuMem[0x0100 | addressBus - 1] | (dataBus << 8);
			addressBus = PC;
			CpuRead();

			++PC;
			break;

		case 0x4C: //JMP abs
			++PC;
			++addressBus;
			CpuRead();

			PC = op1 | (op2 << 8);
			break;
		case 0x6C: //JMP ind
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			CpuRead();

			addressBus = uint8_t(op1 + 1) | (op2 << 8);
			CpuRead();

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
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			CpuRead();

			addressBus = uint8_t(addressBus + 1);
			CpuRead();

			addressBus = cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8);
			dataBus = rA;
			CpuWrite();
			break;
		case 0x85: //STA zp
			++PC;
			addressBus = op1;
			dataBus = rA;
			CpuWrite();
			break;
		case 0x8D: //STA abs
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			dataBus = rA;
			CpuWrite();
			break;
		case 0x91: //STA (ind),y
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + 1);
			CpuRead();

			addressBus = uint8_t(cpuMem[uint8_t(addressBus - 1)] + rY) | (cpuMem[addressBus] << 8);
			CpuRead();

			addressBus += cpuMem[op1] + rY & 0x0100;
			dataBus = rA;
			CpuWrite();
			break;
		case 0x95: //STA zp,x
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			dataBus = rA;
			CpuWrite();
			break;
		case 0x99: //STA abs,y
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = uint8_t(op1 + rY) | (op2 << 8);
			CpuRead();

			addressBus += op1 + rY & 0x0100;
			dataBus = rA;
			CpuWrite();
			break;
		case 0x9D: //STA abs,x
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = uint8_t(op1 + rX) | (op2 << 8);
			CpuRead();

			addressBus += op1 + rX & 0x0100;
			dataBus = rA;
			CpuWrite();
			break;

		case 0x84: //STY zp
			++PC;
			addressBus = op1;
			dataBus = rY;
			CpuWrite();
			break;
		case 0x8C: //STY abs
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			dataBus = rY;
			CpuWrite();
			break;
		case 0x94: //STY zp,x
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			dataBus = rY;
			CpuWrite();
			break;

		case 0x86: //STX zp
			++PC;
			addressBus = op1;
			dataBus = rX;
			CpuWrite();
			break;
		case 0x8E: //STX abs
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			dataBus = rX;
			CpuWrite();
			break;
		case 0x96: //STX zp,y
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rY);
			dataBus = rX;
			CpuWrite();
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

		// case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA: //NOP
			// break;

		case 0x06: case 0x26: case 0x46: case 0x66: case 0xC6: case 0xE6: //z RW
			++PC;
			addressBus = op1;
			CpuRead();

			CpuWrite();
			break;
		case 0x0E: case 0x2E: case 0x4E: case 0x6E: case 0xCE: case 0xEE: //abs RW
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			CpuRead();

			CpuWrite();
			break;
		case 0x16: case 0x36: case 0x56: case 0x76: case 0xD6: case 0xF6: //z,x RW
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			CpuRead();

			CpuWrite();
			break;
		case 0x1E: case 0x3E: case 0x5E: case 0x7E: case 0xDE: case 0xFE: //abs,x RW
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = uint8_t(op1 + rX) | (op2 << 8);
			CpuRead();

			addressBus = op1 + rX + (op2 << 8);
			CpuRead();

			CpuWrite();
			break;

		case 0x01: case 0x21: case 0x41: case 0x61: //(indirect,x) / indexed indirect R
		case 0xA1: case 0xC1: case 0xE1: case 0xA3:
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			CpuRead();

			addressBus = uint8_t(addressBus + 1);
			CpuRead();

			addressBus = cpuMem[uint8_t(addressBus - 1)] | (cpuMem[addressBus] << 8);
			CpuRead();
			break;
		case 0x04: case 0x05: case 0x24: case 0x25: case 0x44: //zero page R
		case 0x45: case 0x64: case 0x65: case 0xA4: case 0xA5:
		case 0xA6: case 0xC4: case 0xC5: case 0xE4: case 0xE5:
		case 0xA7:
			++PC;
			addressBus = op1;
			CpuRead();
			break;
		case 0x09: case 0x29: case 0x49: case 0x69: case 0x80: case 0xA0: //immediate R
		case 0xA2: case 0xA9: case 0xC0: case 0xC9: case 0xE0: case 0xE9: case 0xEB:
			++PC;
			break;
		case 0x0C: case 0x0D: case 0x2C: case 0x2D: case 0x4D: case 0x6D: //absolute R
		case 0xAC: case 0xAD: case 0xAE: case 0xCC: case 0xCD: case 0xEC: case 0xED:
		case 0xAF:
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = op1 | (op2 << 8);
			CpuRead();
			break;
		case 0x11: case 0x31: case 0x51: //(indirect),y / indirect indexed R
		case 0x71: case 0xB1: case 0xD1: case 0xF1: case 0xB3:
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + 1);
			CpuRead();

			addressBus = uint8_t(cpuMem[uint8_t(addressBus - 1)] + rY) | (cpuMem[addressBus] << 8);
			CpuRead();

			if(cpuMem[op1] + rY > 0xFF)
			{
				addressBus += 0x0100;
				CpuRead();
			}
			break;
		case 0x14: case 0x15: case 0x34: case 0x35: //zero page,X R
		case 0x54: case 0x55: case 0x74: case 0x75: case 0xB4:
		case 0xB5: case 0xD4: case 0xD5: case 0xF4: case 0xF5:
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rX);
			CpuRead();
			break;
		case 0x19: case 0x39: case 0x59: case 0x79: //absolute,Y R
		case 0xB9: case 0xBE: case 0xD9: case 0xF9: case 0xBF:
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = uint8_t(op1 + rY) + (op2 << 8);
			CpuRead();

			if(op1 + rY > 0xFF)
			{
				addressBus += 0x0100;
				CpuRead();
			}
			break;
		case 0x1C: case 0x1D: case 0x3C: case 0x3D: //absolute,X R
		case 0x5C: case 0x5D: case 0x7C: case 0x7D: case 0xBC:
		case 0xBD: case 0xDC: case 0xDD: case 0xFC: case 0xFD:
			++PC;
			++addressBus;
			CpuRead();

			++PC;
			addressBus = uint8_t(op1 + rX) + (op2 << 8);
			CpuRead();

			if(op1 + rX > 0xFF)
			{
				addressBus += 0x0100;
				CpuRead();
			}
			break;
		case 0xB6: case 0xB7: //zero page,Y R
			++PC;
			addressBus = op1;
			CpuRead();

			addressBus = uint8_t(addressBus + rY);
			CpuRead();
			break;
	}

	switch(opcode)
	{
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

		case 0xE1: case 0xE5: case 0xE9: case 0xED: //sbc
		case 0xF1: case 0xF5: case 0xF9: case 0xFD: case 0xEB:
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

		case 0x46: case 0x4E: case 0x56: case 0x5E: //lsr
			rP[0] = dataBus & 0x01;
			dataBus >>= 1;
			rP[1] = !dataBus;
			rP.reset(7);
			CpuWrite();
			break;

		case 0x06: case 0x0E: case 0x16: case 0x1E: //asl
			rP[0] = dataBus & 0x80;
			dataBus <<= 1;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite();
			break;

		case 0x26: case 0x2E: case 0x36: case 0x3E: //rol
			{
			const bool newCarry = dataBus & 0x80;
			dataBus = (dataBus << 1) | rP[0];
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite();
			break;

		case 0x66: case 0x6E: case 0x76: case 0x7E: //ror
			{
			const bool newCarry = dataBus & 0x01;
			dataBus = (dataBus >> 1) | (rP[0] << 7);
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite();
			break;

		case 0xC6: case 0xCE: case 0xD6: case 0xDE: //dec
			--dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite();
			break;

		case 0xE6: case 0xEE: case 0xF6: case 0xFE: //inc
			++dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite();
			break;
	}
	CpuOpDone();
}


void nes::CpuRead()
{
	dataBus = cpuMem[addressBus];

	switch(addressBus)
	{
		case 0x2002:
			if(scanlineV == 241 && scanlineH == 1)
			{
				ppuStatus &= 0x7F;
				nmiSuppress = true;
			}
			dataBus = ppuStatus;
			ppuStatus &= 0x7F; //clear nmi_occurred bit
			wToggle = false;
			break;
		case 0x2007:
			if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
			{
				//if in palette range, reload latch(mirrored) but send palette byte immediately
				//perform NT mirroring here
				dataBus = ppudata_latch;
				ppudata_latch = vram[ppu_address];
				ppu_address += (ppuCtrl & 0x04) ? 0x20 : 0x01;
			}
			break;

		case 0x4016:
			dataBus = (addressBus & 0xE0) | (controller_reg & 1); //addressBus = open bus?
			controller_reg >>= 1; //if we have read 4016 8 times, start returning 1s
			break;
	}

	CpuTick();
}


void nes::CpuWrite() // block writes to ppu on the first screen
{
	cpuMem[addressBus] = dataBus;

	switch(addressBus)
	{
		case 0x2000:
			ppuCtrl = dataBus;
			ppu_address_latch = (ppu_address_latch & 0x73FF) | ((ppuCtrl & 0x03) << 10);
			break;
		case 0x2001:
			ppuMask = dataBus;
			break;
		case 0x2003: //OAMADDR
			oamAddr = dataBus;
			break;
		case 0x2004: //OAMDATA
			oam[oamAddr] = dataBus;
			++oamAddr;
			break;
		case 0x2005: //PPUSCROLL
			if(!wToggle)
			{
				ppu_address_latch = (ppu_address_latch & 0x7FE0) | (dataBus >> 3);
				fineX = dataBus & 0x07;
			}
			else
			{
				ppu_address_latch = (ppu_address_latch & 0xC1F) | ((dataBus & 0x07) << 12) | ((dataBus & 0xF8) << 2);
			}
			wToggle = !wToggle;
			break;
		case 0x2006:
			if(!wToggle)
			{
				ppu_address_latch = (ppu_address_latch & 0xFF) | ((dataBus & 0x3F) << 8);
			}
			else
			{
				ppu_address_latch = (ppu_address_latch & 0x7F00) | dataBus;
				ppu_address = ppu_address_latch & 0x3FFF; //assuming only the address gets mirrored down, not the latch
			}
			wToggle = !wToggle;
			break;
		case 0x2007:
			if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
			{
				vram[ppu_address] = dataBus; //perform NT mirroring here
				ppu_address += (ppuCtrl & 0x04) ? 0x20 : 0x01;
			}
			break;
		case 0x4014:
			// if(cycles & 1) cpu_tick(); //+1 cycle if dma started on an odd cycle
			CpuTick(); //tick or read?
			for(uint16_t x=0; x<=255; ++x) //only toggle bool here, move this to CpuOpDone?
			{
				addressBus = (dataBus << 8) + x;
				CpuRead();

				addressBus = 0x2004;
				CpuWrite();
			}
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
	}

	CpuTick();
}


void nes::CpuTick()
{
	PpuTick();
	PpuTick();
	PpuTick();
}


void nes::CpuOpDone()
{
	if(nmiLine)
	{
		addressBus = PC;
		CpuRead();

		++PC;
		++addressBus;
		CpuRead();

		++PC;
		addressBus = 0x0100 | rS;
		dataBus = PC >> 8;
		CpuWrite();

		addressBus = 0x0100 | uint8_t(addressBus - 1);
		dataBus = PC;
		CpuWrite();

		addressBus = 0x0100 | uint8_t(addressBus - 1);
		dataBus = rP.to_ulong();
		CpuWrite();

		rS -= 3;
		addressBus = 0xFFFA;
		CpuRead();

		++addressBus;
		CpuRead();

		PC = cpuMem[0xFFFA] | (dataBus << 8);

		nmiLine = false;
	}
}


void nes::PpuTick()
{
	if(scanlineV < 240) //visible scanlines
	{
		if(scanlineH && scanlineH <= 256)
		{
			uint8_t ppuPixel = (ppu_bg_low >> (15 - fineX)) & 1;
			ppuPixel |= (ppu_bg_high >> (14 - fineX)) & 2;
			ppuPixel |= (ppu_attribute >> (28 - fineX * 2)) & 0x0C;

			const uint32_t renderPos = (scanlineV * 256 + (scanlineH-1)) * 3;
			const uint16_t pIndex = vram[0x3F00 + ppuPixel] * 3;
			render[renderPos  ] = palette[pIndex  ];
			render[renderPos+1] = palette[pIndex+1];
			render[renderPos+2] = palette[pIndex+2];
		}

		PpuRenderFetches();
		// PpuOamScan();
	}
	else if(scanlineV == 241) //vblank scanlines
	{
		if(scanlineH == 1)
		{
			if(!nmiSuppress)
			{
				ppuStatus |= 0x80; //VBL nmi
				nmiLine = (ppuCtrl & ppuStatus) & 0x80;
			}
			else
			{
				nmiSuppress = false;
			}
		}
	}
	else if(scanlineV == 261) //prerender scanline
	{
		if(scanlineH == 1)
		{
			ppuStatus &= 0x1F; //clear sprite overflow, sprite 0 hit and vblank
		}
		else if(scanlineH >= 280 && scanlineH <= 304 && ppuMask & 0x18)
		{
			ppu_address = (ppu_address & 0x41F) | (ppu_address_latch & 0x7BE0);
		}

		PpuRenderFetches();
		// PpuOamScan();

		if(ppu_odd_frame && scanlineH == 339 && ppuMask & 0x18)
		{
			++scanlineH;
		}
	}

	if(scanlineH++ == 340)
	{
		scanlineH = 0;
		if(scanlineV++ == 261)
		{
			scanlineV = 0;
			ppu_odd_frame = !ppu_odd_frame;
			renderFrame = true;
		}
	}
}


void nes::PpuRenderFetches() //things done during visible and prerender scanlines
{
	if(ppuMask & 0x18) //if rendering is enabled
	{
		if(scanlineH == 256) //Y increment
		{
			if(ppu_address < 0x7000)
			{
				ppu_address += 0x1000;
			}
			else
			{
				ppu_address &= 0xFFF;
				if((ppu_address & 0x3E0) == 0x3A0)
				{
					ppu_address &= 0xC1F;
					ppu_address ^= 0x800;
				}
				else if((ppu_address & 0x3E0) == 0x3E0)
				{
					ppu_address &= 0xC1F;
				}
				else
				{
					ppu_address += 0x20;
				}
			}
		}
		else if(scanlineH == 257)
		{
			ppu_address = (ppu_address & 0x7BE0) | (ppu_address_latch & 0x41F);
		}

		if((scanlineH >= 1 && scanlineH <= 257) || (scanlineH >= 321 && scanlineH <= 339))
		{
			if(!(scanlineH & 0x07)) //coarse X increment
			{
				if((ppu_address & 0x1F) != 0x1F)
				{
					++ppu_address;
				}
				else
				{
					ppu_address = (ppu_address & 0x7FE0) ^ 0x400;
				}
			}

			switch(scanlineH & 0x07)
			{
				case 1:
					if(scanlineH != 1 && scanlineH != 321) 
					{
						ppu_bg_low |= ppu_bg_low_latch;
						ppu_bg_high |= ppu_bg_high_latch;
						ppu_attribute |= (ppu_attribute_latch & 0x03) * 0x5555; //2-bit splat
					}
					ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					break;
				case 3:
					if(scanlineH != 339)
					{
						ppu_attribute_latch = vram[0x23C0 | (ppu_address & 0xC00) | (ppu_address >> 4 & 0x38) | (ppu_address >> 2 & 0x07)];
						ppu_attribute_latch >>= (((ppu_address >> 1) & 0x01) | ((ppu_address >> 5) & 0x02)) * 2;
					}
					else
					{
						ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					}
					break;
				case 5:
					ppu_bg_address = (ppu_nametable*16 + (ppu_address >> 12)) | ((ppuCtrl & 0x10) << 8);
					ppu_bg_low_latch = vram[ppu_bg_address];
					break;
				case 7:
					ppu_bg_high_latch = vram[ppu_bg_address + 8];
					break;
			}
		}

		if((scanlineH >= 1 && scanlineH <= 256) || (scanlineH >= 321 && scanlineH <= 336))
		{
			ppu_bg_low <<= 1;
			ppu_bg_high <<= 1;
			ppu_attribute <<= 2;
		}
	}

	if(scanlineH >= 257 && scanlineH <= 320) //rendering enabled only?
	{
		oamAddr = 0;
	}
}


void nes::PpuOamScan()
{
	if(scanlineH && !(scanlineH & 1))
	{
		if(scanlineH <= 64)
		{
			oam2[(scanlineH >> 1) - 1] = 0xFF;
		}
		else if(scanlineH <= 256)
		{
			switch(oam_eval_pattern)
			{
				case 0:
					oam2[oam2Index] = oam[oam_spritenum];
					if(scanlineV >= oam[oam_spritenum] && scanlineV < oam[oam_spritenum] + 8 + (ppuCtrl >> 2 & 8))
					{
						++oam_eval_pattern;
					}
					else
					{
						PpuOamUpdateIndex();
					}
					break;
				case 1: case 2: case 3:
					oam2[++oam2Index] = oam[oam_spritenum + oam_eval_pattern++];
					if(oam_eval_pattern > 3)
					{
						++oam2Index;
						PpuOamUpdateIndex();						
					}
					break;
				case 4:
					oam_spritenum += 4;
					break;
			}
		}
		else if(scanlineH <= 320)
		{
		}
		else
		{
			oam2Index = 0;
			oam_spritenum = 0;
			oam_eval_pattern = 0;			
		}
	}
}


void nes::PpuOamUpdateIndex()
{
	oam_spritenum += 4;
	if(!oam_spritenum)
	{
		oam_eval_pattern = 4;
	}
	else if(oam2Index < 32)
	{
		oam_eval_pattern = 0;
	}
	else if(oam2Index == 32)
	{
		oam2Index = 0;
		oam_block_writes = true; //use this
		// if we find 8 sprites in range, keep scanning to set spr overflow
	}
}
