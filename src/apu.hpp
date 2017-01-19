#pragma once

#include <cstdint>
#include <array>
#include <vector>


struct Pulse
{
	uint16_t freqTimer, freqCounter;
	uint8_t duty, dutyCounter;
	uint8_t volume, envelopeVolume;
	uint8_t envelopeCounter, lengthCounter;
	uint8_t sweepTimer, sweepCounter, sweepShift;
	bool enable, halt, constant, envelopeReset;
	bool sweepNegate, sweepReload, sweepEnabled;
};

struct Triangle
{
	uint16_t freqTimer, freqCounter;
	uint8_t linearLoad, linearCounter;
	uint8_t lengthCounter;
	uint8_t sequencerStep;
	bool enable, halt, linearReload;
};

struct Noise
{
	uint16_t lfsr;
	uint16_t freqTimer, freqCounter;
	uint8_t volume, envelopeVolume;
	uint8_t envelopeCounter, lengthCounter;
	bool enable, halt, constant, mode, envelopeReset;
};

struct Dmc
{
	uint16_t freqTimer, freqCounter;
	uint16_t address, addressLoad, samplesRemaining, sampleLength;
	uint8_t output, outputShift, bitsRemaining, sampleBuffer;
	bool enableIrq, irqPending, loop, enable, silence, sampleBufferEmpty;
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

		const uint16_t GetDmcAddr() const;
		void DmcDma(uint8_t sample);

		void Tick();

		const void* const GetOutput() const;
		uint16_t sampleCount = 0;

		bool PollFrameInterrupt() const;
		bool dmcDma = false;

	private:
		void QuarterFrame();
		void HalfFrame();
		bool SweepForcingSilence(const Pulse &p) const;

		std::array<Pulse, 2> pulse{};
		Triangle triangle{};
		Noise noise{1};
		Dmc dmc{};
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
		uint8_t sequencerResetDelay = 0;

		bool apuTick = false;

		//wood & water rage produces 741-744 samples on intro->menu transition, so add some extra for now
		//is this supposed to happen?
		//this should be because a dma or something similar happens when the emulator wants to render
		std::array<float, 752*2> apuSamples{};
		uint8_t nearestCounter = 0;
		uint8_t outI = 0;

		const std::array<uint8_t, 32> outputCounter //samples at 20 + 9/32 = 20.28125 (20.29220 ideal)
		{{
			20, 20, 21, 20, 20, 20, 21, 20,
			20, 21, 20, 20, 20, 21, 20, 20,
			21, 20, 20, 20, 21, 20, 20, 21,
			20, 20, 20, 21, 20, 20, 21, 20
		}};
};
