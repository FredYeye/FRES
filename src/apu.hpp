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

struct Triangle
{
	uint16_t freqTimer;
	uint16_t freqCounter;
	uint8_t linearLoad;
	uint8_t linearCounter;
	uint8_t lengthCounter;
	uint8_t sequencerStep;
	bool enable;
	bool halt; //name? linear control + halt flag
	bool linearReload;

	const std::array<uint8_t, 32> sequencerTable
	{{
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	}};
};

struct Noise
{
	uint16_t lfsr;
	uint16_t freqTimer;
	uint16_t freqCounter;
	uint8_t volume;
	uint8_t envelopeVolume;
	uint8_t envelopeCounter;
	uint8_t lengthCounter;
	bool enable;
	bool halt;
	bool constant;
	bool mode;
	bool envelopeReset;
};

struct Mixer
{
	std::array<float, 31> pulse;
	std::array<float, 203> tnd;
};


class Apu
{
	public:
		Apu();

		void Pulse0Write(uint8_t dataBus, bool channel); //4000 / 4004
		void Pulse1Write(uint8_t dataBus, bool channel); //4001 / 4005
		void Pulse2Write(uint8_t dataBus, bool channel); //4002 / 4006
		void Pulse3Write(uint8_t dataBus, bool channel); //4003 / 4007

		void Triangle0Write(uint8_t dataBus);            //4008
		void Triangle2Write(uint8_t dataBus);            //400A
		void Triangle3Write(uint8_t dataBus);            //400B

		void Noise0Write(uint8_t dataBus);               //400C
		void Noise2Write(uint8_t dataBus);               //400E
		void Noise3Write(uint8_t dataBus);               //400F

		void Dmc0Write(uint8_t dataBus);                 //4010
		void Dmc1Write(uint8_t dataBus);                 //4011
		void Dmc2Write(uint8_t dataBus);                 //4012
		void Dmc3Write(uint8_t dataBus);                 //4013

		void StatusWrite(uint8_t dataBus);               //4015
		uint8_t StatusRead();                            //4015
		void FrameCounterWrite(uint8_t dataBus);         //4017

		void Tick();

		void* GetOutput();
		uint16_t sampleCount = 0;

	private:
		void QuarterFrame();
		void HalfFrame();
		bool SweepForcingSilence(const Pulse &p);

		std::array<Pulse, 2> pulse{};
		Triangle triangle{};
		Noise noise{{1}};
		Mixer mixer;

		const std::array<uint8_t, 32> lengthTable
		{{
			10,254, 20,  2, 40,  4, 80,  6,160,  8, 60, 10, 14, 12, 26, 14,
			12, 16, 24, 18, 48, 20, 96, 22,192, 24, 72, 26, 16, 28, 32, 30
		}};

		uint8_t frameCounter = 0;
		bool blockIRQ = false; //true?
		bool frameIRQ = false;
		uint16_t sequencerCounter = 0;
		bool sequencerMode = false;

		bool oddEvenTick = false; //sufficient for timers that tick every 2 cpu cycles? also rename to something better

		std::array<float, 736*2> apuSamples{};
		uint8_t nearestCounter = 0;

		const std::array<uint8_t, 32> outputCounter //samples at 20 + 9/32 = 20.28125 (20.29220 ideal)
		{{
			20, 20, 21, 20, 20, 20, 21, 20,
			20, 21, 20, 20, 20, 21, 20, 20,
			21, 20, 20, 20, 21, 20, 20, 21,
			20, 20, 20, 21, 20, 20, 21, 20
		}};
		uint8_t outI = 0;
};
