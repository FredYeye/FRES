#include <iostream>
#include "apu.hpp"


void apu::Pulse1Write(uint8_t dataBus, bool channel)
{
	const std::array<uint8_t, 4> dutyTable{{0b0000010, 0b00000110, 0b00011110, 0b11111001}};
	pulse[channel].duty = dutyTable[dataBus >> 6];
	pulse[channel].halt = dataBus & 0b00100000;
	pulse[channel].constant = dataBus & 0b00010000; //constant volume or envelope flag?
	pulse[channel].volume = dataBus & 0b00001111;   //volume or envelope speed?
}


void apu::Pulse2Write(uint8_t dataBus, bool channel)
{
	pulse[channel].sweepTimer = (dataBus >> 4) & 0b0111;
	pulse[channel].sweepNegate = dataBus & 0b1000;
	pulse[channel].sweepShift = dataBus & 0b0111;
	pulse[channel].sweepReload = true;
	pulse[channel].sweepEnabled = (dataBus & 0b10000000) && pulse[channel].sweepShift;
}


void apu::Pulse3Write(uint8_t dataBus, bool channel)
{
	pulse[channel].freqTimer = (pulse[channel].freqTimer & 0x700) | dataBus; //save high 3 bits or not?
}


void apu::Pulse4Write(uint8_t dataBus, bool channel)
{
	pulse[channel].freqTimer = (pulse[channel].freqTimer & 0xFF) | ((dataBus & 0b00000111) << 8);
	if(pulse[channel].enable)
	{
		pulse[channel].lengthCounter = lengthTable[dataBus >> 3];
	}
	pulse[channel].freqCounter = pulse[channel].freqTimer;
	pulse[channel].dutyCounter = 1;
	pulse[channel].envelopeReset = true;
}


void apu::Noise1Write(uint8_t dataBus)
{
	noiseHalt     = dataBus & 0b00100000;
	noiseConstant = dataBus & 0b00010000;
	noiseDivider  = dataBus & 0b00001111;
}


void apu::Noise2Write(uint8_t dataBus)
{
	noiseMode = dataBus & 0b10000000;
	noisePeriodIndex = dataBus & 0b00001111;
}


void apu::Noise3Write(uint8_t dataBus)
{
	if(noiseEnable)
	{
		noiseLength = lengthTable[dataBus >> 3];
	}
	noiseRestartEnvelope = true;
}


void apu::StatusWrite(uint8_t dataBus) //4015
{
	pulse[0].enable = dataBus & 1;
	if(!pulse[0].enable)
	{
		pulse[0].lengthCounter = 0;
	}
	pulse[1].enable = dataBus & 0b10;
	if(!pulse[1].enable)
	{
		pulse[1].lengthCounter = 0;
	}
}


uint8_t apu::StatusRead()
{
	uint8_t data = 0;
	if(pulse[0].lengthCounter)
	{
		data |= 1;
	}
	if(pulse[1].lengthCounter)
	{
		data |= 0b10;
	}
	data |= frameIRQ << 6;

	frameIRQ = false;
	return data;
}


void apu::FrameCounterWrite(uint8_t dataBus) //4017
{
	sequencerMode = dataBus & 0b10000000;
	blockIRQ = dataBus & 0b01000000;
	if(blockIRQ)
	{
		frameIRQ = false;
	}

	sequencerCounter = 0; //delayed by 3-4 cpu cycles

	if(sequencerMode)
	{
		QuarterFrame();
		HalfFrame();
	}
}


void apu::Tick()
{
	if(oddEvenTick)
	{
		for(auto &p : pulse)
		{
			if(!p.freqCounter--)
			{
				p.freqCounter = p.freqTimer;
				p.dutyCounter <<= 1;
				if(!p.dutyCounter)
				{
					p.dutyCounter = 1;
				}
			}
		}

		switch(sequencerCounter)
		{
			case 0:
				if(!sequencerMode)
				{
					frameIRQ = !blockIRQ;
				}
			break;

			case 7457:
				QuarterFrame();
			break;

			case 14913:
				QuarterFrame();
				HalfFrame();
			break;

			case 22371:
				QuarterFrame();
			break;

			case 29828:
				frameIRQ = !(blockIRQ | sequencerMode);
			break;

			case 29829:
				if(!sequencerMode)
				{
					frameIRQ = !blockIRQ;
					QuarterFrame();
					HalfFrame();
					sequencerCounter = 0xFFFF; //wrap
				}
			break;

			case 37281:
				if(sequencerMode)
				{
					QuarterFrame();
					HalfFrame();
					sequencerCounter = 0xFFFF; //wrap
				}
			break;
		}

		// if(!noiseTimer--)
		// {
			// noiseTimer = noisePeriod[noisePeriodIndex];
			// bool feedback = (noiseMode) ? noiseShiftReg & 0b01000000 : noiseShiftReg & 0b00000010;
			// feedback ^= noiseShiftReg & 1;
			// noiseShiftReg >>= 1;
			// noiseShiftReg |= feedback << 14;
		// }

		uint8_t output = 0;
		for(auto &p : pulse)
		{
			if(p.duty & p.dutyCounter && p.lengthCounter && !SweepForcingSilence(p))
			{
				if(p.constant)
				{
					output += p.volume;
				}
				else
				{
					output += p.envelopeVolume;
				}
			}
		}

		if(++nearestCounter == 20)
		{
			// uint8_t asd = uint8_t((95.88 / (8128.0f / output + 100.0f)) * 255.0f);
			// noiseOutput.push_back(asd);
			
			noiseOutput.push_back(output);
			++sampleCount;
			nearestCounter = 0;
		}
	}

	++sequencerCounter;
	oddEvenTick = !oddEvenTick;
}


uint8_t* apu::GetOutput() //734 samples/frame = 60.0817), use as timer?
{
	return noiseOutput.data();
}


void apu::ClearOutput()
{
	noiseOutput.clear();
	sampleCount = 0;
}


void apu::QuarterFrame()
{
	for(auto &p : pulse)
	{
		if(p.envelopeReset)
		{
			p.envelopeReset = false;
			p.envelopeVolume = 15;
			p.envelopeCounter = p.volume;
		}
		else
		{
			if(!p.envelopeCounter--)
			{
				p.envelopeCounter = p.volume;
				if(p.envelopeVolume)
				{
					--p.envelopeVolume;
				}
				else if(p.halt)
				{
					p.envelopeVolume = 15;
				}
			}
		}
	}
}


void apu::HalfFrame()
{
	for(uint8_t x = 0; x <= 1; ++x)
	{
		if(pulse[x].sweepReload) //edge case(?) not handled yet
		{
			pulse[x].sweepCounter = pulse[x].sweepTimer;
			pulse[x].sweepReload = false;
		}
		else if(!pulse[x].sweepCounter--)
		{
			pulse[x].sweepCounter = pulse[x].sweepTimer;
			if(pulse[x].sweepEnabled && !SweepForcingSilence(pulse[x]))
			{
				if(pulse[x].sweepNegate)
				{
					pulse[x].freqTimer -= (pulse[x].freqTimer >> pulse[x].sweepShift) + (x^1);
				}
				else
				{
					pulse[x].freqTimer += (pulse[x].freqTimer >> pulse[x].sweepShift);
				}
			}
		}

		if(!pulse[x].halt && pulse[x].lengthCounter)
		{
			--pulse[x].lengthCounter;
		}
	}
}



bool apu::SweepForcingSilence(const Pulse &p)
{
	//>= 0x800 surely can be done better, also see !sweepNegate above
	if(p.freqTimer < 8 || (!p.sweepNegate && p.freqTimer + (p.freqTimer >> p.sweepShift) >= 0x800))
	{
		return true;
	}
	return false;
}