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
	const std::array<uint8_t, 31> pulse
	{{
		0, 2, 5, 8, 11, 14, 16, 19, 21, 24, 26, 29, 31, 33, 35, 37,
		40, 42, 44, 46, 48, 50, 51, 53, 55, 57, 59, 60, 62, 64, 65
	}};

	const std::array<uint8_t, 203> tnd
	{{
		0, 1, 3, 5, 6, 8, 10, 11, 13, 14, 16, 18, 19, 21, 22, 24, 25, 27, 28, 30, 31, 33, 34, 36, 37,
		38, 40, 41, 43, 44, 45, 47, 48, 49, 51, 52, 53, 55, 56, 57, 58, 60, 61, 62, 63, 65, 66, 67, 68,
		69, 71, 72, 73, 74, 75, 76, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 109, 110, 111, 112, 113, 114,
		115, 116, 117, 118, 118, 119, 120, 121, 122, 123, 124, 124, 125, 126, 127, 128, 129, 129, 130,
		131, 132, 133, 133, 134, 135, 136, 137, 137, 138, 139, 140, 140, 141, 142, 143, 143, 144, 145,
		146, 146, 147, 148, 148, 149, 150, 151, 151, 152, 153, 153, 154, 155, 155, 156, 157, 157, 158,
		159, 159, 160, 161, 161, 162, 163, 163, 164, 164, 165, 166, 166, 167, 168, 168, 169, 169, 170,
		171, 171, 172, 172, 173, 174, 174, 175, 175, 176, 176, 177, 178, 178, 179, 179, 180, 180, 181,
		181, 182, 183, 183, 184, 184, 185, 185, 186, 186, 187, 187, 188, 188, 189
	}};
};


class apu
{
	public:
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

		void StatusWrite(uint8_t dataBus);               //4015
		uint8_t StatusRead();                            //4015
		void FrameCounterWrite(uint8_t dataBus);         //4017

		void Tick();

		uint8_t* GetOutput();
		void ClearOutput();
		uint32_t sampleCount = 0;

	private:
		void QuarterFrame();
		void HalfFrame();
		bool SweepForcingSilence(const Pulse &p);

		std::array<Pulse, 2> pulse{{{0}, {0}}};
		Triangle triangle{{0}};
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

		std::vector<uint8_t> apuSamples;
		uint8_t nearestCounter = 0;
};
