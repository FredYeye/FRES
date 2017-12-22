#pragma once

#include <map>

#include "ppu.hpp"


enum System : uint8_t {Ntsc = 0, Pal = 1};
enum Type : uint16_t
{
    NROM = 0,
    SFROM = 1, SGROM = 1, SKROM = 1, SLROM = 1, SNROM = 1,
    UNROM = 2,
    CNROM = 3,
    TGROM = 4, TKROM = 4, TLROM = 4, TSROM = 4,
    AOROM = 7,
    VRC_4 = 21
};
enum NametableLayout : uint8_t {vertical = 1, horizontal = 2, single = 3};
enum Extra : uint8_t {none = 0, VRC4e = 1, MMC1A = 1, MMC1B2 = 2, MMC1B3 = 3, MMC3B = 4};
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
            {{0x22D57AC6, 0x066529D1, 0x99FCD299, 0x159D9482, 0x0042C7D0}, {Ntsc, NROM , 16 , 8  , ChrRom, 0, 0, vertical  , none  }}, //ice climber
            {{0x40262A15, 0xEE764F67, 0x6E752C90, 0x626A2F61, 0xDA370A1A}, {Ntsc, NROM , 32 , 8  , ChrRom, 0, 0, horizontal, none  }}, //ice hockey
            {{0xFACEE9C5, 0x77A5262D, 0xBE33AC49, 0x30BB0B58, 0xC8C037F7}, {Ntsc, NROM , 32 , 8  , ChrRom, 0, 0, horizontal, none  }}, //super mario bros.

            //1
            //some of these can have different mmc1 versions, maybe do something later
            {{0xC1A8F6A9, 0x31608048, 0x7CFEACA6, 0x2F3D573C, 0xD9D484E9}, {Ntsc, SFROM, 128, 32 , ChrRom, 0, 0, single    , MMC1A }}, //bubble bobble
            {{0xC82E2963, 0x9E2DFFA2, 0x75414902, 0x5C6784EB, 0x2587B09D}, {Ntsc, SGROM, 256, 8  , ChrRam, 0, 0, single    , MMC1A }}, //bionic commando
            {{0x2EC08F93, 0x41003DED, 0x125458DF, 0x8697CA5E, 0xF09D2209}, {Ntsc, SGROM, 256, 8  , ChrRam, 0, 0, single    , MMC1B2}}, //mega man 2
            {{0x84675A19, 0x66384AFD, 0xB0715672, 0x207ECE39, 0x97B92033}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //bart vs space mutants
            {{0x98845D57, 0x16525E68, 0xEF7417E6, 0xFB55BE30, 0xA4C7130D}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //blaster master
            {{0x15F24516, 0x1179AB19, 0x59B7DC20, 0xE82ADD02, 0x4D23AA3D}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //ninja gaiden
            {{0x83F6E20D, 0x75327C9B, 0x6586E9CB, 0xECF32996, 0xA1707D86}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //pictionary
            {{0x6E85261D, 0x5FE84846, 0x80DECD2D, 0x5EDC3194, 0x65887D2A}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //rad gravity
            {{0x4C6D53EF, 0xFDB90A42, 0x1F8E92C1, 0x1738DD2A, 0xF2EC4495}, {Ntsc, SLROM, 128, 128, ChrRom, 0, 0, single    , MMC1B2}}, //swamp thing
            {{0x11333ADB, 0x723A5975, 0xE0ECCA3A, 0xEE8F4747, 0xAA8D2D26}, {Ntsc, SKROM, 128, 128, ChrRom, 8, 1, single    , MMC1B2}}, //zelda 2
            {{0xC9CFBF54, 0x55085E19, 0x8DCE0392, 0x98B083CD, 0x6FC88BCE}, {Ntsc, SNROM, 256, 8  , ChrRam, 8, 1, single    , MMC1B2}}, //final fantasy
            {{0xBE2F5DC8, 0xC5BA8EC1, 0xA344A71F, 0x9FB20475, 0x0AF24FE7}, {Ntsc, SNROM, 128, 8  , ChrRam, 8, 1, single    , MMC1B3}}, //zelda revA

            //2
            {{0x979494E7, 0x869AC7AB, 0x4815FDBD, 0x1DC99F89, 0x3F713FBF}, {Ntsc, UNROM, 128, 8  , ChrRam, 0, 0, horizontal, none  }}, //contra
            {{0xEE797CF1, 0x7EEF5241, 0x17DC1EF3, 0xD6DDCE0C, 0x193ACFD1}, {Ntsc, UNROM, 128, 8  , ChrRam, 0, 0, horizontal, none  }}, //dragon's lair
            {{0xF85DA3A5, 0xA252567F, 0x3905B112, 0x830B0F1C, 0x297CF34C}, {Ntsc, UNROM, 128, 8  , ChrRam, 0, 0, horizontal, none  }}, //ducktales 2
            {{0xCA03C76B, 0x65F0FE5B, 0x1D05149D, 0x7E9A97B4, 0xD5F44A27}, {Ntsc, UNROM, 128, 8  , ChrRam, 0, 0, horizontal, none  }}, //ghosts 'n goblins

            //3
            {{0x5AFD9664, 0x716116B4, 0x98354B40, 0x48D743D9, 0x8540BF79}, {Ntsc, CNROM, 32 , 32 , ChrRom, 0, 0, horizontal, none  }}, //wood & water rage

            //4
            {{0xC70DB279, 0x964E0F83, 0x5427CA0A, 0x406D54A4, 0x5AE7783A}, {Ntsc, TGROM, 512, 8  , ChrRam, 0, 0, vertical  , MMC3B }}, //mega man 4 revA
            {{0x8A49FE60, 0xB6A151C0, 0x55A63639, 0x894CD366, 0x935A7EE9}, {Ntsc, TKROM, 256, 128, ChrRom, 8, 1, vertical  , MMC3B }}, //crystalis
            {{0xA6C56AC7, 0x87FF11A6, 0x7921867F, 0xFC9F79A5, 0x7A382657}, {Ntsc, TLROM, 128, 128, ChrRom, 0, 0, vertical  , MMC3B }}, //batman
            {{0x6780B3FC, 0xC547C013, 0xEE45AFAC, 0x6BB30C6F, 0xC6D8B46E}, {Ntsc, TLROM, 256, 128, ChrRom, 0, 0, vertical  , MMC3B }}, //mega man 3
            {{0x503EB239, 0x55475D10, 0x5029FDD6, 0xAE082BCC, 0x14E7306A}, {Ntsc, TLROM, 128, 128, ChrRom, 0, 0, vertical  , MMC3B }}, //shatterhand
            {{0xBB894D10, 0x4C796F69, 0xBA16587E, 0xB66C0275, 0xF5C2FC02}, {Ntsc, TSROM, 256, 128, ChrRom, 8, 0, vertical  , MMC3B }}, //super mario bros. 3 revA

            //7
            {{0xD85C9FF4, 0x89672534, 0xFBF61A15, 0xF8FA56FF, 0xF489A34B}, {Ntsc, AOROM, 256, 8  , ChrRam, 0, 0, single    , none  }}, //battletoads
            {{0xA1456332, 0x5B0F33C3, 0x58142E73, 0x63D31614, 0x722FDDB1}, {Ntsc, AOROM, 256, 8  , ChrRam, 0, 0, single    , none  }}, //bt & dd
            {{0xDA5DD988, 0x86A69195, 0x0C22FD43, 0x9DA5E271, 0x8709150A}, {Ntsc, AOROM, 256, 8  , ChrRam, 0, 0, single    , none  }}, //w&w III

            //21
            {{0xF08D61CF, 0x09B6794B, 0xA7E642AC, 0x50161449, 0x3753BA0F}, {Ntsc, VRC_4, 128, 128, ChrRom, 2, 0, horizontal, VRC4e }}, //crisis force
        };
};
