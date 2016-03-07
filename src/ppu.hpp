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

		bool PollNmi();
		void SetNametableMirroring(uint16_t mirroring);

		const bool RenderFrame();
		uint8_t* GetVramPtr();
		const uint8_t* const GetPixelPtr() const;

		uint16_t GetScanlineH();
		uint16_t GetScanlineV();
		const std::array<uint8_t, 0x4000>& GetVram();

	private:
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();
		void ReverseBits(uint8_t &b);

		std::array<uint8_t, 256*240*3> render;

		// const std::array<uint8_t, 64*3> palette //blargg's palette demo in nestopia
		// {{
			 // 84, 84, 84,    0, 30,116,    8, 16,144,   48,  0,136,   68,  0,100,   92,  0, 48,   84,  4,  0,   60, 24,  0,
			 // 32, 42,  0,    8, 58,  0,    0, 64,  0,    0, 60,  0,    0, 50, 60,    0,  0,  0,    0,  0,  0,    0,  0,  0,
			// 152,150,152,    8, 76,196,   48, 50,236,   92, 30,228,  136, 20,176,  160, 20,100,  152, 34, 32,  120, 60,  0,
			 // 84, 90,  0,   40,114,  0,    8,124,  0,    0,118, 40,    0,102,120,    0,  0,  0,    0,  0,  0,    0,  0,  0,
			// 236,238,236,   76,154,236,  120,124,236,  176, 98,236,  228, 84,236,  236, 88,180,  236,106,100,  212,136, 32,
			// 160,170,  0,  116,196,  0,   76,208, 32,   56,204,108,   56,180,204,   60, 60, 60,    0,  0,  0,    0,  0,  0,
			// 236,238,236,  168,204,236,  188,188,236,  212,178,236,  236,174,236,  236,174,212,  236,180,176,  228,196,144,
			// 204,210,120,  180,222,120,  168,226,144,  152,226,180,  160,214,228,  160,162,160,    0,  0,  0,    0,  0,  0
		// }};
		const std::array<uint8_t, 64*3> palette //FBX's "Unsaturated-V5"
		{{
			0x6B,0x6B,0x6B, 0x00,0x1E,0x87, 0x1F,0x0B,0x96, 0x3B,0x0C,0x87, 0x59,0x0D,0x61, 0x5E,0x05,0x28, 0x55,0x11,0x00, 0x46,0x1B,0x00, 
			0x30,0x32,0x00, 0x0A,0x48,0x00, 0x00,0x4E,0x00, 0x00,0x46,0x19, 0x00,0x39,0x5A, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 
			0xB2,0xB2,0xB2, 0x1A,0x53,0xD1, 0x48,0x35,0xEE, 0x71,0x23,0xEC, 0x9A,0x1E,0xB7, 0xA5,0x1E,0x62, 0xA5,0x2D,0x19, 0x87,0x4B,0x00, 
			0x67,0x69,0x00, 0x29,0x84,0x00, 0x03,0x8B,0x00, 0x00,0x82,0x40, 0x00,0x70,0x96, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 
			0xFF,0xFF,0xFF, 0x63,0xAD,0xFD, 0x90,0x8A,0xFE, 0xB9,0x77,0xFC, 0xE7,0x71,0xFE, 0xF7,0x6F,0xC9, 0xF5,0x83,0x6A, 0xDD,0x9C,0x29, 
			0xBD,0xB8,0x07, 0x84,0xD1,0x07, 0x5B,0xDC,0x3B, 0x48,0xD7,0x7D, 0x48,0xC6,0xD8, 0x55,0x55,0x55, 0x00,0x00,0x00, 0x00,0x00,0x00, 
			0xFF,0xFF,0xFF, 0xC4,0xE3,0xFE, 0xD7,0xD5,0xFE, 0xE6,0xCD,0xFE, 0xF9,0xCA,0xFE, 0xFE,0xC9,0xF0, 0xFE,0xD1,0xC7, 0xF7,0xDC,0xAC, 
			0xE8,0xE8,0x9C, 0xD1,0xF2,0x9D, 0xBF,0xF4,0xB1, 0xB7,0xF5,0xCD, 0xB7,0xEB,0xF2, 0xBE,0xBE,0xBE, 0x00,0x00,0x00, 0x00,0x00,0x00
		}};

		std::array<uint8_t, 0x4000> vram;
		std::array<uint8_t, 64*4> oam;
		std::array<uint8_t, 8*4> oam2;

		uint32_t renderPos = 0;

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
		uint8_t ppuDataLatch;
		uint16_t nametableMirroring;
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

		bool oldNmi = false;
};
