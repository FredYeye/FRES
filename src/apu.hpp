#pragma once

#include <cstdint>
#include <array>


class apu
{
	public:
		void Pulse1_1Write(uint8_t dataBus);     //4000
		void Pulse1_2Write(uint8_t dataBus);     //4001
		void Pulse1_3Write(uint8_t dataBus);     //4002
		void Pulse1_4Write(uint8_t dataBus);     //4003

		void Noise1Write(uint8_t dataBus);       //400C
		void Noise2Write(uint8_t dataBus);       //400E
		void Noise3Write(uint8_t dataBus);       //400F

		void StatusWrite(uint8_t dataBus);       //4015
		uint8_t StatusRead();                    //4015
		void FrameCounterWrite(uint8_t dataBus); //4017

		void Tick();

	private:
		uint8_t pulse1 = 0; //name?
		uint8_t pulse1Sweep = 0;
		uint8_t pulse1Timer = 0;
		uint8_t pulse1Length = 0;

		bool noiseHalt = 0;
		bool noiseConstant = 0;
		uint8_t noiseDivider = 0;
		bool noiseMode = 0;
		uint8_t noisePeriodIndex = 0;
		const std::array<uint16_t,16> noisePeriod{{4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068}};
		uint8_t noiseLength = 0;
		uint16_t noiseTimer = 0;
		uint16_t noiseShiftReg = 1;

		uint8_t status = 0;
		uint8_t frameCounter = 0;

		bool oddEvenTick = false; //sufficient for timers that tick every 2 cpu cycles?
};
