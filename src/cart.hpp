#pragma once

#include <map>

#include "ppu.hpp"


enum System : uint8_t {Famicom = 0, Ntsc = 0, Pal = 1};
enum Type : uint16_t {NES_NROM = 0, NES_UNROM = 2, NES_CNROM = 3, NES_AOROM = 7, KONAMI_VRC_4 = 21};
enum NametableLayout : uint8_t {vertical = 1, horizontal = 2, single = 3};
enum Extra : uint8_t {none = 0, VRC4e = 1};
struct cartAttributes
{
	System system;
	Type type;
	uint16_t prg;
	uint16_t chr;
	bool isChrRam;
	uint16_t wram;
	bool wramBattery;
	NametableLayout nametableLayout;
    Extra extra;
};


struct Cart
{
    public:
        Cart(const std::string inFile, std::vector<uint8_t> &prgRom, std::vector<uint8_t> &prgRam);

        std::array<uint8_t, 16> header;
        uint8_t mapper = 0xFF;
        Type type;

        std::array<uint8_t*, 4> pPrgBank;
        std::array<uint8_t*, 4> pPrgRamBank;

        std::vector<uint8_t> chrMem;
        bool isChrRam;

        std::array<NametableOffset, 4> nametableOffsets;

    private:
        void SetDefaultPrgBanksSha(std::vector<uint8_t> &prgRom, cartAttributes attr);
        void SetDefaultPrgBanks(std::vector<uint8_t> &prgRom);
        void SetChrMem(const std::vector<uint8_t> &fileContent);
        void SetDefaultNametableLayout();

        //can't map multiple keys to the same value
        const std::map<std::array<uint32_t, 5>, cartAttributes> cartInfo
        {
            //0
            {{0x22D57AC6, 0x066529D1, 0x99FCD299, 0x159D9482, 0x0042C7D0}, {Ntsc   , NES_NROM    , 16 , 8  , 0, 0, 0, vertical  , none}},  //ice climber
            //2
            {{0x979494E7, 0x869AC7AB, 0x4815FDBD, 0x1DC99F89, 0x3F713FBF}, {Ntsc   , NES_UNROM   , 128, 8  , 1, 0, 0, horizontal, none}},  //contra
            //3
            {{0x5AFD9664, 0x716116B4, 0x98354B40, 0x48D743D9, 0x8540BF79}, {Ntsc   , NES_CNROM   , 32 , 32 , 0, 0, 0, horizontal, none}},  //wood & water rage
            //7
            {{0xD85C9FF4, 0x89672534, 0xFBF61A15, 0xF8FA56FF, 0xF489A34B}, {Ntsc   , NES_AOROM   , 256, 8  , 1, 0, 0, single    , none}},  //bt
            {{0xA1456332, 0x5B0F33C3, 0x58142E73, 0x63D31614, 0x722FDDB1}, {Ntsc   , NES_AOROM   , 256, 8  , 1, 0, 0, single    , none}},  //bt & dd
            //21
            {{0xF08D61CF, 0x09B6794B, 0xA7E642AC, 0x50161449, 0x3753BA0F}, {Famicom, KONAMI_VRC_4, 128, 128, 0, 2, 0, horizontal, VRC4e}}, //crisis force
        };
};
