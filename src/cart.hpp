#pragma once

#include <map>

#include "ppu.hpp"


enum System : uint8_t {Famicom = 0, Ntsc = 0, Pal = 1};
enum Type : uint16_t {NES_NROM = 0, NES_SFROM = 1, NES_SGROM = 1, NES_SKROM = 1, NES_SLROM = 1, NES_SNROM = 1, NES_UNROM = 2, NES_CNROM = 3, NES_AOROM = 7, KONAMI_VRC_4 = 21};
enum NametableLayout : uint8_t {vertical = 1, horizontal = 2, single = 3};
enum Extra : uint8_t {none = 0, VRC4e = 1, MMC1A = 1, MMC1B2 = 2};
enum ChrType : bool {ChrRom = 0, ChrRam = 1};
struct cartAttributes
{
	System system;
	Type type;
	uint16_t prg;
	uint16_t chr;
	ChrType chrType;
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
        bool chrType;

        std::array<NametableOffset, 4> nametableOffsets;

    private:
        void GameInfoSha(std::array<uint32_t, 5> sha1);
        void SetDefaultPrgBanksSha(std::vector<uint8_t> &prgRom, cartAttributes attr);
        void SetDefaultPrgBanks(std::vector<uint8_t> &prgRom);
        void SetChrMem(const std::vector<uint8_t> &fileContent);
        void SetDefaultNametableLayout();

        std::vector<uint8_t> fileContent;
        std::vector<uint8_t> &prgRom;
        std::vector<uint8_t> &prgRam;

        //can't map multiple keys to the same value, maybe fix
        const std::map<std::array<uint32_t, 5>, cartAttributes> cartInfo
        {
            //0
            {{0x22D57AC6, 0x066529D1, 0x99FCD299, 0x159D9482, 0x0042C7D0}, {Ntsc   , NES_NROM    , 16 , 8  , ChrRom, 0, 0, vertical  , none}},   //ice climber

            //1
            {{0xC1A8F6A9, 0x31608048, 0x7CFEACA6, 0x2F3D573C, 0xD9D484E9}, {Ntsc   , NES_SFROM   , 128, 32 , ChrRom, 0, 0, single    , MMC1A }}, //bubble bobble
            {{0x2EC08F93, 0x41003DED, 0x125458DF, 0x8697CA5E, 0xF09D2209}, {Ntsc   , NES_SGROM   , 256, 8  , ChrRam, 0, 0, single    , MMC1B2}}, //mega man 2
            {{0x84675A19, 0x66384AFD, 0xB0715672, 0x207ECE39, 0x97B92033}, {Ntsc   , NES_SLROM   , 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //bart vs space mutants
            {{0x98845D57, 0x16525E68, 0xEF7417E6, 0xFB55BE30, 0xA4C7130D}, {Ntsc   , NES_SLROM   , 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //blaster master
            {{0x6E85261D, 0x5FE84846, 0x80DECD2D, 0x5EDC3194, 0x65887D2A}, {Ntsc   , NES_SLROM   , 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //rad gravity
            {{0x4C6D53EF, 0xFDB90A42, 0x1F8E92C1, 0x1738DD2A, 0xF2EC4495}, {Ntsc   , NES_SLROM   , 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //S W A M P A S S
            {{0x11333ADB, 0x723A5975, 0xE0ECCA3A, 0xEE8F4747, 0xAA8D2D26}, {Ntsc   , NES_SKROM   , 128, 128, ChrRom, 8, 1, single    , MMC1B2}}, //zelda 2
            //some of these can have different mmc1 versions, maybe do something later
            {{0xC9CFBF54, 0x55085E19, 0x8DCE0392, 0x98B083CD, 0x6FC88BCE}, {Ntsc   , NES_SNROM   , 256, 8  , ChrRam, 8, 1, single    , MMC1B2}}, // final fantasy

            //2
            {{0x979494E7, 0x869AC7AB, 0x4815FDBD, 0x1DC99F89, 0x3F713FBF}, {Ntsc   , NES_UNROM   , 128, 8  , ChrRam, 0, 0, horizontal, none}},   //contra

            //3
            {{0x5AFD9664, 0x716116B4, 0x98354B40, 0x48D743D9, 0x8540BF79}, {Ntsc   , NES_CNROM   , 32 , 32 , ChrRom, 0, 0, horizontal, none}},   //wood & water rage

            //7
            {{0xD85C9FF4, 0x89672534, 0xFBF61A15, 0xF8FA56FF, 0xF489A34B}, {Ntsc   , NES_AOROM   , 256, 8  , ChrRam, 0, 0, single    , none}},   //bt
            {{0xA1456332, 0x5B0F33C3, 0x58142E73, 0x63D31614, 0x722FDDB1}, {Ntsc   , NES_AOROM   , 256, 8  , ChrRam, 0, 0, single    , none}},   //bt & dd

            //21
            {{0xF08D61CF, 0x09B6794B, 0xA7E642AC, 0x50161449, 0x3753BA0F}, {Famicom, KONAMI_VRC_4, 128, 128, ChrRom, 2, 0, horizontal, VRC4e}},  //crisis force
        };
};
