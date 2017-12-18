#include <cstring>
#include <iostream>

#include "ppu.hpp"


Ppu::Ppu()
{
	oam.fill(0xFF);
}


void Ppu::CtrlWrite(uint8_t dataBus) //2000
{
	ppuCtrl = dataBus;
	ppuAddressLatch &= 0x73FF;
	ppuAddressLatch |= (dataBus & 0b11) << 10;
}


void Ppu::MaskWrite(uint8_t dataBus) //2001
{
	// bg & spr masks take 1 cycle to apply?
	ppuMask = dataBus;

	grayscaleMask = (dataBus & 1) ? 0x30: 0xFF;

	emphasisMask = 0xFFFFFFFF; //crude implementation of color desaturation, actually attenuates signal by ~25% or something
	if(dataBus & 0b00100000) emphasisMask &= 0xFFC0C0FF; //red
	if(dataBus & 0b01000000) emphasisMask &= 0xFFC0FFC0; //green
	if(dataBus & 0b10000000) emphasisMask &= 0xFFFFC0C0; //blue
}


uint8_t Ppu::StatusRead() //2002
{
	if(scanlineV == 241 && scanlineH < 2) //does dot 2 really suppress?
	{
		nmiFlag = scanlineH << 7; //0=prevent flag from getting set, 1=set flag
		suppressNmi = true; //nmis can't happen regardless

		//dot      0: block NMI flag from getting set this frame
		//dots   1-2: suppress NMIs for this frame
		//dots 340,3: behaves normally
	}
	// else if(scanlineV == 261 && scanlineH == 0) ppuStatus &= 0b10011111; //makes spr overflow and hit timing tests pass

	const uint8_t status = ppuStatus;
	ppuStatus &= 0x7F; //clear nmi_occurred bit
	wToggle = false;

	return status;
}


void Ppu::OamAddrWrite(uint8_t dataBus) //2003
{
	oamAddr = dataBus;
}


const uint8_t Ppu::OamDataRead() const //2004
{
	//do more stuff (reads while sprite eval is running)
	if((oamAddr & 0b11) == 0b10) //mask out unimplemented bits in attribyte byte
	{
		return oam[oamAddr] & 0b11100011;
	}
	return oam[oamAddr];
}


void Ppu::OamDataWrite(uint8_t dataBus) //2004
{
	// todo:
	// Writes to OAMDATA during rendering (on the pre-render line and the visible lines 0-239, provided either sprite or
	// background rendering is enabled) do not modify values in OAM, but do perform a glitchy increment of OAMADDR.
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
		TToVDelay = 0b00000100; //T->V copy should be delayed by 3 clocks. check if this impl is the correct amount later
	}
	wToggle = !wToggle;
}


uint8_t Ppu::DataRead() //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
	{
		const uint8_t currentLatch = (ppuAddress >= 0x3F00) ? paletteIndices[ppuAddress & 0x1F] & grayscaleMask : ppuDataLatch;

		if(ppuAddress >= 0x2000)
		{
			ppuDataLatch = pNametable[(ppuAddress >> 10) & 0b11][ppuAddress & 0x3FF];
		}
		else
		{
			ppuDataLatch = pPattern[ppuAddress >> 10][ppuAddress & 0x3FF];
		}
		ppuAddress += (ppuCtrl & 0b0100) ? 0x20 : 0x01;
		return currentLatch;
	}
	else
	{
		if(scanlineH != 256)
		{
			YIncrement();
		}
		if(scanlineH >= 1 && scanlineH <= 256 || scanlineH >= 321)
		{
			if(scanlineH & 0b111 != 0)
			{
				CoarseXIncrement();
			}
		}
	}

	return 0;
}


void Ppu::DataWrite(uint8_t dataBus) //2007
{
	if((scanlineV >= 240 && scanlineV <= 260) || !(ppuMask & 0x18))
	{
		if(ppuAddress < 0x2000)
		{
			if(isChrRam == 1)
			{
				pPattern[ppuAddress >> 10][ppuAddress & 0x3FF] = dataBus;
			}
		}
		else if(ppuAddress < 0x3F00)
		{
			pNametable[(ppuAddress >> 10) & 0b11][ppuAddress & 0x3FF] = dataBus;
		}
		else
		{
			paletteIndices[ppuAddress & 0x1F] = dataBus & 0x3F;
			if(!(ppuAddress & 0b11)) //sprite palette mirror write
			{
				paletteIndices[(ppuAddress & 0x1F) ^ 0x10] = dataBus & 0x3F;
			}
		}
		ppuAddress += (ppuCtrl & 0b0100) ? 0x20 : 0x01;
	}
	else
	{
		if(scanlineH != 256)
		{
			YIncrement();
		}
		if(scanlineH >= 1 && scanlineH <= 256 || scanlineH >= 321)
		{
			if(scanlineH & 0b111 != 0)
			{
				CoarseXIncrement();
			}
		}
	}
}


void Ppu::Tick()
{
	if(++scanlineH == 341)
	{
		scanlineH = 0;
		if(++scanlineV == 262)
		{
			scanlineV = 0;
			oddFrame = !oddFrame;
			renderFrame = true;
			renderPos = 0;
		}
		return; //idle at dot 0
	}

	if(scanlineV < 240) //visible scanlines
	{
		VisibleScanlines();
	}
	else if(scanlineV == 241 && scanlineH == 1) //vblank scanlines
	{
		ppuStatus |= nmiFlag; //VBL nmi
		nmiFlag = 0x80; //restore flag if it was prevented from getting set
	}
	else if(scanlineV == 261) //prerender scanline
	{
		if(scanlineH == 1)
		{
			ppuStatus &= 0b00011111; //clear sprite overflow, sprite 0 hit and vblank
			suppressNmi = false;
		}
		else if(scanlineH >= 280 && scanlineH <= 304 && ppuMask & 0b00011000)
		{
			ppuAddress &= 0x41F;
			ppuAddress |= ppuAddressLatch & 0x7BE0;
		}

		if(ppuMask & 0b00011000)
		{
			RenderFetches();
			if(oddFrame && scanlineH == 339) //338 passes ppu_vbl_nmi 10
			{
				++scanlineH;
			}
		}
	}

	TToVDelay >>= 1; //place this before the dot 0 skip?
	if(TToVDelay & 1)
	{
		ppuAddress = ppuAddressLatch;
	}
}


void Ppu::VisibleScanlines()
{
	if(scanlineH <= 256)
	{
		uint8_t spritePixel = 0;
		bool spritePriority; //false puts sprite in front of BG
		bool opaqueSprite0 = false;
		if(ppuMask & 0b00010000)
		{
			for(uint8_t x = 0; x < 8; ++x)
			{
				if(!spriteXpos[x])
				{
					if(!(scanlineH <= 8 && !(ppuMask & 0b00000100)))
					{
						spritePixel = spriteBitmapLow[x] >> 7;
						spritePixel |= (spriteBitmapHigh[x] >> 6) & 0b10;
						if(spritePixel)
						{
							spritePixel |= (spriteAttribute[x] & 0b11) << 2;
							spritePriority = spriteAttribute[x] & 0b00100000;
							if(!x)
							{
								opaqueSprite0 = sprite0OnCurrent;
							}
							break;
						}
					}
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
					ppuStatus = 0b01000000; //sprite 0 hit
				}
			}
		}

		uint8_t pIndex;
		if(spritePixel && (!spritePriority || !bgPixel))
		{
			pIndex = paletteIndices[0x10 + spritePixel];
		}
		else
		{
			if(ppuMask & 0b00011000)
			{
				pIndex = paletteIndices[bgPixel];
			}
			else //rendering is turned off
			{
				pIndex = (ppuAddress >= 0x3F00) ? paletteIndices[ppuAddress & 0x1F] : paletteIndices[0];
			}
		}

		render[renderPos++] = palette[pIndex & grayscaleMask] & emphasisMask;
	}

	if(ppuMask & 0b00011000)
	{
		RenderFetches();
		OamScan();
	}
}


void Ppu::RenderFetches() //things done during visible and prerender scanlines
{
	if(scanlineH <= 256 || scanlineH >= 321)
	{
		if(scanlineH == 256)
		{
			YIncrement();
		}

		switch(scanlineH & 0x07)
		{
			case 0: CoarseXIncrement(); break;

			//value is determined on the first cycle of each fetch (1,3,5,7)
			case 1: //NT
				if(scanlineH != 1 && scanlineH != 321) 
				{
					bgLow |= bgLowLatch;
					bgHigh |= bgHighLatch;
					attribute |= (attributeLatch & 0b11) * 0x5555; //2-bit splat
				}
				nametableA = pNametable[(ppuAddress >> 10) & 0b11][ppuAddress & 0x3FF];
			break;

			case 3: //AT
				if(scanlineH != 339)
				{
					uint16_t attributeAddr = 0x23C0 | (ppuAddress & 0xC00);
					attributeAddr |= (ppuAddress >> 4 & 0x38) | (ppuAddress >> 2 & 0b0111);
					attributeLatch = pNametable[(attributeAddr >> 10) & 0b11][attributeAddr & 0x3FF];
					attributeLatch >>= (((ppuAddress >> 1) & 1) | ((ppuAddress >> 5) & 0b10)) * 2;
				}
				else
				{
					nametableA = pNametable[(ppuAddress >> 10) & 0b11][ppuAddress & 0x3FF];
				}
			break;

			case 5: //low
				bgAddress = (nametableA << 4) + (ppuAddress >> 12) | ((ppuCtrl & 0x10) << 8);
				bgLowLatch = pPattern[(bgAddress >> 10) & 7][bgAddress & 0x3FF];
			break;

			case 7: //high
				bgHighLatch = pPattern[(bgAddress >> 10) & 7][bgAddress + 8 & 0x3FF];
			break;
		}

		if(scanlineH <= 256)
		{
			for(uint8_t x = 0; x < 8; ++x)
			{
				if(spriteXpos[x])
				{
					--spriteXpos[x];
				}
				else
				{
					spriteBitmapLow[x] <<= 1;
					spriteBitmapHigh[x] <<= 1;
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
		if(scanlineH == 257)
		{
			ppuAddress &= 0x7BE0;
			ppuAddress |= ppuAddressLatch & 0x41F;
		}

		sprite0OnCurrent = sprite0OnNext;
		oamAddr = 0;

		// sprite fetching
		switch(scanlineH & 7)
		{
			case 3: spriteAttribute[spriteIndex] = oam2[spriteIndex * 4 + 2]; break;
			case 4: spriteXpos[spriteIndex] = oam2[spriteIndex * 4 + 3];      break;
			case 5:
				// spriteAttribute[spriteIndex] = oam2[spriteIndex * 4 + 2]; //test this. should make no diff as oam2 can't be changed here
				// spriteXpos[spriteIndex] = oam2[spriteIndex * 4 + 3]; //just add a note about correct impl

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

				spriteBitmapLow[spriteIndex] = pPattern[(bgAddress >> 10) & 7][bgAddress & 0x3FF];
				if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
				{
					ReverseBits(spriteBitmapLow[spriteIndex]);
				}
			break;

			case 7:
				spriteBitmapHigh[spriteIndex] = pPattern[(bgAddress >> 10) & 7][(bgAddress | 8) & 0x3FF];
				if(spriteAttribute[spriteIndex] & 0b01000000) //flip H
				{
					ReverseBits(spriteBitmapHigh[spriteIndex]);
				}

				if(uint16_t(scanlineV - oam2[spriteIndex * 4]) >= 8 + ((ppuCtrl & 0b00100000) >> 2)) //prevent copying if Y coord is out of range
				{
					spriteBitmapLow[spriteIndex] = 0;
					spriteBitmapHigh[spriteIndex] = 0;
				}

				++spriteIndex &= 0b0111;
			break;
		}
	}
}


void Ppu::OamScan() //dots 1-256
{
	if((scanlineH & 1) == 0)
	{
		if(scanlineH <= 64)
		{
			oam2[(scanlineH >> 1) - 1] = 0xFF;
		}
		else if(scanlineH <= 256)
		{
			if(oamEvalPattern == 0) //search for sprites in range
			{
				oam2[oam2Index] = oam[oamSpritenum]; //oam should start searching at oam[oamAddr]

				const uint8_t spriteHeight = 8 + ((ppuCtrl & 0b00100000) >> 2);
				if(uint16_t(scanlineV - oam[oamSpritenum]) < spriteHeight) //if current scanline is on a sprite
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
				// oamSpritenum += 4; //do nothing
			}
			else //search for overflow
			{
				if(scanlineV >= oam[oamSpritenum + oamDiagonal] && scanlineV < oam[oamSpritenum + oamDiagonal] + 8 + ((ppuCtrl & 0b00100000) >> 2))
				{
					ppuStatus |= 0b00100000;
					oamEvalPattern = 4;
				}
				oamSpritenum += 4;
				++oamDiagonal &= 0b11; //HW bug: search oam "diagonally" by adding 0-3 per search

				if(oamSpritenum == 0) //stop searching
				{
					oamEvalPattern = 4;
				}
			}

			if(scanlineH == 256)
			{
				oam2Index = 0;
				oamSpritenum = 0;
				oamEvalPattern = 0;
				oamDiagonal = 0;
			}
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
	else if(oam2Index < 32) //search for next sprite
	{
		oamEvalPattern = 0;
	}
	else if(oam2Index == 32) //8 sprites found
	{
		oamEvalPattern = 5;
	}
}


void Ppu::YIncrement()
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
			ppuAddress ^= 0x3A0 | 0x800; //clear coarse Y (0x3A0) and flip nametable (0x800)
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


void Ppu::CoarseXIncrement()
{
	if((ppuAddress & 0x1F) != 0x1F)
	{
		++ppuAddress;
	}
	else
	{
		ppuAddress ^= 0x1F | 0x400; //clear coarse X (0x1F) and flip nametable (0x400)
	}
}


const bool Ppu::PollNmi() const
{
	return !suppressNmi && ppuStatus & ppuCtrl & 0x80;
}


void Ppu::ReverseBits(uint8_t &b) const
{
	b = b >> 4 | b << 4;
	b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
	b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
}


const uint32_t* const Ppu::GetPixelPtr() const
{
	return render.data();
}


const uint16_t Ppu::GetScanlineH() const
{
	return scanlineH;
}


const uint16_t Ppu::GetScanlineV() const
{
	return scanlineV;
}


void Ppu::SetNametableMirroring(const std::array<NametableOffset, 4> &offset)
{
	for(int x = 0; x < 4; x++)
	{
		pNametable[x] = nametable.data() + offset[x];
	}
}


void Ppu::SetPatternBanks1(const uint8_t bank, const uint16_t offset)
{
	pPattern[bank] = pattern.data() + (offset << 10);
}


void Ppu::SetPatternBanks2(const uint8_t bank, const uint8_t offset)
{
	for(int x = 0; x < 2; x++)
	{
		pPattern[(bank << 1) + x] = &pattern[(offset << 11) + 0x400 * x];
	}
}


void Ppu::SetPatternBanks4(const bool bank, const uint8_t offset)
{
	for(int x = 0; x < 4; x++)
	{
		pPattern[(bank << 2) + x] = &pattern[(offset << 12) + 0x400 * x];
	}
}


void Ppu::SetPatternBanks8(const uint8_t offset)
{
	for(int x = 0; x < 8; x++)
	{
		pPattern[x] = &pattern[(offset << 13) + 0x400 * x];
	}
}


void Ppu::SetPattern(std::vector<uint8_t> &chr) //better name for this function?
{
	pattern = chr;
	for(int x = 0; x < 8; x++)
	{
		pPattern[x] = &pattern[0x400 * x];
	}
}


void Ppu::SetChrType(bool type)
{
	isChrRam = type;
}
