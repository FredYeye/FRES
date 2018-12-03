// #define DEBUG
// #define DUMP_VRAM

#include <fstream>
#include <iostream>
#include <iomanip>

#include "nes.hpp"
#include "apu.hpp"
#include "ppu.hpp"


Nes::Nes(std::string inFile)
{
	Cart cart(inFile, prgRom, prgRam);
	type = cart.type;
	pPrgBank = cart.pPrgBank;
	pPrgRamBank = cart.pPrgRamBank;
	ppu.SetPattern(cart.chrMem);
	ppu.SetNametableArrangement(cart.nametableOffsets);
	ppu.SetChrType(cart.chrType);

	Reset();
}


void Nes::AdvanceFrame(uint8_t input, uint8_t input2)
{
	while(!ppu.renderFrame)
	{
		RunOpcode();

		if(readJoy1)
		{
			controller_reg = input;
			controller_reg2 = input2;
		}
	}
	ppu.renderFrame = false;

	if(input2 & 0b10) //H key, temp
	{
		Reset();
	}
}


const NesInfo Nes::GetInfo() const
{
	return {rA, rX, rY, rS};
}


void Nes::Reset()
{
	apu.Reset();

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


void Nes::RunOpcode()
{
	const uint8_t opcode = dataBus;


	#ifdef DEBUG
		DebugCpu(opcode);
	#endif
	#ifdef DUMP_VRAM
		if(ppu.GetScanlineV() == 261 && ppu.GetScanlineH() == 0)
		{
			// std::string outfile = "vram.txt";
			// std::ofstream result(outfile.c_str(), std::ios::out | std::ios::binary);
			// result.write((char*)&ppu.GetVram()[0],0x4000);
			// result.close();
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
		{
			const uint16_t interruptVector = 0xFFFE ^ (nmiPending[1] << 2); //possible NMI hijack
			nmiPending[0] &= !nmiPending[1]; //haven't tested, but should be correct
			CpuRead(interruptVector);
			tempData = dataBus;
			rP.set(2);
			CpuRead(interruptVector + 1);
		}
			nmiPending[1] = false; //NMI delayed until after next instruction
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

		case 0x10: Branch(!rP[7], op1); break; //BPL
		case 0x30: Branch( rP[7], op1); break; //BMI
		case 0x50: Branch(!rP[6], op1); break; //BVC
		case 0x70: Branch( rP[6], op1); break; //BVS
		case 0x90: Branch(!rP[0], op1); break; //BCC
		case 0xB0: Branch( rP[0], op1); break; //BCS
		case 0xD0: Branch(!rP[1], op1); break; //BNE
		case 0xF0: Branch( rP[1], op1); break; //BEQ

		case 0x18: rP.reset(0); break; //CLC
		case 0x38: rP.set(0);   break; //SEC
		case 0x58: rP.reset(2); break; //CLI
		case 0x78: rP.set(2);   break; //SEI
		case 0xB8: rP.reset(6); break; //CLV
		case 0xD8: rP.reset(3); break; //CLD
		case 0xF8: rP.set(3);   break; //SED

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
		case 0xE3: case 0xE7: case 0xEF: case 0xF3: case 0xF7: case 0xFB: case 0xFF: //isc
			++dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

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

		case 0x23: case 0x27: case 0x2F: case 0x33: case 0x37: case 0x3B: case 0x3F: //rla
			{
			const bool newCarry = dataBus & 0x80;
			dataBus = (dataBus << 1) | rP[0];
			rP[0] = newCarry;
			}
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

		case 0x21: case 0x25: case 0x29: case 0x2D: //and
		case 0x31: case 0x35: case 0x39: case 0x3D:
			rA &= dataBus;
			rP[1] = !rA;
			rP[7] = rA & 0x80;
		break;

		case 0xC3: case 0xC7: case 0xCF: case 0xD3: case 0xD7: case 0xDB: case 0xDF: //dcp
			--dataBus;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

		case 0xC1: case 0xC5: case 0xC9: case 0xCD: //cmp
		case 0xD1: case 0xD5: case 0xD9: case 0xDD:
			rP[0] = !(rA - dataBus & 0x0100);
			rP[1] = !(rA - dataBus);
			rP[7] = rA - dataBus & 0x80;
		break;

		case 0x43: case 0x47: case 0x4F: case 0x53: case 0x57: case 0x5B: case 0x5F: //sre
			rP[0] = dataBus & 0x01;
			dataBus >>= 1;
			rP[1] = !dataBus;
			rP.reset(7);
			CpuWrite(addressBus, dataBus);

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

		case 0x03: case 0x07: case 0x0F: case 0x13: case 0x17: case 0x1B: case 0x1F: //slo
			rP[0] = dataBus & 0x80;
			dataBus <<= 1;
			rP[1] = !dataBus;
			rP[7] = dataBus & 0x80;
			CpuWrite(addressBus, dataBus);

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

		case 0xA3: case 0xA7: case 0xAF: case 0xB3: case 0xB7: case 0xAB: case 0xBF: //lax
			rA = dataBus;

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

	irqPending[2] |= irqPending[1]; //interrupt polling
	nmiPending[2] |= nmiPending[1]; //

	CpuRead(PC); //fetch next opcode
	CpuOpDone();
}


void Nes::Branch(const bool flag, const uint8_t op1)
{
	++PC;
	if(flag)
	{
		irqPending[2] |= irqPending[1]; //branches check irq on the first cycle
		nmiPending[2] |= nmiPending[1];

		const uint16_t pagePC = PC + int8_t(op1);
		PC = (PC & 0xFF00) | (pagePC & 0x00FF);
		CpuRead(PC);

		irqPending[1] = false; //taken branch without page crossing doesn't check for irqs on second cycle
		nmiPending[1] = false;

		if(PC != pagePC)
		{
			PC = pagePC;
			CpuRead(PC);
		}
	}
}


void Nes::CpuRead(const uint16_t address)
{
	rw = 1;
	addressBus = address;

	switch(addressBus >> 13)
	{
		case 0x0000 >> 13: dataBus = cpuRam[addressBus & 0x07FF]; break;

		case 0x2000 >> 13:
			switch(addressBus & 7)
			{
				case 2: dataBus = ppu.StatusRead();  break;
				case 4: dataBus = ppu.OamDataRead(); break;
				case 7: dataBus = ppu.DataRead();    break;
			}
		break;

		case 0x4000 >> 13:
			switch(addressBus)
			{
				case 0x4014: dataBus = 0x40;             break; //open bus, maybe hax something better later
				case 0x4015: dataBus = apu.StatusRead(); break;

				case 0x4016:
					dataBus = ((addressBus >> 8) & 0xE0) | (controller_reg & 1); //addressBus = open bus?
					controller_reg >>= 1; //todo: if we have read 4016 8 times, start returning 1s
				break;

				case 0x4017:
					dataBus = ((addressBus >> 8) & 0xE0) | (controller_reg2 & 1);
					controller_reg2 >>= 1;
				break;
			}
		break;

		case 0x6000 >> 13:
			if(prgRam.size()) //todo: maybe & with some generic cart.ramEnabled
			{
				//todo: vrc4 with 2kb wram should return open bus if addressBus >= 0x7000
				dataBus = pPrgRamBank[(addressBus >> 11) & 0b11][addressBus & 0x07FF];
			}
			else
			{
				dataBus = addressBus >> 8;
			}
		break;

		case 0x8000 >> 13: dataBus = pPrgBank[0][addressBus & 0x1FFF]; break;
		case 0xA000 >> 13: dataBus = pPrgBank[1][addressBus & 0x1FFF]; break;
		case 0xC000 >> 13: dataBus = pPrgBank[2][addressBus & 0x1FFF]; break;
		case 0xE000 >> 13: dataBus = pPrgBank[3][addressBus & 0x1FFF]; break;
	}

	CpuTick();

	if(apu.dmcDma && !dmcDmaActive) //check for dmc dma after read is finished (writes block this)
	{
		dmcDmaActive = true;

		const uint16_t tempAddr = addressBus;

		if(!dmaPending)
		{
			if(rw == 1)
			{
				CpuRead(addressBus);
			}
			CpuRead(addressBus);
		}

		CpuRead(apu.GetDmcAddr()); //dma fetch
		apu.DmcDma(dataBus);
		CpuRead(tempAddr); //resume

		dmcDmaActive = false;
	}
}


void Nes::CpuWrite(const uint16_t address, const uint8_t data)
{
	rw = 0;
	addressBus = address;
	dataBus = data;

	switch(addressBus >> 13)
	{
		case 0x0000 >> 13: cpuRam[addressBus & 0x07FF] = dataBus; break;

		case 0x2000 >> 13:
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

		case 0x4000 >> 13:
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

		case 0x6000 >> 13:
			if(prgRam.size()) //todo: maybe & with some generic cart.ramEnabled
			{
				//todo: writes to vrc4 with 2kb wram should do nothing if addressBus >= 0x7000
				pPrgRamBank[(addressBus >> 11) & 0b11][addressBus & 0x07FF] = dataBus;
			}
		break;

		case 0x8000 >> 13: case 0xA000 >> 13: case 0xC000 >> 13: case 0xE000 >> 13:
			Addons();
		break;
	}

	CpuTick();
}


void Nes::CpuTick()
{
	ppu.Tick();
	PollInterrupts();
	ppu.Tick();
	ppu.Tick();
	apu.Tick();
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

		for(int x = 0; x < 256; ++x)
		{
			CpuRead(dmaAddress + x); //should dmaAddress be 1st or 2nd write from R&W ops (2nd currently)?
			CpuWrite(0x2004, dataBus);
		}

		CpuRead(PC);
		dmaPending = false;
	}

	if(nmiPending[2] | irqPending[2])
	{
		CpuRead(addressBus);                                //fetch op1, increment suppressed
		CpuWrite(0x100 | rS--, PC >> 8);                    //push PC high on stack
		CpuWrite(0x100 | rS--, PC);                         //push PC low on stack
		CpuWrite(0x100 | rS--, rP.to_ulong() & 0b11101111); //push flags on stack with B clear

		#ifdef DEBUG
			if(nmiPending[1]) std::cout << "NMI ";
			else              std::cout << "IRQ ";
		#endif

		//determine if this is an IRQ or NMI, also see if NMI will hijack an IRQ (0xFFFE -> 0xFFFA)
		const uint16_t interruptVector = 0xFFFE ^ (nmiPending[1] << 2);
		nmiPending[0] &= !nmiPending[1];                    //toggle nmi[0] since it gets stuck on

		CpuRead(interruptVector);                           //read vector low, set I flag
		tempData = dataBus;                                 //
		rP.set(2);                                          //

		irqPending[2] = false;                              //clear interrupts
		nmiPending[2] = false;                              //

		CpuRead(interruptVector + 1);                       //read vector high
		PC = tempData | (dataBus << 8);                     //fetch next opcode
		CpuRead(PC);                                        //
	}
}


void Nes::PollInterrupts()
{
	nmiPending[1] = nmiPending[0]; //first cycle after nmiPending set, polling will see now
	const bool oldNmi = nmi;
	nmi = ppu.PollNmi();
	nmiPending[0] |= !oldNmi & nmi; //nmiPending[0] gets set = nmi detected, but interrupt polling will miss

	irqPending[1] = irqPending[0]; //same as nmi
	irqPending[0] = !rP.test(2) & (apu.PollFrameInterrupt() | VRC4Interrupt() | MMC3Interrupt()); //OR with some general cartIRQ later
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

	std::cout << std::uppercase << std::hex << std::setfill('0')
			  << std::setw(4) << PC
			  << "  " << std::setw(2) << +opcode
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
	uint8_t a = 0;
	switch(address & 0xE000)
	{
		case 0x0000: case 0x2000: case 0x4000: case 0x6000:
				std::cout << "running code below 0x8000, at 0x"
				<< std::uppercase << std::hex << std::setfill('0') << std::setw(4) << address << "\n";
		break;

		case 0x8000: a = pPrgBank[0][address & 0x1FFF]; break;
		case 0xA000: a = pPrgBank[1][address & 0x1FFF]; break;
		case 0xC000: a = pPrgBank[2][address & 0x1FFF]; break;
		case 0xE000: a = pPrgBank[3][address & 0x1FFF]; break;
	}

	return a;
}


//put this elsewhere later
void Nes::Addons()
{
	switch(type)
	{
		case UNROM:
			pPrgBank[0] = &prgRom[(dataBus & 0b1111) * 0x4000];
			pPrgBank[1] = pPrgBank[0] + 0x2000;
		break;

		case CNROM:
			ppu.SetPatternBanks8(dataBus & 0b0011);
		break;

		case AOROM:
			pPrgBank[0] = &prgRom[(dataBus & 0b0111) * 0x8000];
			pPrgBank[1] = pPrgBank[0] + 0x2000;
			pPrgBank[2] = pPrgBank[0] + 0x4000;
			pPrgBank[3] = pPrgBank[0] + 0x6000;

			if(dataBus & 0b00010000)
			{
				ppu.SetNametableArrangement({B,B,B,B});
			}
			else
			{
				ppu.SetNametableArrangement({A,A,A,A});
			}
		break;

		case SFROM: MMC1Registers(); break;
		case TLROM: MMC3Registers(); break;
		case VRC_4: VRC4Registers(); break;
	}
}


void Nes::MMC1Registers()
{
	if(mmc1.lastWrittenTo != cycleCount - 1) //compare with previous write to disallow consecutive writes
	{
		if(dataBus & 0x80)
		{
			mmc1.shiftReg = 0b100000;
			mmc1.prgMode = 0b11;
			pPrgBank[0] = prgRom.data() + (mmc1.prg << 14);
			pPrgBank[1] = pPrgBank[0] + 8 * 1024;
			pPrgBank[2] = prgRom.data() + prgRom.size() - 16 * 1024;
			pPrgBank[3] = pPrgBank[2] + 8 * 1024;
		}
		else
		{
			mmc1.shiftReg |= (dataBus & 1) << 6;
			mmc1.shiftReg >>= 1;
			if(mmc1.shiftReg & 1)
			{
				mmc1.shiftReg >>= 1;
				switch(addressBus >> 13)
				{
					case 0x8000 >> 13:
						switch(mmc1.shiftReg & 0b11)
						{
							case 0: ppu.SetNametableArrangement({A,A,A,A}); break;
							case 1: ppu.SetNametableArrangement({B,B,B,B}); break;
							case 2: ppu.SetNametableArrangement({A,B,A,B}); break;
							case 3: ppu.SetNametableArrangement({A,A,B,B}); break;
						}

						mmc1.prgMode = (mmc1.shiftReg >> 2) & 0b11;
						switch(mmc1.prgMode)
						{
							case 0: case 1: //32k mode
								pPrgBank[0] = prgRom.data() + ((mmc1.prg & 0b11110) << 14);
								pPrgBank[2] = pPrgBank[0] + 16 * 1024;
							break;
							case 2: //low bank fixed, high switchable
								pPrgBank[0] = prgRom.data();
								pPrgBank[2] = prgRom.data() + (mmc1.prg << 14);
							break;
							case 3: //high bank fixed, low switchable
								pPrgBank[0] = prgRom.data() + (mmc1.prg << 14);
								pPrgBank[2] = prgRom.data() + prgRom.size() - 16 * 1024;
							break;
						}
						pPrgBank[1] = pPrgBank[0] + 8 * 1024;
						pPrgBank[3] = pPrgBank[2] + 8 * 1024;

						mmc1.chrMode = mmc1.shiftReg >> 4;
						if(mmc1.chrMode == 1)
						{
							ppu.SetPatternBanks4(0, mmc1.chr0);
							ppu.SetPatternBanks4(1, mmc1.chr1);
						}
						else
						{
							ppu.SetPatternBanks8(mmc1.chr0 >> 1);
						}
					break;

					case 0xA000 >> 13:
						mmc1.chr0 = mmc1.shiftReg;
						if(mmc1.chrMode == 1)
						{
							ppu.SetPatternBanks4(0, mmc1.chr0);
						}
						else
						{
							ppu.SetPatternBanks8(mmc1.chr0 >> 1);
						}
					break;

					case 0xC000 >> 13:
						mmc1.chr1 = mmc1.shiftReg;
						if(mmc1.chrMode == 1)
						{
							ppu.SetPatternBanks4(1, mmc1.chr1);
						}
					break;

					case 0xE000 >> 13:
						mmc1.prg = mmc1.shiftReg & 0b01111;
						switch(mmc1.prgMode)
						{
							case 0: case 1: //32k mode
								pPrgBank[0] = prgRom.data() + ((mmc1.prg & 0b11110) << 14);
								pPrgBank[2] = pPrgBank[0] + 16 * 1024;
							break;
							case 2: //low bank fixed, high switchable
								pPrgBank[0] = prgRom.data();
								pPrgBank[2] = prgRom.data() + (mmc1.prg << 14);
							break;
							case 3: //high bank fixed, low switchable
								pPrgBank[0] = prgRom.data() + (mmc1.prg << 14);
								pPrgBank[2] = prgRom.data() + prgRom.size() - 16 * 1024;
							break;
						}
						pPrgBank[1] = pPrgBank[0] + 8 * 1024;
						pPrgBank[3] = pPrgBank[2] + 8 * 1024;
						mmc1.wramEnable = mmc1.shiftReg & 0b10000;
					break;
				}
				mmc1.shiftReg = 0b100000;
			}
		}
	}
	mmc1.lastWrittenTo = cycleCount;
}


void Nes::MMC3Registers()
{
	switch(((addressBus >> 12) & 0b0110) | (addressBus & 1))
	{
		case 0: //8000
			mmc3.bankRegSelect = dataBus & 0b0111;
			mmc3.prgMode = dataBus & 0b01000000;
			mmc3.chrMode = dataBus & 0b10000000;

			pPrgBank[mmc3.prgMode << 1] = &prgRom[mmc3.bankReg[6] << 13];
			pPrgBank[!mmc3.prgMode << 1] = &prgRom[prgRom.size() - 16 * 1024];

			ppu.SetPatternBanks2((mmc3.chrMode << 1)    , mmc3.bankReg[0]);
			ppu.SetPatternBanks2((mmc3.chrMode << 1) | 1, mmc3.bankReg[1]);

			ppu.SetPatternBanks1((!mmc3.chrMode << 2)    , mmc3.bankReg[2]);
			ppu.SetPatternBanks1((!mmc3.chrMode << 2) | 1, mmc3.bankReg[3]);
			ppu.SetPatternBanks1((!mmc3.chrMode << 2) | 2, mmc3.bankReg[4]);
			ppu.SetPatternBanks1((!mmc3.chrMode << 2) | 3, mmc3.bankReg[5]);

		break;

		case 1: //8001
			mmc3.bankReg[mmc3.bankRegSelect] = dataBus;

			switch(mmc3.bankRegSelect)
			{
				case 0: case 1:
					mmc3.bankReg[mmc3.bankRegSelect] >>= 1;
					ppu.SetPatternBanks2((mmc3.chrMode << 1) | mmc3.bankRegSelect, mmc3.bankReg[mmc3.bankRegSelect]);
				break;

				case 2: case 3: case 4: case 5:
					ppu.SetPatternBanks1((!mmc3.chrMode << 2) | (mmc3.bankRegSelect - 2), mmc3.bankReg[mmc3.bankRegSelect]);
				break;

				case 6:
					mmc3.bankReg[6] &= 0b00111111;
					pPrgBank[mmc3.prgMode << 1] = &prgRom[mmc3.bankReg[6] << 13];
				break;

				case 7:
					mmc3.bankReg[7] &= 0b00111111;
					pPrgBank[1] = &prgRom[mmc3.bankReg[7] << 13];
				break;
			}
		break;

		case 2: //A000
			if(dataBus & 1)
			{
				ppu.SetNametableArrangement({A, A, B, B});
			}
			else
			{
				ppu.SetNametableArrangement({A, B, A, B});
			}
		break;

		case 3: //A001
		break;

		case 4: //C000
			mmc3.irqLatch = dataBus;
		break;

		case 5: //C001
			mmc3.irqReload = true;
			//also set counter to 0?
		break;

		case 6: //E000
			mmc3.irqEnable = false;
			mmc3.irqPending = false;
		break;

		case 7: //E001
			mmc3.irqEnable = true;
		break;
	}
}


bool Nes::MMC3Interrupt()
{
	//todo: investigate revision differences

	mmc3.A12[2] = mmc3.A12[1];
	mmc3.A12[1] = mmc3.A12[0];
	mmc3.A12[0] = ppu.GetA12();

	if(mmc3.A12[0] && !(mmc3.A12[1] | mmc3.A12[2])) //clock irq via A12 0 -> 0 -> 1 change
	{
		if(mmc3.irqReload || mmc3.irqCounter == 0)
		{
			mmc3.irqCounter = mmc3.irqLatch;
			mmc3.irqReload = false;
		}
		else
		{
			--mmc3.irqCounter;
		}

		if(mmc3.irqCounter == 0)
		{
			mmc3.irqPending |= mmc3.irqEnable;
		}
	}

	return mmc3.irqPending;
}


void Nes::VRC4Registers()
{
	const uint16_t vrc4Address = (addressBus & 0xFF00) | ((addressBus & 0xFF) >> 2);
	switch(vrc4Address)
	{
		case 0x8000: case 0x8001: case 0x8002: case 0x8003:
			pPrgBank[vrc4.prgMode] = prgRom.data() + ((dataBus & 0b00011111) << 13);
		break;
		case 0x9000: case 0x9001:
			switch(dataBus & 0b11)
			{
				case 0: ppu.SetNametableArrangement({A,B,A,B}); break;
				case 1: ppu.SetNametableArrangement({A,A,B,B}); break;
				case 2: ppu.SetNametableArrangement({A,A,A,A}); break;
				case 3: ppu.SetNametableArrangement({B,B,B,B}); break;
			}
		break;
		case 0x9002: case 0x9003:
			if(vrc4.prgMode != dataBus & 0b10)
			{
				uint8_t *tempBank = pPrgBank[0];
				pPrgBank[0] = pPrgBank[2];
				pPrgBank[2] = tempBank;
			}
			vrc4.prgMode = dataBus & 0b10;
		break;
		case 0xA000: case 0xA001: case 0xA002: case 0xA003:
			pPrgBank[1] = prgRom.data() + ((dataBus & 0b00011111) << 13);
		break;

		case 0xB000: case 0xB002: case 0xC000: case 0xC002: case 0xD000: case 0xD002: case 0xE000: case 0xE002:
		{
			const uint8_t regSelect = ((vrc4Address >> 11) - 0x16) | ((vrc4Address >> 1) & 1);
			vrc4.chrSelect[regSelect] = (vrc4.chrSelect[regSelect] & 0x01F0) | (dataBus & 0b1111);
			ppu.SetPatternBanks1(regSelect, vrc4.chrSelect[regSelect]);
		}
		break;
		case 0xB001: case 0xB003: case 0xC001: case 0xC003: case 0xD001: case 0xD003: case 0xE001: case 0xE003:
		{
			const uint8_t regSelect = ((vrc4Address >> 11) - 0x16) | ((vrc4Address >> 1) & 1);
			vrc4.chrSelect[regSelect] = (vrc4.chrSelect[regSelect] & 0x0F) | ((dataBus & 0b00011111) << 4);
			ppu.SetPatternBanks1(regSelect, vrc4.chrSelect[regSelect]);
		}
		break;

		case 0xF000:
			vrc4.irqLatch &= 0b11110000;
			vrc4.irqLatch |= dataBus & 0b1111;
		break;
		case 0xF001:
			vrc4.irqLatch &= 0b1111;
			vrc4.irqLatch |= dataBus << 4;
		break;
		case 0xF002:
			vrc4.irqPending = false;
			vrc4.irqAckEnable = dataBus & 1;
			vrc4.irqEnable = dataBus & 0b10;
			vrc4.irqMode = dataBus & 0b0100;
			if(vrc4.irqEnable)
			{
				vrc4.irqCounter = vrc4.irqLatch;
				vrc4.prescalerCounter2 = 341;
			}
		break;
		case 0xF003:
			vrc4.irqPending = false;
			vrc4.irqEnable = vrc4.irqAckEnable;
		break;
	}
}


bool Nes::VRC4Interrupt()
{
	if(vrc4.irqEnable)
	{
		if(vrc4.irqCounter == 0xFF)
		{
			vrc4.irqPending = true;
			vrc4.irqCounter = vrc4.irqLatch;
		}
		else if(vrc4.irqMode == true)
		{
			++vrc4.irqCounter;
		}
		else
		{
			vrc4.prescalerCounter2 -= 3;
			if(vrc4.prescalerCounter2 <= 0)
			{
				++vrc4.irqCounter;
				vrc4.prescalerCounter2 += 341;
			}
		}
	}
	return vrc4.irqPending;
}
