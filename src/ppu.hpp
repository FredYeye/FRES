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
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();
		void ReverseBits(uint8_t &b);

		std::array<uint32_t, 256*240> render;

		const std::array<uint32_t, 64> palette //FirebrandX's "Unsaturated-V6"
		{{
			0xFF6B6B6B, 0xFF871E00, 0xFF960B1F, 0xFF870C3B, 0xFF610D59, 0xFF28055E, 0xFF001155, 0xFF001B46,
			0xFF003230, 0xFF00480A, 0xFF004E00, 0xFF194600, 0xFF583A00, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFB2B2B2, 0xFFD1531A, 0xFFEE3548, 0xFFEC2371, 0xFFB71E9A, 0xFF621EA5, 0xFF192DA5, 0xFF004B87,
			0xFF006967, 0xFF008429, 0xFF008B03, 0xFF408200, 0xFF917800, 0xFF000000, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFDAD63, 0xFFFE8A90, 0xFFFC77B9, 0xFFFE71E7, 0xFFC96FF7, 0xFF6A83F5, 0xFF299CDD,
			0xFF07B8BD, 0xFF07D184, 0xFF3BDC5B, 0xFF7DD748, 0xFFCECC48, 0xFF555555, 0xFF000000, 0xFF000000,
			0xFFFFFFFF, 0xFFFEE3C4, 0xFFFED5D7, 0xFFFECDE6, 0xFFFECAF9, 0xFFF0C9FE, 0xFFC7D1FE, 0xFFACDCF7,
			0xFF9CE8E8, 0xFF9DF2D1, 0xFFB1F4BF, 0xFFCDF5B7, 0xFFEEF0B7, 0xFFBEBEBE, 0xFF000000, 0xFF000000
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
		bool suppressNmiFlag = false;

		std::array<uint8_t, 8> spriteBitmapLow;
		std::array<uint8_t, 8> spriteBitmapHigh;
		std::array<uint8_t, 8> spriteAttribute;
		std::array<uint8_t, 8> spriteXpos;
		uint8_t spriteIndex = 0;

		bool sprite0OnNext = false;
		bool sprite0OnCurrent = false;
};
