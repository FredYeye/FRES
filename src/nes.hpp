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
		void AdvanceFrame(uint8_t input, uint8_t input2);

		Ppu ppu;
		Apu apu;

	private:
		void LoadRom(std::string inFile);
		void RunOpcode();
		void CpuRead(const uint16_t address);
		void CpuWrite(const uint16_t address, const uint8_t data);
		void CpuTick();
		void CpuOpDone();
		void PollInterrupts();
		void Branch(const bool flag, const uint8_t op1);

		void DebugCpu(uint8_t opcode);
		uint8_t DebugRead(uint16_t address);

		uint32_t cycleCount = 0;

		uint16_t PC;
		uint8_t rA = 0, rX = 0, rY = 0, rS = 0;
		std::bitset<8> rP; //0:C | 1:Z | 2:I | 3:D | 4:B | 5:1 | 6:V | 7:N

		uint16_t addressBus;
		uint16_t dmaAddress;
		uint8_t dataBus;
		uint8_t controller_reg, controller_reg2;

		std::array<uint8_t, 0x800> cpuRam;
		std::vector<uint8_t> prgRom;
		std::vector<uint8_t> chrRom;
		std::array<uint8_t*, 4> bankPtr;

		bool readJoy1 = false;

		bool nmi = false;
		std::array<bool, 3> nmiPending{};

		bool irqLine = false;
		bool irqPending = false;

		bool dmaPending = false;
		bool dmcDmaActive = false;

		uint8_t mapper;
		uint8_t tempData;
};
