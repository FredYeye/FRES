// #define DEBUG
// #define DUMP_VRAM

#include "nes.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>


nes::nes(std::string inFile)
{
	LoadRom(inFile);
	PC = cpu_mem[0xFFFC] + (cpu_mem[0xFFFD] << 8);
	rS = 0xFD;
	rP = 0x34;
}


void nes::AdvanceFrame(uint8_t input)
{
	renderFrame = false;
	while(!renderFrame)
	{
		runOpcode();

		if(controller_update) //doesn't work outside while
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

	std::string contentStr = contents.str();
	std::vector<uint8_t> contentVec(contentStr.begin(), contentStr.end());


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


	std::copy_n(contentVec.begin() + 0x10, 0x4000, cpu_mem.begin() + 0x8000);
	std::copy_n(contentVec.begin() + 0x4010, 0x2000, vram.begin());
	std::copy(cpu_mem.begin()+0x8000, cpu_mem.begin()+0xC000, cpu_mem.begin()+0xC000);
}


const std::array<uint8_t, 256*240*3>* const nes::GetPixelPtr() const
{
	return &render;
}


void nes::runOpcode()
{
	bool alu = false;

	uint8_t opcode = cpu_mem[PC];
	uint8_t op1 = cpu_mem[PC+1];
	uint8_t op2 = cpu_mem[PC+2];

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
		 << " PPU:" << std::setw(3) << std::dec << scanline_h
		 << " SL:" << std::setw(3) << std::dec << scanline_v << std::endl;
	#endif
	#ifdef DUMP_VRAM
	if(scanline_v == 261) {
		std::string outfile = "vram.txt";
		std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
		result.write((char*)&vram[0],0x4000);
		result.close();
	}
	#endif

	++PC;
	cpu_read();

	switch(opcode)
	{
		// opcodes left
		// 03 07 0B 0F
		// 13 17 1B 1F
		// 23 27 2B 2F
		// 33 37 3B 3F
		// 43 47 4B 4F
		// 53 57 5B 5F
		// 63 67 6B 6F
		// 73 77 7B 7F
		// 82 83 87 89 8B 8F
		// 93 97 9B 9C 9E 9F
		// A3 A7 AB AF
		// B3 B7 BB BF
		// C2 C3 C7 CB CF
		// D3 D7 DB DF
		// E2 E3 E7 EB EF
		// F3 F7 FB FF

		case 0x00: //BRK
			++PC;
			cpu_read();
			cpu_mem[0x100+rS] = PC >> 8;
			--rS;
			cpu_write();
			cpu_mem[0x100+rS] = PC;
			--rS;
			cpu_write();
			rP.set(4);
			cpu_mem[0x100+rS] = rP.to_ulong();
			--rS;
			cpu_write();
			PC = cpu_mem[0xFFFE];
			rP.set(2);
			cpu_read();
			PC += cpu_mem[0xFFFF] << 8;
			cpu_read();
			break;
		case 0x06: //ASL
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1] & 0x80;
			cpu_mem[op1] <<= 1;
			rP[1] = !cpu_mem[op1];
			rP[7] = cpu_mem[op1] & 0x80;
			cpu_write();
			break;
		case 0x08: //PHP
			cpu_read();
			rP.set(4);
			cpu_mem[0x100+rS] = rP.to_ulong();
			--rS;
			cpu_write();
			break;
		case 0x0A: //ASL
			cpu_read();
			rP[0] = rA & 0x80;
			rA <<= 1;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x0E: //ASL
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_mem[op1 + (op2 << 8)] <<= 1;
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_write();
			break;
		case 0x10: //BPL
			++PC;
			cpu_read();
			if(!rP.test(7))
			{
				uint8_t prevPCH = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPCH)
				{
					cpu_read();
				}
			}
			break;
		case 0x16: //ASL
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_mem[op1 + rX & 0xFF] <<= 1;
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_write();
			break;
		case 0x18: //CLC
			cpu_read();
			rP.reset(0);
			break;
		case 0x1E: //ASL
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] <<= 1;
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_write();
			break;
		case 0x20: //JSR
			++PC;
			cpu_read();
			cpu_read();
			cpu_mem[0x100+rS] = PC >> 8;
			--rS;
			cpu_write();
			cpu_mem[0x100+rS] = PC;
			--rS;
			cpu_write();
			PC = op1 + (op2 << 8);
			cpu_read();
			break;
		case 0x26: //ROL
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1];
			cpu_mem[op1] = (cpu_mem[op1] << 1) | rP[0];
			rP[0] = prevMem & 0x80;
			}
			rP[1] = !cpu_mem[op1];
			rP[7] = cpu_mem[op1] & 0x80;
			cpu_write();
			break;
		case 0x28: //PLP
			cpu_read();
			++rS;
			cpu_read();
			rP = cpu_mem[0x100+rS] | 0x20;
			cpu_read();
			break;
		case 0x2A: //ROL
			cpu_read();
			{
			uint8_t prevrA = rA;
			rA = (rA << 1) | rP[0];
			rP[0] = prevrA & 0x80;
			}
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x2E: //ROL
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + (op2 << 8)];
			cpu_mem[op1 + (op2 << 8)] = (cpu_mem[op1 + (op2 << 8)] << 1) | rP[0];
			rP[0] = prevMem & 0x80;
			}
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_write();
			break;
		case 0x30: //BMI
			++PC;
			cpu_read();
			if(rP.test(7))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0x36: //ROL
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + rX & 0xFF];
			cpu_mem[op1 + rX & 0xFF] = (cpu_mem[op1 + rX & 0xFF] << 1) | rP[0];
			rP[0] = prevMem & 0x80;
			}
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_write();
			break;
		case 0x38: //SEC
			cpu_read();
			rP.set(0);
			break;
		case 0x3E: //ROL
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] = (cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] << 1) | rP[0];
			rP[0] = prevMem & 0x80;
			}
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_write();
			break;
		case 0x40: //RTI
			cpu_read();
			++rS;
			cpu_read();
			rP = cpu_mem[0x100+rS] | 0x20;
			++rS;
			cpu_read();
			PC = cpu_mem[0x100+rS];
			++rS;
			cpu_read();
			PC += cpu_mem[0x100+rS] << 8;
			cpu_read();
			break;
		case 0x46: //LSR
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1] & 0x01;
			cpu_mem[op1] >>= 1;
			rP[1] = !cpu_mem[op1];
			rP.reset(7);
			cpu_write();
			break;
		case 0x48: //PHA
			cpu_read();
			cpu_mem[0x100+rS] = rA; //can do rS--
			--rS;
			cpu_write();
			break;
		case 0x4A: //LSR
			cpu_read();
			rP[0] = rA & 0x01;
			rA >>= 1;
			rP[1] = !rA;
			rP.reset(7);
			break;
		case 0x4C: //JMP
			++PC;
			cpu_read();
			PC = op1 + (op2 << 8);
			cpu_read();
			break;
		case 0x4E: //LSR
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + (op2 << 8)] & 0x01;
			cpu_mem[op1 + (op2 << 8)] >>= 1;
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP.reset(7);
			cpu_write();
			break;
		case 0x50: //BVC
			++PC;
			cpu_read();
			if(!rP.test(6))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0x56: //LSR
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + rX & 0xFF] & 0x01;
			cpu_mem[op1 + rX & 0xFF] >>= 1;
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP.reset(7);
			cpu_write();
			break;
		case 0x5E: //LSR
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			rP[0] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x01;
			cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] >>= 1;
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP.reset(7);
			cpu_write();
			break;
		case 0x60: //RTS
			cpu_read();
			++rS;
			cpu_read();
			PC = cpu_mem[0x100+rS];
			++rS;
			cpu_read();
			PC += cpu_mem[0x100+rS] << 8;
			cpu_read();
			++PC;
			cpu_read();
			break;
		case 0x66: //ROR
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1];
			cpu_mem[op1] = (cpu_mem[op1] >> 1) | (rP[0] << 7);
			rP[0] = prevMem & 0x01;
			}
			rP[1] = !cpu_mem[op1];
			rP[7] = cpu_mem[op1] & 0x80;
			cpu_write();
			break;
			case 0x68: //PLA
			cpu_read();
			++rS;
			cpu_read();
			rA = cpu_mem[0x100+rS];
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			cpu_read();
			break;
		case 0x6A: //ROR
			cpu_read();
			{
			uint8_t prevrA = rA;
			rA = (rA >> 1) | (rP[0] << 7);
			rP[0] = prevrA & 0x01;
			}
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x6C: //JMP
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			PC = cpu_mem[op1 + (op2 << 8)];
			PC += cpu_mem[(op1 + 1 & 0xFF) + (op2 << 8)] << 8;
			cpu_read();
			break;
		case 0x6E: //ROR
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + (op2 << 8)];
			cpu_mem[op1 + (op2 << 8)] = (cpu_mem[op1 + (op2 << 8)] >> 1) | (rP[0] << 7);
			rP[0] = prevMem & 0x01;
			}
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_write();
			break;
		case 0x70: //BVS
			++PC;
			cpu_read();
			if(rP.test(6))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0x76: //ROR
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + rX & 0xFF];
			cpu_mem[op1 + rX & 0xFF] = (cpu_mem[op1 + rX & 0xFF] >> 1) | (rP[0] << 7);
			rP[0] = prevMem & 0x01;
			}
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_write();
			break;
		case 0x78: //SEI
			cpu_read();
			rP.set(2);
			break;
		case 0x7E: //ROR
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			{
			uint8_t prevMem = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] = (cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] >> 1) | (rP[0] << 7);
			rP[0] = prevMem & 0x01;
			}
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_write();
			break;
		case 0x81: //STA
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_read();
			address = cpu_mem[op1 + rX & 0xFF] + (cpu_mem[op1 + rX + 1 & 0xFF] << 8);
			content = rA;
			cpu_mem[address] = rA;
			cpu_write();
			break;
		case 0x84: //STY
			++PC;
			cpu_read();
			cpu_mem[op1] = rY;
			cpu_write();
			break;
		case 0x85: //STA
			++PC;
			cpu_read();
			cpu_mem[op1] = rA;
			cpu_write();
			break;
		case 0x86: //STX
			++PC;
			cpu_read();
			cpu_mem[op1] = rX;
			cpu_write();
			break;
		case 0x88: //DEY
			cpu_read();
			--rY;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0x8A: //TXA
			cpu_read();
			rA = rX;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x8C: //STY
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = op1 + (op2 << 8);
			content = rY;
			cpu_mem[address] = rY;
			cpu_write();
			break;
		case 0x8D: //STA
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = op1 + (op2 << 8);
			content = rA;
			cpu_mem[address] = rA;
			cpu_write();
			break;
		case 0x8E: //STX
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = op1 + (op2 << 8);
			content = rX;
			cpu_mem[op1 + (op2 << 8)] = rX;
			cpu_write();
			break;
		case 0x90: //BCC
			++PC;
			cpu_read();
			if(!rP.test(0))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0x91: //STA
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_read();
			address = cpu_mem[op1] + (cpu_mem[op1 + 1 & 0xFF] << 8) + rY;
			content = rA;
			cpu_mem[address] = rA;
			cpu_write();
			break;
		case 0x94: //STY
			++PC;
			cpu_read();
			cpu_read();
			cpu_mem[op1 + rX & 0xFF] = rY;
			cpu_write();
			break;
		case 0x95: //STA
			++PC;
			cpu_read();
			cpu_read();
			cpu_mem[op1 + rX & 0xFF] = rA;
			cpu_write();
			break;
		case 0x96: // STX
			++PC;
			cpu_read();
			cpu_read();
			cpu_mem[op1 + rY & 0xFF] = rX;
			cpu_write();
			break;
		case 0x98: //TYA
			cpu_read();
			rA = rY;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
			break;
		case 0x99: //STA
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			address = op1 + (op2 << 8) + rY;
			content = rA;
			cpu_mem[address] = rA;
			cpu_write();
			break;
		case 0x9A: //TXS
			cpu_read();
			rS = rX;
			break;
		case 0x9D: //STA
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			address = op1 + (op2 << 8) + rX;
			content = rA;
			cpu_mem[address] = rA;
			cpu_write();
			break;
		case 0xA8: //TAY
			cpu_read();
			rY = rA;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0xAA: //TAX
			cpu_read();
			rX = rA;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0xB0: //BCS
			++PC;
			cpu_read();
			if(rP.test(0))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0xB8: //CLV
			cpu_read();
			rP.reset(6);
			break;
		case 0xBA: //TSX
			cpu_read();
			rX = rS;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0xC6: //DEC
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			--cpu_mem[op1];
			rP[1] = !cpu_mem[op1];
			rP[7] = cpu_mem[op1] & 0x80;
			cpu_write();
			break;
		case 0xC8: //INY
			cpu_read();
			++rY;
			rP[1] = !rY;
			rP[7] = rY & 0x80;
			break;
		case 0xCA: //DEX
			cpu_read();
			--rX;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0xCE: //DEC
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			--cpu_mem[op1 + (op2 << 8)];
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_write();
			break;
		case 0xD0: //BNE
			++PC;
			cpu_read();
			if(!rP.test(1))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0xD6: //DEC
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			--cpu_mem[op1 + rX & 0xFF];
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_write();
			break;
		case 0xD8: //CLD
			cpu_read();
			rP.reset(3);
			break;
		case 0xDE: //DEC
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			--cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_write();
			break;
		case 0xE6: //INC
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			++cpu_mem[op1];
			rP[1] = !cpu_mem[op1];
			rP[7] = cpu_mem[op1] & 0x80;
			cpu_write();
			break;
		case 0xE8: //INX
			cpu_read();
			++rX;
			rP[1] = !rX;
			rP[7] = rX & 0x80;
			break;
		case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xEA: case 0xFA: //NOP
			cpu_read();
			break;
		case 0xEE: //INC
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_write();
			++cpu_mem[op1 + (op2 << 8)];
			rP[1] = !cpu_mem[op1 + (op2 << 8)];
			rP[7] = cpu_mem[op1 + (op2 << 8)] & 0x80;
			cpu_write();
			break;
		case 0xF0: //BEQ
			++PC;
			cpu_read();
			if(rP.test(1))
			{
				uint8_t prevPC = PC >> 8;
				PC += int8_t(op1);
				cpu_read();
				if((PC >> 8) != prevPC)
				{
					cpu_read();
				}
			}
			break;
		case 0xF6: //INC
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			++cpu_mem[op1 + rX & 0xFF];
			rP[1] = !cpu_mem[op1 + rX & 0xFF];
			rP[7] = cpu_mem[op1 + rX & 0xFF] & 0x80;
			cpu_write();
			break;
		case 0xF8: //SED
			cpu_read();
			rP.set(3);
			break;
		case 0xFE: //INC
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_write();
			++cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[1] = !cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF];
			rP[7] = cpu_mem[op1 + (op2 << 8) + rX & 0xFFFF] & 0x80;
			cpu_write();
			break;

		case 0x01: case 0x21: case 0x41: case 0x61:
		case 0xA1: case 0xC1: case 0xE1: //(indirect,x) / indexed indirect
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			cpu_read();
			address = cpu_mem[op1 + rX & 0xFF] + (cpu_mem[op1 + rX + 1 & 0xFF] << 8);
			content = cpu_mem[address];
			cpu_read();
			alu = true;
			break;

		case 0x04: case 0x05: case 0x24: case 0x25: case 0x44: //zero page
		case 0x45: case 0x64: case 0x65: case 0xA4: case 0xA5:
		case 0xA6: case 0xC4: case 0xC5: case 0xE4: case 0xE5:
			++PC;
			cpu_read();
			content = cpu_mem[op1];
			cpu_read();
			alu = true;
			break;

		case 0x09: case 0x29: case 0x49: case 0x69: case 0x80: case 0xA0: //immediate
		case 0xA2: case 0xA9: case 0xC0: case 0xC9: case 0xE0: case 0xE9:
			++PC;
			cpu_read();
			content = op1;
			alu = true;
			break;

		case 0x0C: case 0x0D: case 0x2C: case 0x2D: case 0x4D: case 0x6D: //absolute
		case 0xAC: case 0xAD: case 0xAE: case 0xCC: case 0xCD: case 0xEC: case 0xED:
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = op1 + (op2 << 8);
			content = cpu_mem[address];
			cpu_read();
			alu = true;
			break;

		case 0x11: case 0x31: case 0x51: //(indirect),y / indirect indexed
		case 0x71: case 0xB1: case 0xD1: case 0xF1:
			++PC;
			cpu_read();
			cpu_read();
			cpu_read();
			address = (cpu_mem[op1] + rY & 0xFF) + (cpu_mem[op1 + 1 & 0xFF] << 8);
			content = cpu_mem[address];
			cpu_read();
			if((cpu_mem[op1] + rY) > 0xFF)
			{
				address = cpu_mem[op1] + (cpu_mem[op1 + 1 & 0xFF] << 8) + rY; // address = address + 0x100 & 0xFFFF; //address gets reset. recalculate
				content = cpu_mem[address];
				cpu_read();
			}
			alu = true;
			break;

		case 0x14: case 0x15: case 0x34: case 0x35: //zero page,X
		case 0x54: case 0x55: case 0x74: case 0x75: case 0xB4:
		case 0xB5: case 0xD4: case 0xD5: case 0xF4: case 0xF5:
			++PC;
			cpu_read();
			cpu_read();
			content = cpu_mem[op1 + rX & 0xFF];
			cpu_read();
			alu = true;
			break;

		case 0x19: case 0x39: case 0x59: case 0x79: //absolute,Y
		case 0xB9: case 0xBE: case 0xD9: case 0xF9:
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = (op1 + rY & 0xFF) + (op2 << 8);
			content = cpu_mem[address];
			cpu_read();
			if((op1 + rY) > 0xFF)
			{
				address = op1 + (op2 << 8) + rY;
				content = cpu_mem[address];
				cpu_read();
			}
			alu = true;
			break;

		case 0x1C: case 0x1D: case 0x3C: case 0x3D: //absolute,X
		case 0x5C: case 0x5D: case 0x7C: case 0x7D: case 0xBC:
		case 0xBD: case 0xDC: case 0xDD: case 0xFC: case 0xFD:
			++PC;
			cpu_read();
			++PC;
			cpu_read();
			address = (op1 + rX & 0xFF) + (op2 << 8);
			content = cpu_mem[address];
			cpu_read();
			if((op1 + rX) > 0xFF)
			{
				address = op1 + (op2 << 8) + rX;
				content = cpu_mem[address];
				cpu_read();
			}
			alu = true;
			break;

		case 0xB6: //zero page,Y
			++PC;
			cpu_read();
			cpu_read();
			content = cpu_mem[op1 + rY & 0xFF];
			cpu_read();
			alu = true;
			break;

		default:
			std::cout << "MELTDOWN " << std::hex << int(opcode) << std::endl;
			exit(0);
	}

	if(alu)
	{
		switch(opcode)
		{
			case 0x61: case 0x65: case 0x69: case 0x6D:
			case 0x71: case 0x75: case 0x79: case 0x7D: //adc
				{
				uint8_t prevrA = rA;
				rA += content + rP[0];
				rP[0] = (prevrA + content + rP[0]) & 0x100;
				rP[1] = !rA;
				rP[6] = (prevrA ^ rA) & (content ^ rA) & 0x80;
				}
				rP[7] = rA & 0x80;
				break;

			case 0x21: case 0x25: case 0x29: case 0x2D:
			case 0x31: case 0x35: case 0x39: case 0x3D: //and
				rA &= content;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;

			case 0xC1: case 0xC5: case 0xC9: case 0xCD:
			case 0xD1: case 0xD5: case 0xD9: case 0xDD: //cmp
				rP[0] = (rA >= content) ? 1 : 0;
				rP[1] = !(rA - content);
				rP[7] = rA - content & 0x80;
				break;

			case 0x41: case 0x45: case 0x49: case 0x4D:
			case 0x51: case 0x55: case 0x59: case 0x5D: //eor
				rA ^= content;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;

			case 0xA1: case 0xA5: case 0xA9: case 0xAD:
			case 0xB1: case 0xB5: case 0xB9: case 0xBD: //lda
				rA = content;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;

			case 0x01: case 0x05: case 0x09: case 0x0D:
			case 0x11: case 0x15: case 0x19: case 0x1D: //ora
				rA |= content;
				rP[1] = !rA;
				rP[7] = rA & 0x80;
				break;

			case 0xE1: case 0xE5: case 0xE9: case 0xED:
			case 0xF1: case 0xF5: case 0xF9: case 0xFD: //sbc
				{
				uint8_t prevrA = rA;
				rA = (rA - content) - !rP[0];
				rP[0] = ((prevrA - content) - !rP[0] < 0) ? 0 : 1;
				rP[1] = !rA;
				rP[6] = (prevrA ^ rA) & (~content ^ rA) & 0x80;
				}
				rP[7] = rA & 0x80;
				break;

			case 0xE0: case 0xE4: case 0xEC: //cpx
				rP[0] = (rX >= content) ? 1 : 0;
				rP[1] = !(rX - content);
				rP[7] = rX - content & 0x80;
				break;

			case 0xC0: case 0xC4: case 0xCC: //cpy
				rP[0] = (rY >= content) ? 1 : 0;
				rP[1] = !(rY - content);
				rP[7] = rY - content & 0x80;
				break;

			case 0xA2: case 0xA6: case 0xAE: case 0xB6: case 0xBE: //ldx
				rX = content;
				rP[1] = !rX;
				rP[7] = rX & 0x80;
				break;

			case 0xA0: case 0xA4: case 0xAC: case 0xB4: case 0xBC: //ldy
				rY = content;
				rP[1] = !rY;
				rP[7] = rY & 0x80;
				break;

			case 0x24: case 0x2C: //bit
				rP[1] = !(content & rA);
				rP[6] = content & 0x40;
				rP[7] = content & 0x80;
				break;
		}
	}
	cpu_op_done();
}


void nes::cpu_read()
{
	switch(address)
	{
		case 0x2002:
			content = ppustatus;
			ppustatus &= 0x7F;
			// cpu_mem[0x2002] = ppustatus; //probably not necessary
			w_toggle = false;
			break;
		case 0x2007:
			if((scanline_v >= 240 && scanline_v <= 260) || !(ppumask & 0x18)) {
				//if in palette range, reload latch(mirrored) but send palette byte immediately
				//perform NT mirroring here
				content = ppudata_latch;
				ppudata_latch = vram[ppu_address];
				ppu_address += (ppuctrl & 0x04) ? 0x20 : 0x01;
			}
			break;

		case 0x4016:
			content = (address & 0xE0) | (controller_reg & 1); //address = open bus?
			controller_reg >>= 1; //if we have read 4016 8 times, start returning 1s
			break;
	}

	cpu_tick();
	return;
}


void nes::cpu_write()
{ // block writes to ppu on the first screen
	switch(address)
	{
		case 0x2000:
			ppuctrl = content;
			ppu_address_latch = (ppu_address_latch & 0x73FF) | ((ppuctrl & 0x03) << 10);
			break;
		case 0x2001:
			ppumask = content;
			break;
		case 0x2005:
			if(!w_toggle)
			{
				ppu_address_latch = (ppu_address_latch & 0x7FE0) | (content >> 3);
				fine_x = content & 0x07;
			}
			else
			{
				ppu_address_latch = (ppu_address_latch & 0xC1F) | ((content & 0x07) << 12) | ((content & 0xF8) << 2);
			}
			w_toggle = !w_toggle;
			break;
		case 0x2006:
			if(!w_toggle)
			{
				ppu_address_latch = (ppu_address_latch & 0xFF) | ((content & 0x3F) << 8);
			}
			else
			{
				ppu_address_latch = (ppu_address_latch & 0x7F00) | content;
				ppu_address = ppu_address_latch & 0x3FFF; //assuming only the address gets mirrored down, not the latch
			}
			w_toggle = !w_toggle;
			break;
		case 0x2007:
			if((scanline_v >= 240 && scanline_v <= 260) || !(ppumask & 0x18))
			{
				vram[ppu_address] = content; //perform NT mirroring here
				ppu_address += (ppuctrl & 0x04) ? 0x20 : 0x01;
			}
			break;
		case 0x4014:
			// the cpu writes to oamdata (0x2004) 256 times, the cpu is suspended during the transfer.

			// if(cycles & 1) cpu_tick(); //+1 cycle if dma started on an odd cycle
			// cpu_tick();
			for(uint16_t x=0; x<=255; ++x)
			{
				// cpu_tick(); //r:0x20xx
				ppu_oam[x] = cpu_mem[(content << 8) + x];
				++oamaddr;
				// cpu_tick(); //w:0x4014
			}
			break;
		case 0x4016:
			if(content & 1)
			{
				controller_update = true;
			}
			else /*if(!(content & 1))*/
			{
				controller_update = false;
			}
			break;
	}

	cpu_tick();
	return;
}


void nes::cpu_tick()
{
	ppu_tick();
	ppu_tick();
	ppu_tick();

	address = 0; //really needed?

	return;
}


void nes::cpu_op_done()
{
	if(nmi_line)
	{
		// ++PC;
		// cpu_read();
		cpu_mem[0x100+rS] = PC >> 8;
		--rS;
		// cpu_write();
		cpu_mem[0x100+rS] = PC;
		--rS;
		// cpu_write();
		rP.reset(4);
		cpu_mem[0x100+rS] = rP.to_ulong();
		--rS;
		// cpu_write();
		PC = cpu_mem[0xFFFA];
		rP.set(2);
		// cpu_read();
		PC += cpu_mem[0xFFFB] << 8;
		// cpu_read();

		nmi_line = false;
	}

	return;
}


void nes::ppu_tick()
{
	if(scanline_v < 240)
	{ //visible scanlines
		if(scanline_v != 261 && scanline_h && scanline_h <= 256)
		{
			uint8_t ppu_pixel = (ppu_bg_low >> (15 - fine_x)) & 1;
			ppu_pixel |= (ppu_bg_high >> (14 - fine_x)) & 2;
			ppu_pixel |= (ppu_attribute >> (28 - fine_x * 2)) & 0x0C;

			uint32_t renderpos = (scanline_v*256 + (scanline_h-1)) * 3;
			uint16_t pindex = vram[0x3F00 + ppu_pixel] * 3;
			render[renderpos  ] = palette[pindex  ];
			render[renderpos+1] = palette[pindex+1];
			render[renderpos+2] = palette[pindex+2];
		}

		ppu_render_fetches();

	} else if(scanline_v == 241) { //vblank scanlines
		if(scanline_h == 1) {
			ppustatus |= 0x80;
			nmi_line = cpu_mem[0x2000] & ppustatus & 0x80;
		}
	} else if(scanline_v == 261) { //prerender scanline
		if(scanline_h == 1) {
			ppustatus &= 0x1F; //clear sprite overflow, sprite 0 hit and vblank
		} else if(scanline_h >= 280 && scanline_h <= 304 && ppumask & 0x18) {
			ppu_address = (ppu_address & 0x41F) | (ppu_address_latch & 0x7BE0);
		}

		ppu_render_fetches();

		if(ppu_odd_frame && scanline_h == 339) {
			++scanline_h;
		}
	}

	if(scanline_h == 340) {
		scanline_h = 0;
		if(scanline_v == 261) {
			scanline_v = 0;
			ppu_odd_frame = !ppu_odd_frame;
			renderFrame = true;
		} else {
			++scanline_v;
		}
	} else {
		++scanline_h;
	}

	return;
}


void nes::ppu_render_fetches()
{ //things done during visible and prerender scanlines
	if(ppumask & 0x18) { //if rendering is enabled
		if(scanline_h == 256) { //Y increment
			if(ppu_address < 0x7000) {
				ppu_address += 0x1000;
			} else {
				ppu_address &= 0xFFF;
				if((ppu_address & 0x3E0) == 0x3A0) {
					ppu_address &= 0xC1F;
					ppu_address ^= 0x800;
				} else if((ppu_address & 0x3E0) == 0x3E0) {
					ppu_address &= 0xC1F;
				} else {
					ppu_address += 0x20;
				}
			}
		}

		if(scanline_h == 257) {
			ppu_address = (ppu_address & 0x7BE0) | (ppu_address_latch & 0x41F);
		}

		if((scanline_h >= 1 && scanline_h <= 257) || (scanline_h >= 321 && scanline_h <= 339)) {
			if(!(scanline_h & 0x07)) { //coarse X increment
				if((ppu_address & 0x1F) != 0x1F) {
					++ppu_address;
				} else {
					ppu_address = (ppu_address & 0x7FE0) ^ 0x400;
				}
			}

			switch(scanline_h & 0x07) {
				case 1:
					if(scanline_h != 1 && scanline_h != 321) {
						ppu_bg_low |= ppu_bg_low_latch;
						ppu_bg_high |= ppu_bg_high_latch;
						ppu_attribute |= (ppu_attribute_latch & 0x03) * 0x5555; //2-bit splat
					}
					ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					break;
				case 3:
					if(scanline_h != 339) {
						ppu_attribute_latch = vram[0x23C0 | (ppu_address & 0xC00) | (ppu_address >> 4 & 0x38) | (ppu_address >> 2 & 0x07)];
						ppu_attribute_latch >>= (((ppu_address >> 1) & 0x01) | ((ppu_address >> 5) & 0x02)) * 2;
					} else {
						ppu_nametable = vram[0x2000 | (ppu_address & 0xFFF)];
					}
					break;
				case 5:
					ppu_bg_address = (ppu_nametable*16 + (ppu_address >> 12)) | ((ppuctrl & 0x10) << 8); //tile X is correct, but Y is always 0.
					ppu_bg_low_latch = vram[ppu_bg_address];
					break;
				case 7:
					ppu_bg_high_latch = vram[ppu_bg_address + 8];
					break;
			}
		}

		if((scanline_h >= 1 && scanline_h <= 256) || (scanline_h >= 321 && scanline_h <= 336)) {
			ppu_bg_low <<= 1;
			ppu_bg_high <<= 1;
			ppu_attribute <<= 2;
		}

	}

	if(scanline_h >= 257 && scanline_h <= 320) { //rendering enabled only?
		oamaddr = 0;
	}

	return;
}
