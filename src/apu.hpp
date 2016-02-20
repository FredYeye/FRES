#pragma once

#include <cstdint>
#include <array>
#include <vector>


struct Pulse
{
	uint16_t freqTimer;
	uint16_t freqCounter;
	uint8_t duty;
	uint8_t dutyCounter;
	uint8_t volume;
	uint8_t envelopeVolume;
	uint8_t envelopeCounter;
	uint8_t lengthCounter;
	bool enable;
	bool halt; //name?
	bool constant; //rename
	bool envelopeReset;

	uint8_t sweepTimer;
	uint8_t sweepCounter;
	uint8_t sweepShift;
	bool sweepNegate;
	bool sweepReload;
	bool sweepEnabled;
};


class apu
{
	public:
		void Pulse1Write(uint8_t dataBus, bool channel);       //4000
		void Pulse2Write(uint8_t dataBus, bool channel);       //4001
		void Pulse3Write(uint8_t dataBus, bool channel);       //4002
		void Pulse4Write(uint8_t dataBus, bool channel);       //4003

		void Noise1Write(uint8_t dataBus);       //400C
		void Noise2Write(uint8_t dataBus);       //400E
		void Noise3Write(uint8_t dataBus);       //400F

		void StatusWrite(uint8_t dataBus);       //4015
		uint8_t StatusRead();                    //4015
		void FrameCounterWrite(uint8_t dataBus); //4017

		void Tick();

		uint8_t* GetOutput();
		void ClearOutput();
		uint32_t sampleCount = 0;

	private:
		void QuarterFrame();
		void HalfFrame();
		bool SweepForcingSilence(const Pulse &p);

		std::array<Pulse, 2> pulse{{{0}, {0}}};

		bool noiseHalt = 0;
		bool noiseConstant = 0;
		uint8_t noiseDivider = 0;
		bool noiseMode = 0;
		uint8_t noisePeriodIndex = 0;
		const std::array<uint16_t,16> noisePeriod{{4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068}};
		uint8_t noiseLength = 0;
		uint16_t noiseTimer = 0;
		uint16_t noiseShiftReg = 1;
		bool noiseEnable = false;
		bool noiseRestartEnvelope = false;
		const std::array<uint8_t, 32> lengthTable
		{{
			10,254, 20,  2, 40,  4, 80,  6,160,  8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22,192, 24, 72, 26, 16, 28, 32, 30
		}};

		uint8_t status = 0;
		uint8_t frameCounter = 0;
		bool blockIRQ = false; //true?
		bool frameIRQ = false;
		uint16_t sequencerCounter = 0;
		bool sequencerMode = false;

		bool oddEvenTick = false; //sufficient for timers that tick every 2 cpu cycles? also rename to something better

		std::vector<uint8_t> noiseOutput;
		uint8_t nearestCounter = 0;
};
