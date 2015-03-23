#pragma once

#include <array>
#include <bitset>
#include <iostream>


class nes
{
	public:
		nes(std::string inFile);
		void AdvanceFrame(uint8_t input);
		const std::array<uint8_t, 256*240*3>* const GetPixelPtr() const;

	private:
		void LoadRom(std::string inFile);
		void runOpcode();
		void cpuRead();
		void cpuWrite();
		void cpu_tick();
		void cpu_op_done();
		void ppu_tick();
		void ppu_render_fetches();
		void ppu_oam_scan();
		void ppu_oam_update_index();

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

		std::array<uint8_t, 0x10000> cpuMem;
		std::array<uint8_t, 0x4000> vram;
		std::array<uint8_t, 0x100> ppu_oam;
		std::array<uint8_t, 0x20> ppu_secondary_oam;

		uint16_t PC;
		uint8_t rA = 0, rX = 0, rY = 0;
		uint8_t rS = 0;
		std::bitset<8> rP; //0:C | 1:Z | 2:I | 3:D | 4:B | 5:1 | 6:V | 7:N

		uint16_t addressBus;
		uint8_t dataBus;
		uint8_t controller_reg;
		bool controller_update = false;

		uint8_t fine_x = 0;
		uint8_t ppuctrl = 0, ppumask = 0, ppustatus = 0, oamaddr = 0;

		uint16_t scanline_h = 0, scanline_v = 0;
		uint16_t ppu_address, ppu_address_latch;
		uint16_t ppu_bg_low, ppu_bg_high;

		bool nmi_line = false;
		bool w_toggle = false;

		bool ppu_odd_frame = false;
		uint8_t ppu_bg_low_latch, ppu_bg_high_latch;
		uint8_t ppu_nametable;
		uint8_t ppu_attribute_latch;
		uint32_t ppu_attribute;
		uint16_t ppu_bg_address;
		uint8_t ppudata_latch;
		uint16_t ppu_nt_mirroring;
		uint8_t secondary_oam_index = 0;
		uint8_t oam_eval_pattern = 0;
		uint8_t oam_spritenum = 0;
		bool oam_block_writes = false;

		bool renderFrame;
};
