#include "ppu.hpp"
#include <cstring>
#include <iostream>


Ppu::ppu()
{
	oam.fill(0xFF);
}


uint8_t Ppu::StatusRead() //2002
{
	if(scanlineV == 241)
	{
		if(scanlineH == 1) //nmi checks get offset by +1
		{
			suppressNmiFlag = true;
			//     0: reads it as clear and never sets the flag or generates NMI for that frame
		}
		else if(scanlineH == 2 || scanlineH == 3)
		{
			suppressNmi = true;
			//   1-2: reads it as set, clears it, and suppresses the NMI for that frame
			// 340,3: behaves normally (reads flag's value, clears it, and doesn't affect NMI operation)
		}
	}

	const uint8_t status = ppuStatus;
	ppuStatus &= 0x7F; //clear nmi_occurred bit
	wToggle = false;

	return status;
}


uint8_t Ppu::OamDataRead()
{
	//do more stuff (reads while sprite eval is running)
	return oam[oamAddr];
}


uint8_t Ppu::DataRead() //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18)) //figure out what happens otherwise
	{
		//if in palette range, reload latch(mirrored down) but send palette byte immediately
		//perform NT mirroring here?

		uint8_t currentLatch = ppuDataLatch;
		ppuDataLatch = vram[ppuAddress];
		ppuAddress += (ppuCtrl & 0x04) ? 0x20 : 0x01;
		return currentLatch;
	}

	return 0;
}


void Ppu::CtrlWrite(uint8_t dataBus) //2000
{
	ppuCtrl = dataBus;
	ppuAddressLatch = (ppuAddressLatch & 0x73FF) | ((ppuCtrl & 0x03) << 10);
}


void Ppu::MaskWrite(uint8_t dataBus) //2001
{
	ppuMask = dataBus;
}


void Ppu::OamAddrWrite(uint8_t dataBus) //2003
{
	oamAddr = dataBus;
}


void Ppu::OamDataWrite(uint8_t dataBus) //2004
{
	oam[oamAddr++] = dataBus;
}


void Ppu::ScrollWrite(uint8_t dataBus) //2005
{
	if(!wToggle)
	{
		ppuAddressLatch = (ppuAddressLatch & 0x7FE0) | (dataBus >> 3);
		fineX = dataBus & 0x07;
	}
	else
	{
		ppuAddressLatch = (ppuAddressLatch & 0xC1F) | ((dataBus & 0x07) << 12) | ((dataBus & 0xF8) << 2);
	}
	wToggle = !wToggle;
}


void Ppu::AddrWrite(uint8_t dataBus) //2006
{
	if(!wToggle)
	{
		ppuAddressLatch = (ppuAddressLatch & 0xFF) | ((dataBus & 0x3F) << 8);
	}
	else
	{
		ppuAddressLatch = (ppuAddressLatch & 0x7F00) | dataBus;
		ppuAddress = ppuAddressLatch & 0x3FFF; //assuming only the address gets mirrored down, not the latch
	}
	wToggle = !wToggle;
}


void Ppu::DataWrite(uint8_t dataBus) //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
	{
		if(ppuAddress >= 0x3F10 && !(ppuAddress & 3)) //sprite palette mirror write
		{
			vram[ppuAddress & ~0x10] = dataBus; //need else after this?
		}

		vram[ppuAddress] = dataBus; //perform NT mirroring here?
		ppuAddress += (ppuCtrl & 0x04) ? 0x20 : 0x01;
	}
}


void Ppu::Tick()
{
	if(scanlineV < 240) //visible scanlines
	{
		if(scanlineH && scanlineH <= 256)
		{
			uint8_t bgPixel = 0;
			uint8_t spritePixel = 0;
			bool spritePriority = true; //false puts sprite in front of BG

			if(ppuMask & 0b00010000)
			{
				for(uint8_t x = 0; x < 8; ++x)
				{
					if(!spriteXpos[x] && !spritePixel)
					{
						spritePixel = spriteBitmapLow[x] >> 7;
						spritePixel |= (spriteBitmapHigh[x] >> 6) & 0b10;

						spriteBitmapLow[x] <<= 1;
						spriteBitmapHigh[x] <<= 1;
						spritePriority = spriteAttribute[x] & 0b00100000;

						if(spritePixel)
						{
							spritePixel |= (spriteAttribute[x] & 0b11) << 2;
						}
					}
					else if(!spriteXpos[x])
					{
						spriteBitmapLow[x] <<= 1;
						spriteBitmapHigh[x] <<= 1;
					}
				}

				if(scanlineH <= 8 && !(ppuMask & 0b00000100))
				{
					spritePixel = 0;
				}
			}

			if(ppuMask & 0b00001000)
			{
				bgPixel = (bgLow >> (15 - fineX)) & 1;
				bgPixel |= (bgHigh >> (14 - fineX)) & 2;

				if(bgPixel)
				{
					bgPixel |= (attribute >> (28 - fineX * 2)) & 0x0C;

					if(scanlineH <= 8 && !(ppuMask & 0b00000010))
					{
						bgPixel = 0;
					}
					else if(spritePixel && scanlineH != 256)
					{
						ppuStatus |= 0b01000000; //sprite 0 hit
					}
				}
			}

			uint16_t pIndex = vram[0x3F00 + bgPixel] * 3;
			if(spritePixel && (!spritePriority || !(bgPixel & 0b11)))
			{
				pIndex = vram[0x3F10 + spritePixel] * 3;
			}

			const uint32_t renderPos = (scanlineV * 256 + (scanlineH - 1)) * 3;
			std::memcpy(&render[renderPos], &palette[pIndex], 3);
		}

		RenderFetches();
		if(ppuMask & 0b00011000)
		{
			OamScan();
		}
	}
	else if(scanlineV == 241) //vblank scanlines
	{
		if(scanlineH == 1)
		{
			if(suppressNmiFlag == false)
			{
				ppuStatus |= 0x80; //VBL nmi
			}
		}
	}
	else if(scanlineV == 261) //prerender scanline
	{
		if(scanlineH == 1)
		{
			ppuStatus &= 0x1F; //clear sprite overflow, sprite 0 hit and vblank
			suppressNmi = false;
			suppressNmiFlag = false;
		}
		else if(scanlineH >= 280 && scanlineH <= 304 && ppuMask & 0x18)
		{
			ppuAddress = (ppuAddress & 0x41F) | (ppuAddressLatch & 0x7BE0);
		}

		RenderFetches();

		if(oddFrame && scanlineH == 339 && ppuMask & 0x18)
		{
			++scanlineH;
		}
	}

	if(scanlineH++ == 340)
	{
		scanlineH = 0;
		if(scanlineV++ == 261)
		{
			scanlineV = 0;
			oddFrame = !oddFrame;
			renderFrame = true;
		}
	}
}


void Ppu::RenderFetches() //things done during visible and prerender scanlines
{
	if(ppuMask & 0b00011000) //if rendering is enabled
	{
		if(scanlineH == 256) //Y increment
		{
			if(ppuAddress < 0x7000)
			{
				ppuAddress += 0x1000;
			}
			else
			{
				ppuAddress &= 0xFFF;
				if((ppuAddress & 0x3E0) == 0x3A0)
				{
					ppuAddress &= 0xC1F;
					ppuAddress ^= 0x800;
				}
				else if((ppuAddress & 0x3E0) == 0x3E0)
				{
					ppuAddress &= 0xC1F;
				}
				else
				{
					ppuAddress += 0x20;
				}
			}
		}
		else if(scanlineH == 257)
		{
			ppuAddress = (ppuAddress & 0x7BE0) | (ppuAddressLatch & 0x41F);
		}

		if((scanlineH && scanlineH <= 256) || scanlineH >= 321)
		{
			switch(scanlineH & 0x07)
			{
				case 0: //coarse X increment
					if((ppuAddress & 0x1F) != 0x1F)
					{
						++ppuAddress;
					}
					else
					{
						ppuAddress = (ppuAddress & 0x7FE0) ^ 0x400;
					}
					break;

				//value is determined on the first cycle of each fetch (1,3,5,7)
				case 1: //NT
					if(scanlineH != 1 && scanlineH != 321) 
					{
						bgLow |= bgLowLatch;
						bgHigh |= bgHighLatch;
						attribute |= (attributeLatch & 0x03) * 0x5555; //2-bit splat
					}
					nametable = vram[0x2000 | (ppuAddress & 0xFFF)];
					break;
				case 3: //AT
					if(scanlineH != 339)
					{
						attributeLatch = vram[0x23C0 | (ppuAddress & 0xC00) | (ppuAddress >> 4 & 0x38) | (ppuAddress >> 2 & 0x07)];
						attributeLatch >>= (((ppuAddress >> 1) & 0x01) | ((ppuAddress >> 5) & 0x02)) * 2;
					}
					else
					{
						nametable = vram[0x2000 | (ppuAddress & 0xFFF)];
					}
					break;
				case 5: //low
					bgAddress = nametable*16 + (ppuAddress >> 12) | ((ppuCtrl & 0x10) << 8);
					bgLowLatch = vram[bgAddress];
					break;
				case 7: //high
					bgHighLatch = vram[bgAddress + 8];
					break;
			}

			if(scanlineH <= 256)
			{
				for(uint8_t &x : spriteXpos)
				{
					if(x)
					{
						--x;
					}
				}
			}

			if(scanlineH <= 336)
			{
				bgLow <<= 1;
				bgHigh <<= 1;
				attribute <<= 2;
			}
		}
		else //257-320
		{
			oamAddr = 0;

			//sprite fetching
			switch(scanlineH & 0x07)
			{
				case 3: //load sprite attr
					spriteAttribute[spriteIndex] = oam2[spriteIndex * 4 + 2];
					break;
				case 4: //load sprite X
					spriteXpos[spriteIndex] = oam2[spriteIndex * 4 + 3];
					break;
				case 5: //sprite low
					bgAddress = ((ppuCtrl & 0b1000) << 9) | (oam2[spriteIndex * 4 + 1] << 4);
					{
					uint8_t yOffset = (scanlineV - oam2[spriteIndex * 4]);
					if(spriteAttribute[spriteIndex] & 0b10000000) //flip V
					{
						yOffset = ~yOffset;
					}
					bgAddress |= yOffset & 7;
					}

					spriteBitmapLow[spriteIndex] = vram[bgAddress];
					if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
					{
						ReverseBits(spriteBitmapLow[spriteIndex]);
					}
					break;
				case 7: //sprite high
					spriteBitmapHigh[spriteIndex] = vram[bgAddress | 8];
					if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
					{
						ReverseBits(spriteBitmapHigh[spriteIndex]);
					}
					if(++spriteIndex == 8)
					{
						spriteIndex = 0;
					}
					break;
			}
		}
	}
}


void Ppu::OamScan()
{
	if(scanlineH && !(scanlineH & 1))
	{
		if(scanlineH <= 64)
		{
			oam2[(scanlineH >> 1) - 1] = 0xFF;
		}
		else if(scanlineH <= 256)
		{
			if(!oamEvalPattern) //search for sprites in range
			{
				oam2[oam2Index] = oam[oamSpritenum]; //oam search starts at oam[oamAddr]
				if(scanlineV >= oam[oamSpritenum] && scanlineV < oam[oamSpritenum] + 8 + (ppuCtrl >> 2 & 8))
				{
					++oamEvalPattern;
					++oam2Index;
				}
				else
				{
					OamUpdateIndex();
				}
			}
			else if(oamEvalPattern <= 3)
			{
				oam2[oam2Index++] = oam[oamSpritenum + oamEvalPattern++];
				if(oamEvalPattern > 3)
				{
					OamUpdateIndex();						
				}
			}
			else if(oamEvalPattern == 4) //entire oam searched
			{
				oamSpritenum += 4;
			}
			else //search for overflow
			{
				if(scanlineV >= oam[oamSpritenum] && scanlineV < oam[oamSpritenum] + 8 + (ppuCtrl >> 2 & 8))
				{
					ppuStatus |= 0x20;
					// std::cout << "OF ";
				}
				oamSpritenum += 5; //incorrect, should be +=4, +0-3 (inc n and m w/o carry)
				if(oamSpritenum <= 4) //stop searching. <= 4 instead of 0 since increment is bugged
				{
					oamEvalPattern = 4;
				}
			}
		}
		else
		{
			oam2Index = 0;
			oamSpritenum = 0;
			oamEvalPattern = 0;			
		}
	}
}


void Ppu::OamUpdateIndex()
{
	oamSpritenum += 4;
	if(!oamSpritenum) //entire oam searched
	{
		oamEvalPattern = 4;
	}
	else if(oam2Index < 32)
	{
		oamEvalPattern = 0;
	}
	else if(oam2Index == 32) //8 sprites found
	{
		oamEvalPattern = 5;
	}
}


bool Ppu::PollNmi()
{
	if(suppressNmi)
	{
		return false;
	}

	bool nmi = ppuStatus & ppuCtrl & 0x80;
	if(nmi & !oldNmi)
	{
		oldNmi = nmi;
		return true;
	}
	else
	{
		oldNmi = nmi;
		return false;
	}
}


void Ppu::ReverseBits(uint8_t &b)
{
	b = b >> 4 | b << 4;
	b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
	b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
}


const bool Ppu::RenderFrame()
{
	const bool currentRenderFrame = renderFrame;
	renderFrame = false;
	return currentRenderFrame;
}


std::array<uint8_t, 0x4000>::iterator Ppu::VramIterator()
{
	return vram.begin();
}


const std::array<uint8_t, 256*240*3>* const Ppu::GetPixelPtr() const
{
	return &render;
}


uint16_t Ppu::GetScanlineH()
{
	return scanlineH;
}


uint16_t Ppu::GetScanlineV()
{
	return scanlineV;
}


const std::array<uint8_t, 0x4000>& Ppu::GetVram()
{
	return vram;
}
