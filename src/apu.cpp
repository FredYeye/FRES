#include "apu.hpp"


void apu::Pulse1_1Write(uint8_t dataBus)
{
	pulse1 = dataBus;
}


void apu::Pulse1_2Write(uint8_t dataBus)
{
	pulse1Sweep = dataBus;
}


void apu::Pulse1_3Write(uint8_t dataBus)
{
	pulse1Timer = dataBus;
}


void apu::Pulse1_4Write(uint8_t dataBus)
{
	pulse1Length = dataBus;
}


void apu::Noise1Write(uint8_t dataBus)
{
	noiseHalt = dataBus & 0b00100000;
	noiseConstant = dataBus & 0b00010000;
	noiseDivider = dataBus & 0b00001111;
}


void apu::Noise2Write(uint8_t dataBus)
{
	noiseMode = dataBus & 0b10000000;
	noisePeriodIndex = dataBus & 0b00001111;
}


void apu::Noise3Write(uint8_t dataBus)
{
	noiseLength = (dataBus & 0b11111000) >> 3;
	//restart envelope
}


void apu::StatusWrite(uint8_t dataBus)
{
	status = (status & 0b01100000) | (dataBus & 00011111);
}


uint8_t apu::StatusRead()
{
	return status;
}


void apu::FrameCounterWrite(uint8_t dataBus)
{
	frameCounter = (frameCounter & 0b00111111) | (dataBus & 0b11000000);
}


void apu::Tick()
{
	if(oddEvenTick)
	{
		if(!--noiseTimer)
		{
			bool feedback = noiseShiftReg & 1;
			noiseShiftReg >>= 1;
			feedback ^= noiseShiftReg & 1;
			noiseShiftReg |= feedback << 14;
		}
	}

	oddEvenTick = !oddEvenTick;
}
