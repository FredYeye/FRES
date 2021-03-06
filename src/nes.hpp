#pragma once

#include <array>
#include <bitset>
#include <vector>

#include "apu.hpp"
#include "ppu.hpp"
#include "cart.hpp"

struct NesInfo
{
    uint8_t rA, rX, rY, rS;
};

struct VRC4
{
	std::array<uint16_t, 8> chrSelect{};
	int16_t prescalerCounter2 = 341;
	uint8_t prgMode = 0;
	uint8_t irqLatch = 0;
	uint8_t irqCounter = 0;
	bool irqPending = false;
	bool irqEnable = false;
	bool irqAckEnable = false;
	bool irqMode = false;
};

struct MMC1
{
	uint32_t lastWrittenTo = 0;
	uint8_t shiftReg = 0b100000;
	uint8_t prgMode = 0b11;
	uint8_t prg = 0;
	bool wramEnable = 0;
	bool chrMode = 0;
	uint8_t chr0 = 0;
	uint8_t chr1 = 0;
};

struct MMC3
{
	std::array<uint8_t, 8> bankReg{};
	uint8_t bankRegSelect, irqLatch = 0, irqCounter = 0;
	bool prgMode, chrMode, irqEnable = 0, irqPending = 0, irqReload = 0;
	std::array<bool, 3> A12{};
};

class Nes
{
	public:
		Nes(std::string inFile);
		void AdvanceFrame(uint8_t input, uint8_t input2);

		const NesInfo GetInfo() const;

		Ppu ppu;
		Apu apu;

	private:
		void Reset();

		void RunOpcode();
		void Branch(const bool flag, const uint8_t op1);
		void CpuRead(const uint16_t address);
		void CpuWrite(const uint16_t address, const uint8_t data);
		void CpuTick();
		void CpuOpDone();
		void PollInterrupts();

		void DebugCpu(uint8_t opcode);
		uint8_t DebugRead(uint16_t address);

		void Addons();
		void MMC1Registers();
		void MMC3Registers();
		bool MMC3Interrupt();
		void VRC4Registers();
		bool VRC4Interrupt();

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
		std::array<uint8_t*, 4> pPrgBank;

		std::vector<uint8_t> prgRam;
		std::array<uint8_t*, 4> pPrgRamBank;

		bool readJoy1 = false;

		bool nmi = false;
		std::array<bool, 3> nmiPending{};
		std::array<bool, 3> irqPending{};

		bool dmaPending = false;
		bool dmcDmaActive = false;
		bool rw;

		Type type;
		uint8_t tempData;

		VRC4 vrc4;
		MMC1 mmc1;
		MMC3 mmc3;
};
