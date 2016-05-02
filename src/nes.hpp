#pragma once

#include <array>
#include <bitset>
#include <vector>

#include "apu.hpp"
#include "ppu.hpp"


class Nes
{
	public:
		Nes(std::string inFile);
		void AdvanceFrame(uint8_t input);

		Ppu ppu;
		Apu apu;

	private:
		void LoadRom(std::string inFile);
		void RunOpcode();
		void CpuRead(uint16_t address);
		void CpuWrite(uint16_t address, uint8_t data);
		void CpuTick();
		void CpuOpDone();
		void PollInterrupts();

		void DebugCpu(uint8_t opcode);

		uint32_t cycleCount = 0;

		uint16_t PC;
		uint8_t rA = 0, rX = 0, rY = 0, rS = 0;
		std::bitset<8> rP; //0:C | 1:Z | 2:I | 3:D | 4:B | 5:1 | 6:V | 7:N

		uint16_t addressBus;
		uint16_t dmaAddress;
		uint8_t dataBus;
		uint8_t controller_reg;

		std::array<uint8_t, 0x800> cpuRam;
		std::vector<uint8_t> prgRom;
		std::vector<uint8_t> chrRom;
		std::array<uint8_t*, 4> bankPtr;

		bool readJoy1 = false;

		bool nmiLine = false;
		bool delayedNmi = false;
		bool nmi = false;
		bool nmiPending = false;

		bool irqLine = false;
		bool irqPending = false;

		bool dmaPending = false;

		uint8_t mapper;
		uint8_t tempData;
};
