#include <iostream>
#include "apu.hpp"


Apu::Apu()
{
	for(uint8_t x = 0; x < 31; ++x)
	{
		// mixer.pulse[x] = std::round((95.52 / (8128.0 / x+100)) * 255.0); //8-bit
		// mixer.pulse[x] = std::round((95.52 / (8128.0 / x+100)) * 65535.0); //16-bit
		mixer.pulse[x] = 95.52 / (8128.0 / x+100); //float
	}

	for(uint8_t x = 0; x < 203; ++x)
	{
		// mixer.tnd[x] = std::round((163.67 / (24329.0 / x+100)) * 255.0); //8-bit
		// mixer.tnd[x] = std::round((163.67 / (24329.0 / x+100)) * 65535.0); //16-bit
		mixer.tnd[x] = 163.67 / (24329.0 / x+100); //float
	}
}


void Apu::Pulse0Write(uint8_t dataBus, bool channel)
{
	const std::array<uint8_t, 4> dutyTable{{0b0000010, 0b00000110, 0b00011110, 0b11111001}};

	pulse[channel].duty = dutyTable[dataBus >> 6];
	pulse[channel].halt = dataBus & 0b00100000;
	pulse[channel].constant = dataBus & 0b00010000; //constant volume or envelope flag?
	pulse[channel].volume = dataBus & 0b00001111;   //volume or envelope speed?
}


void Apu::Pulse1Write(uint8_t dataBus, bool channel)
{
	pulse[channel].sweepTimer = (dataBus >> 4) & 0b0111;
	pulse[channel].sweepNegate = dataBus & 0b1000;
	pulse[channel].sweepShift = dataBus & 0b0111;
	pulse[channel].sweepReload = true;
	pulse[channel].sweepEnabled = (dataBus & 0b10000000) && pulse[channel].sweepShift;
}


void Apu::Pulse2Write(uint8_t dataBus, bool channel)
{
	pulse[channel].freqTimer = (pulse[channel].freqTimer & 0x700) | dataBus; //save high 3 bits or not?
}


void Apu::Pulse3Write(uint8_t dataBus, bool channel)
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


void Apu::Triangle0Write(uint8_t dataBus)
{
	triangle.halt = dataBus & 0b10000000;
	triangle.linearLoad = dataBus & 0b01111111;
}


void Apu::Triangle2Write(uint8_t dataBus)
{
	triangle.freqTimer = (triangle.freqTimer & 0x700) | dataBus;
}


void Apu::Triangle3Write(uint8_t dataBus)
{
	triangle.freqTimer = (triangle.freqTimer & 0xFF) | ((dataBus & 0b00000111) << 8);
	if(triangle.enable)
	{
		triangle.lengthCounter = lengthTable[dataBus >> 3];
	}
	triangle.linearReload = true;
}


void Apu::Noise0Write(uint8_t dataBus)
{
	noise.halt = dataBus & 0b00100000;
	noise.constant = dataBus & 0b00010000;
	noise.volume = dataBus & 0b00001111;
}


void Apu::Noise2Write(uint8_t dataBus)
{
	const std::array<uint16_t, 16> noisePeriod
	{{
		4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
	}};

	noise.freqTimer = noisePeriod[dataBus & 0b1111];
	noise.mode = dataBus & 0b10000000;
}


void Apu::Noise3Write(uint8_t dataBus)
{
	if(noise.enable)
	{
		noise.lengthCounter = lengthTable[dataBus >> 3];
	}
	noise.envelopeReset = true;
}


void Apu::Dmc0Write(uint8_t dataBus)
{
	const std::array<uint16_t, 16> rateIndex
	{{
		428/2, 380/2, 340/2, 320/2, 286/2, 254/2, 226/2, 214/2,
		190/2, 160/2, 142/2, 128/2, 106/2,  84/2,  72/2,  54/2
	}};

	dmc.enableIrq = dataBus & 0b10000000;
	dmc.loop = dataBus & 0b01000000;
	dmc.freqTimer = dataBus & 0b1111;

	if(!dmc.enableIrq)
	{
		dmc.irqPending = false;
	}
}


void Apu::Dmc1Write(uint8_t dataBus)
{
	dmc.output = dataBus & 0b01111111; //todo: timer quirk
}


void Apu::Dmc2Write(uint8_t dataBus)
{
	dmc.addressLoad = 0xC000 | (dataBus << 6);
}


void Apu::Dmc3Write(uint8_t dataBus)
{
	dmc.sampleLengthLoad = (dataBus << 4) + 1;
}


void Apu::StatusWrite(uint8_t dataBus) //4015
{
	pulse[0].enable = dataBus & 0b0001;
	if(!pulse[0].enable)
	{
		pulse[0].lengthCounter = 0;
	}

	pulse[1].enable = dataBus & 0b0010;
	if(!pulse[1].enable)
	{
		pulse[1].lengthCounter = 0;
	}

	triangle.enable = dataBus & 0b0100;
	if(!triangle.enable)
	{
		triangle.lengthCounter = 0;
	}

	noise.enable = dataBus & 0b1000;
	if(!noise.enable)
	{
		noise.lengthCounter = 0;
	}

	dmc.enable = dataBus & 0b00010000;
	if(dmc.enable && !dmc.sampleLength)
	{
		dmc.sampleLength = dmc.sampleLengthLoad;
		dmc.address = dmc.addressLoad;
	}
	else
	{
		dmc.sampleLength = 0;
	}
	dmc.irqPending = false;
}


uint8_t Apu::StatusRead() //4015
{
	uint8_t data = 0;
	if(pulse[0].lengthCounter)
	{
		data |= 0b0001;
	}
	if(pulse[1].lengthCounter)
	{
		data |= 0b0010;
	}
	if(triangle.lengthCounter)
	{
		data |= 0b0100;
	}
	if(noise.lengthCounter)
	{
		data |= 0b1000;
	}
	if(dmc.sampleLength)
	{
		data |= 0b00010000;
	}
	data |= dmc.irqPending << 7;

	data |= frameIRQ << 6;
	frameIRQ = false;

	return data;
}


void Apu::FrameCounterWrite(uint8_t dataBus) //4017
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


void Apu::Tick()
{
	bool ultrasonic = false;
	if(triangle.freqTimer < 2 && !triangle.freqCounter)
	{
		ultrasonic = true;
	}
	if(triangle.lengthCounter && triangle.linearCounter && !ultrasonic)
	{
		if(!triangle.freqCounter--)
		{
			triangle.freqCounter = triangle.freqTimer;
			++triangle.sequencerStep &= 0b00011111;
		}
	}

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

		if(!noise.freqCounter--)
		{
			noise.freqCounter = noise.freqTimer;
			bool feedback = (noise.mode) ? noise.lfsr & 0b01000000 : noise.lfsr & 0b00000010;
			feedback ^= noise.lfsr & 1;
			noise.lfsr >>= 1;
			noise.lfsr |= feedback << 14;
		}

		if(!dmc.freqCounter--)
		{
			dmc.freqCounter = dmc.freqTimer;
			if(!dmc.silence)
			{
				if(dmc.outputShift & 1)
				{
					if(dmc.output < 0x7E)
					{
						dmc.output += 2;
					}
				}
				else if(dmc.output > 1)
				{
					dmc.output -= 2;
				}
			}

			dmc.outputShift >>= 1;
			--dmc.bitsRemaining &= 0b0111;
			if(!dmc.bitsRemaining)
			{
				dmc.outputShift = dmc.sampleBuffer;
				dmc.silence = !dmc.sampleBuffer;
				//check if is_sample_buffer_empty bool is necessary
			}
		}

		if(dmc.sampleLength && !dmc.bitsRemaining)
		{
			// dmc.sampleBuffer = nes.CpuRead(dma.address);
			++dmc.address |= 0x8000;
			if(!--dmc.sampleLength)
			{
				if(dmc.loop)
				{
					dmc.sampleLength = dmc.sampleLengthLoad;
					dmc.address = dmc.addressLoad;
				}
				else if(dmc.enableIrq)
				{
					dmc.irqPending = true;
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

		uint8_t pulseOutput = 0;
		for(auto &p : pulse)
		{
			if(p.duty & p.dutyCounter && p.lengthCounter && !SweepForcingSilence(p))
			{
				pulseOutput += (p.constant) ? p.volume : p.envelopeVolume;
			}
		}

		uint8_t triangleOutput = (ultrasonic) ? 7 : triangle.sequencerTable[triangle.sequencerStep]; //should be 7.5. HMM set to 0?

		uint8_t noiseOutput = 0;
		if(!(noise.lfsr & 1) && noise.lengthCounter)
		{
			noiseOutput = (noise.constant) ? noise.volume : noise.envelopeVolume;
		}

		if(++nearestCounter == outputCounter[outI])
		{
			nearestCounter = 0;
			++outI &= 0b11111;
			const float output = mixer.pulse[pulseOutput] + mixer.tnd[triangleOutput * 3 + noiseOutput * 2]; //+dmc.output
			apuSamples[sampleCount * 2] = output;
			apuSamples[sampleCount * 2 + 1] = output;
			++sampleCount;
		}
	}

	++sequencerCounter;
	oddEvenTick = !oddEvenTick;
}


void* Apu::GetOutput() //734 samples/frame = 60.0817), use as timer?
{
	return apuSamples.data();
}


bool Apu::PollFrameInterrupt()
{
	return frameIRQ;
}


void Apu::QuarterFrame()
{
	for(auto &p : pulse)
	{
		if(p.envelopeReset)
		{
			p.envelopeReset = false;
			p.envelopeVolume = 15;
			p.envelopeCounter = p.volume;
		}
		else if(!p.envelopeCounter--)
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

	if(triangle.linearReload)
	{
		triangle.linearCounter = triangle.linearLoad;
	}
	else if(triangle.linearCounter)
	{
		--triangle.linearCounter;
	}

	if(!triangle.halt)
	{
		triangle.linearReload = false;
	}

	if(noise.envelopeReset)
	{
		noise.envelopeReset = false;
		noise.envelopeVolume = 15;
		noise.envelopeCounter = noise.volume;
	}
	else if(!noise.envelopeCounter--)
	{
		noise.envelopeCounter = noise.volume;
		if(noise.envelopeVolume)
		{
			--noise.envelopeVolume;
		}
		else if(noise.halt)
		{
			noise.envelopeVolume = 15;
		}
	}
}


void Apu::HalfFrame()
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

	if(!triangle.halt && triangle.lengthCounter)
	{
		--triangle.lengthCounter;
	}

	if(!noise.halt && noise.lengthCounter)
	{
		--noise.lengthCounter;
	}
}


bool Apu::SweepForcingSilence(const Pulse &p)
{
	//>= 0x800 surely can be done better, also see !sweepNegate above
	if(p.freqTimer < 8 || (!p.sweepNegate && p.freqTimer + (p.freqTimer >> p.sweepShift) >= 0x800))
	{
		return true;
	}
	return false;
}
