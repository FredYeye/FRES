#pragma once

#include <cstdint>


class apu
{
	public:
		void Pulse1Write(uint8_t dataBus);
		void Pulse1SweepWrite(uint8_t dataBus);
		void Pulse1TimerWrite(uint8_t dataBus);

		void StatusWrite(uint8_t dataBus);
		void FrameCounterWrite(uint8_t dataBus);
	private:
		uint8_t pulse1 = 0; //name?
		uint8_t pulse1Sweep = 0;
		uint8_t pulse1Timer = 0;

		uint8_t status = 0;
		uint8_t frameCounter = 0;
};
