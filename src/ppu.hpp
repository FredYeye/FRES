#pragma once

#include <cstdint>
#include <array>
#include <vector>

enum NametableOffset : uint16_t {A = 0, B = 0x400, C = 0x800, D = 0xC00};

class Ppu
{
	public:
		Ppu();

		void CtrlWrite(uint8_t dataBus);    //2000
		void MaskWrite(uint8_t dataBus);    //2001
		uint8_t StatusRead();               //2002
		void OamAddrWrite(uint8_t dataBus); //2003
		const uint8_t OamDataRead() const;  //2004
		void OamDataWrite(uint8_t dataBus); //2004
		void ScrollWrite(uint8_t dataBus);  //2005
		void AddrWrite(uint8_t dataBus);    //2006
		uint8_t DataRead();                 //2007
		void DataWrite(uint8_t dataBus);    //2007

		void Tick();

		const bool PollNmi() const;

		const bool RenderFrame();
		const uint32_t* const GetPixelPtr() const;

		const uint16_t GetScanlineH() const;
		const uint16_t GetScanlineV() const;

		void SetNametableArrangement(const std::array<NametableOffset, 4> &offset);
		void SetPatternBanks1(const uint8_t bank, const uint16_t offset);
		void SetPatternBanks2(const uint8_t bank, const uint8_t offset);
		void SetPatternBanks4(const bool bank, const uint8_t offset);
		void SetPatternBanks8(const uint8_t offset);
		void SetPattern(std::vector<uint8_t> &chr);
		void SetChrType(bool type);

		bool renderFrame = false;


		bool GetA12();

	private:
		void VisibleScanlines();
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();

		void YIncrement();
		void CoarseXIncrement();

		void ReverseBits(uint8_t &b) const;

		std::array<uint32_t, 256*240> render;

		const std::array<uint32_t, 64> palette //PVM Style D93 (FBX) http://www.firebrandx.com/nespalette.html
		{{
			0xFF636B69, 0xFF741700, 0xFF87001E, 0xFF730034, 0xFF570056, 0xFF13005E, 0xFF001A53, 0xFF00243B,
			0xFF003024, 0xFF003A06, 0xFF003F00, 0xFF1E3B00, 0xFF4E3300, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFB3BBB9, 0xFFB95314, 0xFFDA2C4D, 0xFFDE1E67, 0xFF9C1898, 0xFF44239D, 0xFF003EA0, 0xFF00558D,
			0xFF006D65, 0xFF00792C, 0xFF008100, 0xFF427D00, 0xFF8A7800, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFFA869, 0xFFFF9196, 0xFFFA8AB2, 0xFFFA7DEA, 0xFFC77BF3, 0xFF598EF2, 0xFF27ADE6,
			0xFF05C8D7, 0xFF07DF90, 0xFF3CE564, 0xFF7DE245, 0xFFD9D548, 0xFF48504E, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFFEAD2, 0xFFFFE2E2, 0xFFFFD8E9, 0xFFFFD2F5, 0xFFEAD9F8, 0xFFB9DEFA, 0xFF9BE8F9,
			0xFF8CF2F3, 0xFF91FAD3, 0xFFA8FCB8, 0xFFCAFAAE, 0xFFF3F3CA, 0xFFB8C0BE, 0xFF000000, 0xFF000000,
		}};

		std::array<uint8_t*, 8> pPattern;
		std::array<uint8_t*, 4> pNametable;
		std::vector<uint8_t> pattern;
		std::array<uint8_t, 0x1000> nametable; //alt. vector

		std::array<uint8_t, 32> paletteIndices;
		uint32_t emphasisMask = 0xFFFFFFFF;
		uint8_t grayscaleMask = 0xFF;

		std::array<uint8_t, 64*4> oam;
		std::array<uint8_t, 8*4> oam2;

		uint16_t renderPos = 0;

		uint8_t ppuCtrl = 0;
		uint8_t ppuMask = 0;
		uint8_t ppuStatus = 0;
		uint8_t oamAddr = 0;

		uint16_t scanlineH = 340, scanlineV = 261;
		uint16_t ppuAddress, ppuAddressLatch; //v,t rename?

		uint16_t ppuAddressBus;
		uint8_t nametableA; //rename
		uint32_t attribute;
		uint8_t attributeLatch;
		uint16_t bgLow, bgHigh;
		uint8_t bgLowLatch, bgHighLatch;

		bool wToggle = false;

		bool oddFrame = false;
		uint8_t fineX = 0;
		uint8_t ppuDataLatch = 0;
		uint8_t oam2Index = 0;
		uint8_t oamEvalPattern = 0;
		uint8_t oamSpritenum = 0; //0-3 = sprite0, 4-7 = sprite1 [...] 252-255 = sprite63
		uint8_t oamDiagonal = 0;

		bool suppressNmi = false;
		uint8_t nmiFlag = 0x80;

		std::array<uint8_t, 8> spriteBitmapLow;
		std::array<uint8_t, 8> spriteBitmapHigh;
		std::array<uint8_t, 8> spriteAttribute;
		std::array<uint8_t, 8> spriteXpos;
		uint8_t spriteIndex = 0;

		bool sprite0OnNext = false;
		bool sprite0OnCurrent = false;

		bool isChrRam;

		uint8_t TToVDelay = 0;
};
