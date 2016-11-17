#pragma once

#include <cstdint>
#include <array>


class Ppu
{
	public:
		Ppu();
		uint8_t StatusRead();
		uint8_t OamDataRead();
		uint8_t DataRead();

		void CtrlWrite(uint8_t dataBus);
		void MaskWrite(uint8_t dataBus);
		void OamAddrWrite(uint8_t dataBus);
		void OamDataWrite(uint8_t dataBus);
		void ScrollWrite(uint8_t dataBus);
		void AddrWrite(uint8_t dataBus);
		void DataWrite(uint8_t dataBus);

		void Tick();

		bool PollNmi() const;
		void SetNametableMirroring(uint16_t mirroring, uint16_t base);

		const bool RenderFrame();
		uint8_t* GetVramPtr();
		const uint32_t* const GetPixelPtr() const;

		uint16_t GetScanlineH();
		uint16_t GetScanlineV();
		const std::array<uint8_t, 0x4000>& GetVram();

	private:
		void VisibleScanlines();
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();
		void ReverseBits(uint8_t &b);

		std::array<uint32_t, 256*240> render;

		const std::array<uint32_t, 64> palette //FirebrandX's "Unsaturated-Final" http://www.firebrandx.com/nespalette.html
		{{
		0xFF676767, 0xFF8E1F00, 0xFF9E0623, 0xFF8E0040, 0xFF670060, 0xFF1C0067, 0xFF00105B, 0xFF002543,
		0xFF003431, 0xFF004807, 0xFF004F00, 0xFF224600, 0xFF613A00, 0xFF000000, 0xFF000000, 0xFF000000,
		0xFFB3B3B3, 0xFFDF5A20, 0xFFFB3851, 0xFFEE277A, 0xFFC220A5, 0xFF6B22B0, 0xFF0237AD, 0xFF00568D,
		0xFF00706E, 0xFF008A2E, 0xFF009206, 0xFF478A00, 0xFF9B7B03, 0xFF101010, 0xFF000000, 0xFF000000,
		0xFFFFFFFF, 0xFFFFAE62, 0xFFFF8B91, 0xFFFF78BC, 0xFFFF6EE9, 0xFFCD6CFC, 0xFF6782FA, 0xFF269BE2,
		0xFF01B9C0, 0xFF00D284, 0xFF38DE58, 0xFF7DD946, 0xFFD2CE49, 0xFF494949, 0xFF000000, 0xFF000000,
		0xFFFFFFFF, 0xFFFFE3C1, 0xFFFFD4D5, 0xFFFFCCE7, 0xFFFFC9FB, 0xFFF0C7FF, 0xFFC5D0FF, 0xFFAADAF8,
		0xFF9AE6EB, 0xFF9AF1D1, 0xFFAFF7BE, 0xFFCDF4B6, 0xFFEFF0B7, 0xFFB2B2B2, 0xFF000000, 0xFF000000
		}};

		std::array<uint8_t, 0x4000> vram;
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
		uint8_t nametable;
		uint32_t attribute;
		uint8_t attributeLatch;
		uint16_t bgLow, bgHigh;
		uint8_t bgLowLatch, bgHighLatch;

		bool wToggle = false;

		bool oddFrame = false;
		uint8_t fineX = 0;
		uint8_t ppuDataLatch = 0;
		uint16_t nametableMirroring, nametableBase;
		uint8_t oam2Index = 0;
		uint8_t oamEvalPattern = 0;
		uint8_t oamSpritenum = 0; //0-3 = sprite0, 4-7 = sprite1 [...] 252-255 = sprite63

		bool renderFrame = false;
		bool suppressNmi = false;

		std::array<uint8_t, 8> spriteBitmapLow;
		std::array<uint8_t, 8> spriteBitmapHigh;
		std::array<uint8_t, 8> spriteAttribute;
		std::array<uint8_t, 8> spriteXpos;
		uint8_t spriteIndex = 0;

		bool sprite0OnNext = false;
		bool sprite0OnCurrent = false;
};
