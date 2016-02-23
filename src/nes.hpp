#pragma once

#include <array>
#include <bitset>

#include "apu.hpp"
#include "ppu.hpp"


class nes
{
	public:
		nes(std::string inFile);
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

		void DebugCpu();

		std::array<uint8_t, 0x10000> cpuMem;

		uint16_t PC;
		uint8_t rA = 0, rX = 0, rY = 0, rS = 0;
		std::bitset<8> rP; //0:C | 1:Z | 2:I | 3:D | 4:B | 5:1 | 6:V | 7:N

		uint16_t addressBus;
		uint8_t dataBus;
		uint8_t controller_reg;
		bool readJoy1 = false;

		bool nmiLine = false;
};
