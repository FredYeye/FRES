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
	const std::array<float, 31> pulse
	{{
		0.0f, 0.0116091f, 0.0229395f, 0.0340009f, 0.044803f, 0.0553547f, 0.0656645f, 0.0757408f,
		0.0855914f, 0.0952237f, 0.104645f, 0.113862f, 0.122882f, 0.13171f, 0.140353f, 0.148816f,
		0.157105f, 0.165226f, 0.173183f, 0.180981f, 0.188626f, 0.19612f, 0.20347f, 0.210679f,
		0.217751f, 0.224689f, 0.231499f, 0.238182f, 0.244744f, 0.251186f, 0.257513f
	}};

	const std::array<float, 203> tnd
	{{
		0.0, 0.00669982, 0.013345, 0.0199363, 0.0264742, 0.0329594, 0.0393927, 0.0457745, 0.0521055, 0.0583864, 0.0646176, 0.0707999, 0.0769337, 0.0830196, 0.0890583, 0.0950501, 0.100996, 0.106896, 0.112751, 0.118561, 0.124327, 0.130049, 0.135728, 0.141365, 0.146959, 0.152512, 0.158024, 0.163494, 0.168925, 0.174315, 0.179666, 0.184978, 0.190252, 0.195487, 0.200684, 0.205845, 0.210968, 0.216054, 0.221105, 0.22612, 0.231099, 0.236043, 0.240953, 0.245828, 0.250669, 0.255477, 0.260252, 0.264993, 0.269702, 0.274379, 0.279024, 0.283638, 0.28822, 0.292771, 0.297292, 0.301782, 0.306242, 0.310673, 0.315074, 0.319446, 0.323789, 0.328104, 0.33239, 0.336649, 0.340879, 0.345083, 0.349259, 0.353408, 0.35753, 0.361626, 0.365696, 0.36974, 0.373759, 0.377752, 0.38172, 0.385662, 0.389581, 0.393474, 0.397344, 0.401189, 0.405011, 0.408809, 0.412584, 0.416335, 0.420064, 0.42377, 0.427454, 0.431115, 0.434754, 0.438371, 0.441966, 0.44554, 0.449093, 0.452625, 0.456135, 0.459625, 0.463094, 0.466543, 0.469972, 0.47338, 0.476769, 0.480138, 0.483488, 0.486818, 0.490129, 0.493421, 0.496694, 0.499948, 0.503184, 0.506402, 0.509601, 0.512782, 0.515946, 0.519091, 0.522219, 0.52533, 0.528423, 0.531499, 0.534558, 0.537601, 0.540626, 0.543635, 0.546627, 0.549603, 0.552563, 0.555507, 0.558434, 0.561346, 0.564242, 0.567123, 0.569988, 0.572838, 0.575673, 0.578493, 0.581298, 0.584088, 0.586863, 0.589623, 0.59237, 0.595101, 0.597819, 0.600522, 0.603212, 0.605887, 0.608549, 0.611197, 0.613831, 0.616452, 0.619059, 0.621653, 0.624234, 0.626802, 0.629357, 0.631899, 0.634428, 0.636944, 0.639448, 0.641939, 0.644418, 0.646885, 0.649339, 0.651781, 0.654212, 0.65663, 0.659036, 0.661431, 0.663813, 0.666185, 0.668544, 0.670893, 0.673229, 0.675555, 0.677869, 0.680173, 0.682465, 0.684746, 0.687017, 0.689276, 0.691525, 0.693763, 0.695991, 0.698208, 0.700415, 0.702611, 0.704797, 0.706973, 0.709139, 0.711294, 0.71344, 0.715576, 0.717702, 0.719818, 0.721924, 0.724021, 0.726108, 0.728186, 0.730254, 0.732313, 0.734362, 0.736402, 0.738433, 0.740455, 0.742468
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

		void* GetOutput();
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

		std::array<float, 746*2> apuSamples{{0}};
		uint8_t nearestCounter = 0;
};
