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
	pulse[channel].sweepTimer = ((dataBus >> 4) & 0b0111); //+1?
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


void Apu::Triangle0Write(uint8_t dataBus) //4008
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


void Apu::Dmc0Write(uint8_t dataBus) //4010
{
	const std::array<uint16_t, 16> rateIndex
	{{
		428/2, 380/2, 340/2, 320/2, 286/2, 254/2, 226/2, 214/2,
		190/2, 160/2, 142/2, 128/2, 106/2,  84/2,  72/2,  54/2
	}};

	dmc.enableIrq = dataBus & 0b10000000;
	dmc.loop = dataBus & 0b01000000;
	dmc.freqTimer = rateIndex[dataBus & 0b1111];

	if(!dmc.enableIrq)
	{
		dmc.irqPending = false;
	}
}


void Apu::Dmc1Write(uint8_t dataBus) //4011
{
	dmc.output = dataBus & 0b01111111; //todo: timer quirk
}


void Apu::Dmc2Write(uint8_t dataBus) //4012
{
	dmc.addressLoad = 0xC000 | (dataBus << 6);
}


void Apu::Dmc3Write(uint8_t dataBus) //4013
{
	dmc.sampleLength = (dataBus << 4) + 1;
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

	// dmc.enable = dataBus & 0b00010000;
	if(dataBus & 0b00010000)
	{
		if(!dmc.samplesRemaining)
		{
			dmc.samplesRemaining = dmc.sampleLength;
			dmc.address = dmc.addressLoad;
		}
	}
	else
	{
		dmc.samplesRemaining = 0;
	}
	dmc.irqPending = false;
}


uint8_t Apu::StatusRead() //4015
{
	uint8_t data = 0;

	data  = bool(pulse[0].lengthCounter);
	data |= bool(pulse[1].lengthCounter) << 1;
	data |= bool(triangle.lengthCounter) << 2;
	data |= bool(noise.lengthCounter   ) << 3;
	data |= bool(dmc.samplesRemaining  ) << 4;

	data |= frameIRQ << 6;
	frameIRQ = false;

	data |= dmc.irqPending << 7;

	return data;
}


void Apu::FrameCounterWrite(uint8_t dataBus) //4017
{
	blockIRQ = dataBus & 0b01000000;
	if(blockIRQ)
	{
		frameIRQ = false;
	}

	sequencerMode = dataBus & 0b10000000;
	if(sequencerMode == 1)
	{
		QuarterFrame();
		HalfFrame();
	}

	sequencerResetDelay[3] = apuTick; //reset the sequencer timer 3 cycles later if on a apu tick, else 4. add one cycle because the current cycle doesn't count
	sequencerResetDelay[4] = true;
}


const uint16_t Apu::GetDmcAddr() const
{
	return dmc.address;
}


void Apu::DmcDma(uint8_t sample)
{
	dmc.sampleBuffer = sample;
	++dmc.address |= 0x8000;
	dmc.sampleBufferEmpty = false;

	if(--dmc.samplesRemaining == 0)
	{
		if(dmc.loop)
		{
			dmc.samplesRemaining = dmc.sampleLength;
			dmc.address = dmc.addressLoad;
		}
		else if(dmc.enableIrq)
		{
			dmc.irqPending = true;
		}
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
		if(--triangle.freqCounter <= 0)
		{
			triangle.freqCounter = triangle.freqTimer;
			++triangle.sequencerStep &= 0b00011111;
		}
	}

	for(int x = 0; x < 4; x++)
	{
		sequencerResetDelay[x] = sequencerResetDelay[x+1];
	}
	if(sequencerResetDelay[0])
	{
		sequencerCounter = 0;
		sequencerResetDelay.fill(false);
	}

	switch(sequencerCounter++)
	{
		case 37281: //step 4 of mode 1
			if(sequencerMode == 1)
			{
				sequencerCounter = 0;
			}
		case 14913: //steps 2 of both modes
			HalfFrame();
		case 7457: case 22371: //steps 1 and 3 of both modes
			QuarterFrame();
		break;

		case 29830: //step 4 of mode 0, part 2
			if(sequencerMode == 0)
			{
				sequencerCounter = 1; //wrap to 0+1 now and set frameIRQ
			}
		case 29828: //pre-step 4 irq flag
			frameIRQ = !(blockIRQ | sequencerMode);
		break;

		case 29829: //step 4 of mode 0
			if(sequencerMode == 0)
			{
				frameIRQ = !blockIRQ;
				QuarterFrame();
				HalfFrame();
				//can't wrap sequencerCounter to 0 yet, need to set frameIRQ on wrap to 0
			}
		break;
	}

	if(apuTick)
	{
		for(auto &p : pulse)
		{
			if(--p.freqCounter <= 0)
			{
				p.freqCounter = p.freqTimer;
				p.dutyCounter <<= 1;
				if(!p.dutyCounter)
				{
					p.dutyCounter = 1;
				}
			}
		}

		if(--noise.freqCounter <= 0)
		{
			noise.freqCounter = noise.freqTimer;
			bool feedback = (noise.mode) ? noise.lfsr & 0b01000000 : noise.lfsr & 0b00000010;
			feedback ^= noise.lfsr & 1;
			noise.lfsr >>= 1;
			noise.lfsr |= feedback << 14;
		}

		if(--dmc.freqCounter <= 0)
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
			if(--dmc.bitsRemaining == 0)
			{
				dmc.bitsRemaining = 8; //doing this in the if case below makes dmc_basics pass, but not the correct solution (i think...)
				dmc.outputShift = dmc.sampleBuffer;
				dmc.silence = dmc.sampleBufferEmpty;
				dmc.sampleBufferEmpty = true;
			}
		}

		if(dmc.samplesRemaining && dmc.sampleBufferEmpty)
		{
			dmc.bitsRemaining = 8;
			dmcDma = true;
		}

		if(++nearestCounter == outputCounter[outI])
		{
			nearestCounter = 0;
			++outI &= 0b11111;

			uint8_t pulseOutput = 0;
			for(const auto &p : pulse)
			{
				if(p.duty & p.dutyCounter && p.lengthCounter && !SweepForcingSilence(p))
				{
					pulseOutput += (p.constant) ? p.volume : p.envelopeVolume;
				}
			}

			const uint8_t triangleOutput = (ultrasonic) ? 7 : triangle.sequencerTable[triangle.sequencerStep]; //should be 7.5. HMM set to 0?

			uint8_t noiseOutput = 0;
			if(!(noise.lfsr & 1) && noise.lengthCounter)
			{
				noiseOutput = (noise.constant) ? noise.volume : noise.envelopeVolume;
			}

			const float output = mixer.pulse[pulseOutput] + mixer.tnd[triangleOutput * 3 + noiseOutput * 2 + dmc.output];
			apuSamples[sampleCount * 2] = output;
			apuSamples[sampleCount * 2 + 1] = output;
			++sampleCount;
		}
	}

	apuTick = !apuTick;
}


const void* const Apu::GetOutput() const //734 samples/frame = 60.0817), use as timer?
{
	return apuSamples.data();
}


bool Apu::PollFrameInterrupt() const
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
		if(!pulse[x].sweepCounter--)
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
		if(pulse[x].sweepReload)
		{
			pulse[x].sweepCounter = pulse[x].sweepTimer;
			pulse[x].sweepReload = false;
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


bool Apu::SweepForcingSilence(const Pulse &p) const
{
	if(p.freqTimer < 8)
	{
		return true;
	}
	return !p.sweepNegate & (p.freqTimer + (p.freqTimer >> p.sweepShift) >> 11); //if freqTimer >= 0x800
}
