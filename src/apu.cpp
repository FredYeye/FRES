#include "apu.hpp"


void apu::Pulse1Write(uint8_t dataBus) //4000
{
	pulse1 = dataBus;
}


void apu::Pulse1SweepWrite(uint8_t dataBus) //4001
{
	pulse1Sweep = dataBus;
}


void apu::Pulse1TimerWrite(uint8_t dataBus) //4002
{
	pulse1Timer = dataBus;
}


void apu::Pulse1LengthWrite(uint8_t dataBus) //4003
{
	pulse1Length = dataBus;
}


void apu::StatusWrite(uint8_t dataBus) //4015
{
	status = (status & 0b01100000) | (dataBus & 00011111);
}


uint8_t apu::StatusRead() //4015
{
	return status;
}


void apu::FrameCounterWrite(uint8_t dataBus) //4017
{
	frameCounter = (frameCounter & 0b00111111) | (dataBus & 0b11000000);
}