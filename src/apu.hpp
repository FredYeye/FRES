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
		0, 3, 6, 9, 11, 14, 17, 19, 22, 24, 27, 29, 31, 34, 36, 38,
		40, 42, 44, 46, 48, 50, 52, 54, 56, 57, 59, 61, 62, 64, 66
	}};

	const std::array<uint8_t, 203> tnd
	{{
		0, 2, 3, 5, 7, 8, 10, 12, 13, 15, 16, 18, 20, 21, 23, 24, 26, 27, 29, 30, 32, 33, 35, 36, 37,
		39, 40, 42, 43, 44, 46, 47, 49, 50, 51, 52, 54, 55, 56, 58, 59, 60, 61, 63, 64, 65, 66, 68, 69,
		70, 71, 72, 73, 75, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
		115, 116, 117, 118, 119, 120, 121, 122, 122, 123, 124, 125, 126, 127, 127, 128, 129, 130, 131,
		132, 132, 133, 134, 135, 136, 136, 137, 138, 139, 139, 140, 141, 142, 142, 143, 144, 145, 145,
		146, 147, 148, 148, 149, 150, 150, 151, 152, 152, 153, 154, 155, 155, 156, 157, 157, 158, 159,
		159, 160, 160, 161, 162, 162, 163, 164, 164, 165, 166, 166, 167, 167, 168, 169, 169, 170, 170,
		171, 172, 172, 173, 173, 174, 175, 175, 176, 176, 177, 177, 178, 179, 179, 180, 180, 181, 181,
		182, 182, 183, 184, 184, 185, 185, 186, 186, 187, 187, 188, 188, 189, 189
	}};
};


class Apu
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
		// void ClearOutput();
		uint16_t sampleCount = 0;

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

		std::array<uint8_t, 746> apuSamples;
		uint8_t nearestCounter = 0;
};
