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

		uint16_t GetScanlineH() const;
		uint16_t GetScanlineV() const;

		void SetNametableMirroring(const std::array<NametableOffset, 4> &offset);
		void SetPatternBank(const uint8_t bank, const uint16_t offset);
		void SetPatternBanks4(const bool bank, const uint8_t offset);
		void SetPatternBanks8(const uint8_t offset);
		void SetPattern(std::vector<uint8_t> &chr);

	private:
		void VisibleScanlines();
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();

		void YIncrement();
		void CoarseXIncrement();

		void ReverseBits(uint8_t &b) const;

		std::array<uint32_t, 256*240> render;

		const std::array<uint32_t, 64> palette //Finalized Nostalgia (FBX) http://www.firebrandx.com/nespalette.html
		{{
			0xFF656565, 0xFF7D1200, 0xFF8E0018, 0xFF820036, 0xFF5D0056, 0xFF18005A, 0xFF00054F, 0xFF001938,
			0xFF00311D, 0xFF003D00, 0xFF004100, 0xFF173B00, 0xFF552E00, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFAFAFAF, 0xFFC84E19, 0xFFE32F47, 0xFFD71F6B, 0xFFAE1B93, 0xFF5E1A9E, 0xFF003297, 0xFF004B7B,
			0xFF00675B, 0xFF007A26, 0xFF008200, 0xFF3E7A00, 0xFF8A6E00, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFFA964, 0xFFFF898E, 0xFFFF76B6, 0xFFFF6FE0, 0xFFC46CEF, 0xFF6A80F0, 0xFF2C98D8,
			0xFF0AB4B9, 0xFF0CCB83, 0xFF3FD65B, 0xFF7ED14A, 0xFFCBC74D, 0xFF4C4C4C, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFFE5C7, 0xFFFFD9D9, 0xFFFFD1E9, 0xFFFFCEF9, 0xFFF1CCFF, 0xFFCBD4FF, 0xFFB1DFF8,
			0xFFA4EAED, 0xFFA4F4D6, 0xFFB8F8C5, 0xFFD3F6BE, 0xFFF1F1BF, 0xFFB9B9B9, 0xFF000000, 0xFF000000
		}};

		std::vector<uint8_t> pattern;
		std::array<uint8_t, 0x1000> nametable; //alt. vector
		std::array<uint8_t, 32> paletteIndices;
		std::array<uint8_t*, 8> pPattern;
		std::array<uint8_t*, 4> pNametable;
		std::array<uint8_t, 64*4> oam;
		std::array<uint8_t, 8*4> oam2;

		uint16_t renderPos = 0;

		uint8_t ppuCtrl = 0;
		uint8_t ppuMask = 0;
		uint8_t ppuStatus = 0;
		uint8_t oamAddr = 0;

		uint16_t scanlineH = 0, scanlineV = 0;
		uint16_t ppuAddress, ppuAddressLatch;

		uint16_t bgAddress;
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

		bool renderFrame = false;
		bool suppressNmi = false;

		std::array<uint8_t, 8> spriteBitmapLow;
		std::array<uint8_t, 8> spriteBitmapHigh;
		std::array<uint8_t, 8> spriteAttribute;
		std::array<uint8_t, 8> spriteXpos;
		uint8_t spriteIndex = 0;

		uint8_t spriteHit = 0;

		bool sprite0OnNext = false;
		bool sprite0OnCurrent = false;
};
