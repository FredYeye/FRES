#pragma once

#include <cstdint>
#include <array>


class ppu
{
	public:
		ppu();
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

		const bool RenderFrame();
		std::array<uint8_t, 0x4000>::iterator VramIterator();
		const std::array<uint8_t, 256*240*3>* const GetPixelPtr() const;

		uint16_t GetScanlineH();
		uint16_t GetScanlineV();
		const std::array<uint8_t, 0x4000>& GetVram();

	private:
		void RenderFetches();
		void OamScan();
		void OamUpdateIndex();
		void ReverseBits(uint8_t &b);

		std::array<uint8_t, 256*240*3> render;
		const std::array<uint8_t, 64*3> palette
		{{
			 84, 84, 84,    0, 30,116,    8, 16,144,   48,  0,136,   68,  0,100,   92,  0, 48,   84,  4,  0,   60, 24,  0,
			 32, 42,  0,    8, 58,  0,    0, 64,  0,    0, 60,  0,    0, 50, 60,    0,  0,  0,    0,  0,  0,    0,  0,  0,
			152,150,152,    8, 76,196,   48, 50,236,   92, 30,228,  136, 20,176,  160, 20,100,  152, 34, 32,  120, 60,  0,
			 84, 90,  0,   40,114,  0,    8,124,  0,    0,118, 40,    0,102,120,    0,  0,  0,    0,  0,  0,    0,  0,  0,
			236,238,236,   76,154,236,  120,124,236,  176, 98,236,  228, 84,236,  236, 88,180,  236,106,100,  212,136, 32,
			160,170,  0,  116,196,  0,   76,208, 32,   56,204,108,   56,180,204,   60, 60, 60,    0,  0,  0,    0,  0,  0,
			236,238,236,  168,204,236,  188,188,236,  212,178,236,  236,174,236,  236,174,212,  236,180,176,  228,196,144,
			204,210,120,  180,222,120,  168,226,144,  152,226,180,  160,214,228,  160,162,160,    0,  0,  0,    0,  0,  0
		}};

		std::array<uint8_t, 0x4000> vram;
		std::array<uint8_t, 64*4> oam;
		std::array<uint8_t, 8*4> oam2;

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
		uint16_t ppuNtMirroring;
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
