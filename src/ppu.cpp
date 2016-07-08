#include <cstring>
#include <iostream>

#include "ppu.hpp"


Ppu::Ppu()
{
	oam.fill(0xFF);
}


uint8_t Ppu::StatusRead() //2002
{
	if(scanlineV == 241)
	{
		// nmi checks get offset by +1. see if this is actually stupid later.
		if(scanlineH == 0+1)
		{
			suppressNmiFlag = true;
			//     0: reads it as clear and never sets the flag or generates NMI for that frame
		}
		else if(scanlineH == 1+1 || scanlineH == 2+1)
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


uint8_t Ppu::OamDataRead() //2004
{
	//do more stuff (reads while sprite eval is running)
	return oam[oamAddr];
}


uint8_t Ppu::DataRead() //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18)) //figure out what happens otherwise
	{
		uint8_t currentLatch = (ppuAddress >= 0x3F00) ? vram[ppuAddress & 0x3F1F] : ppuDataLatch;

		if(ppuAddress >= 0x2000)
		{
			ppuDataLatch = vram[ppuAddress & nametableMirroring | nametableBase];
		}
		else
		{
			ppuDataLatch = vram[ppuAddress];
		}
		ppuAddress += (ppuCtrl & 0b0100) ? 0x20 : 0x01;
		return currentLatch;
	}

	return 0;
}


void Ppu::CtrlWrite(uint8_t dataBus) //2000
{
	ppuCtrl = dataBus;
	ppuAddressLatch &= 0x73FF;
	ppuAddressLatch |= (dataBus & 0b11) << 10;
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
		ppuAddressLatch &= 0x7FE0;
		ppuAddressLatch |= dataBus >> 3;
		fineX = dataBus & 0b0111;
	}
	else
	{
		ppuAddressLatch &= 0xC1F;
		ppuAddressLatch |= ((dataBus & 0b0111) << 12) | ((dataBus & 0b11111000) << 2);
	}
	wToggle = !wToggle;
}


void Ppu::AddrWrite(uint8_t dataBus) //2006
{
	if(!wToggle)
	{
		ppuAddressLatch &= 0xFF;
		ppuAddressLatch |= (dataBus & 0b00111111) << 8;
	}
	else
	{
		ppuAddressLatch &= 0x7F00;
		ppuAddressLatch |= dataBus;
		ppuAddress = ppuAddressLatch;
	}
	wToggle = !wToggle;
}


void Ppu::DataWrite(uint8_t dataBus) //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
	{
		if(ppuAddress < 0x2000)
		{
			vram[ppuAddress] = dataBus;
		}
		else if(ppuAddress < 0x3F00)
		{
			vram[ppuAddress & nametableMirroring | nametableBase] = dataBus;
		}
		else
		{
			vram[ppuAddress & 0x3F1F] = dataBus;
			if(!(ppuAddress & 0b11)) //sprite palette mirror write
			{
				vram[(ppuAddress & 0x3F1F) ^ 0x10] = dataBus;
			}
		}
		ppuAddress += (ppuCtrl & 0b0100) ? 0x20 : 0x01;
	}
}


void Ppu::Tick()
{
	if(scanlineV < 240) //visible scanlines
	{
		if(scanlineH && scanlineH <= 256)
		{
			uint8_t spritePixel = 0;
			bool spritePriority = true; //false puts sprite in front of BG
			bool opaqueSprite0 = false;
			if(ppuMask & 0b00010000 && scanlineV) //slV >=1 correct solution?
			{
				for(uint8_t x = 0; x < 8; ++x)
				{
					if(!spriteXpos[x])
					{
						if(!spritePixel && !(scanlineH <= 8 && !(ppuMask & 0b00000100)))
						{
							spritePriority = spriteAttribute[x] & 0b00100000;
							spritePixel = spriteBitmapLow[x] >> 7;
							spritePixel |= (spriteBitmapHigh[x] >> 6) & 0b10;
							if(spritePixel)
							{
								spritePixel |= (spriteAttribute[x] & 0b11) << 2;
								if(!x)
								{
									opaqueSprite0 = sprite0OnCurrent;
								}
							}
						}
						spriteBitmapLow[x] <<= 1;
						spriteBitmapHigh[x] <<= 1;
					}
				}
			}

			uint8_t bgPixel = 0;
			if(ppuMask & 0b00001000 && !(scanlineH <= 8 && !(ppuMask & 0b00000010)))
			{
				bgPixel = (bgLow >> (15 - fineX)) & 1;
				bgPixel |= (bgHigh >> (14 - fineX)) & 2;

				if(bgPixel)
				{
					bgPixel |= (attribute >> (28 - fineX * 2)) & 0b1100;

					if(opaqueSprite0 && scanlineH != 256)
					{
						ppuStatus |= 0b01000000; //sprite 0 hit
					}
				}
			}

			uint8_t pIndex;
			if(spritePixel && (!spritePriority || !bgPixel))
			{
				pIndex = vram[0x3F10 + spritePixel] * 3;
			}
			else
			{
				pIndex = vram[0x3F00 + bgPixel] * 3;
			}

			std::memcpy(&render[renderPos], &palette[pIndex], 3);
			renderPos += 3;
		}

		if(ppuMask & 0b00011000)
		{
			RenderFetches();
			OamScan();
		}
	}
	else if(scanlineV == 241) //vblank scanlines
	{
		if(scanlineH == 1 && !suppressNmiFlag)
		{
			ppuStatus |= 0x80; //VBL nmi
		}
	}
	else if(scanlineV == 261) //prerender scanline
	{
		if(scanlineH == 1)
		{
			ppuStatus &= 0b00011111; //clear sprite overflow, sprite 0 hit and vblank
			suppressNmi = false;
			suppressNmiFlag = false;
		}
		else if(scanlineH >= 280 && scanlineH <= 304 && ppuMask & 0b00011000)
		{
			ppuAddress &= 0x41F;
			ppuAddress |= ppuAddressLatch & 0x7BE0;
		}

		if(ppuMask & 0b00011000)
		{
			RenderFetches();
			if(oddFrame && scanlineH == 339)
			{
				++scanlineH;
			}
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
			renderPos = 0;
		}
	}
}


void Ppu::RenderFetches() //things done during visible and prerender scanlines
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
		ppuAddress &= 0x7BE0;
		ppuAddress |= ppuAddressLatch & 0x41F;
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
					ppuAddress &= 0x7FE0;
					ppuAddress ^= 0x400;
				}
				break;

			//value is determined on the first cycle of each fetch (1,3,5,7)
			case 1: //NT
				if(scanlineH != 1 && scanlineH != 321) 
				{
					bgLow |= bgLowLatch;
					bgHigh |= bgHighLatch;
					attribute |= (attributeLatch & 0b11) * 0x5555; //2-bit splat
				}
				nametable = vram[0x2000 | ppuAddress & nametableMirroring | nametableBase];
				break;
			case 3: //AT
				if(scanlineH != 339)
				{
					uint16_t attributeAddr = 0x23C0 | (ppuAddress & 0xC00);
					attributeAddr |= (ppuAddress >> 4 & 0x38) | (ppuAddress >> 2 & 0b0111);
					attributeLatch = vram[attributeAddr & nametableMirroring | nametableBase];
					attributeLatch >>= (((ppuAddress >> 1) & 1) | ((ppuAddress >> 5) & 0b10)) * 2;
				}
				else
				{
					nametable = vram[0x2000 | ppuAddress & nametableMirroring | nametableBase];
				}
				break;
			case 5: //low
				bgAddress = (nametable << 4) + (ppuAddress >> 12) | ((ppuCtrl & 0x10) << 8);
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
		sprite0OnCurrent = sprite0OnNext;
		oamAddr = 0;

		// sprite fetching
		switch(scanlineH & 7) // do everything in case 7? / all sprites in one for loop
		{
			case 3: spriteAttribute[spriteIndex] = oam2[spriteIndex * 4 + 2]; break;
			case 4: spriteXpos[spriteIndex] = oam2[spriteIndex * 4 + 3];      break;
			case 5:
				if(!(ppuCtrl & 0b00100000))
				{
					bgAddress = ((ppuCtrl & 0b1000) << 9) | (oam2[spriteIndex * 4 + 1] << 4);
				}
				else //8x16 mode
				{
					bgAddress = (oam2[spriteIndex * 4 + 1] << 12) | (oam2[spriteIndex * 4 + 1] << 4);
					bgAddress &= 0x1FE0;
				}

				{
				uint8_t yOffset = scanlineV - oam2[spriteIndex * 4];
				if(spriteAttribute[spriteIndex] & 0b10000000) //flip V
				{
					yOffset = ~yOffset;
				}
				bgAddress |= yOffset & 0b0111;
				if(ppuCtrl & 0b00100000)
				{
					bgAddress += (yOffset & 8) << 1;
				}
				}

				spriteBitmapLow[spriteIndex] = vram[bgAddress];
				if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
				{
					ReverseBits(spriteBitmapLow[spriteIndex]);
				}
				break;
			case 7:
				spriteBitmapHigh[spriteIndex] = vram[bgAddress | 8];
				if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
				{
					ReverseBits(spriteBitmapHigh[spriteIndex]);
				}

				if(oam2[spriteIndex*4] >= 0xEF) // correct solution? sprite 63's Y coord check, does it need to be a range check?
				{
					spriteBitmapLow[spriteIndex] = 0;
					spriteBitmapHigh[spriteIndex] = 0;
				}
				++spriteIndex &= 0b0111;
				break;
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

				if(scanlineV >= oam[oamSpritenum] && scanlineV < oam[oamSpritenum] + 8 + ((ppuCtrl & 0b00100000) >> 2))
				{
					++oamEvalPattern;
					++oam2Index;
					if(!oamSpritenum)
					{
						sprite0OnNext = true;
					}
				}
				else
				{
					if(!oamSpritenum)
					{
						sprite0OnNext = false;
					}
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
				if(scanlineV >= oam[oamSpritenum] && scanlineV < oam[oamSpritenum] + 8 + ((ppuCtrl & 0b00100000) >> 2))
				{
					ppuStatus |= 0b00100000;
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


bool Ppu::PollNmi() const
{
	return (!suppressNmi) ? ppuStatus & ppuCtrl & 0x80 : false;
}


void Ppu::SetNametableMirroring(uint16_t mirroring, uint16_t base)
{
	nametableMirroring = 0x2FFF ^ mirroring;
	nametableBase = base;
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


uint8_t* Ppu::GetVramPtr()
{
	return vram.data();
}


const uint8_t* const Ppu::GetPixelPtr() const
{
	return render.data();
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
