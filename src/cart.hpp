#pragma once

#include "ppu.hpp"


struct Cart
{
    public:
        Cart(const std::string inFile);

        std::array<uint8_t, 16> header;
        uint8_t mapper;

        std::vector<uint8_t> prgRom;
        std::array<uint8_t*, 4> pPrgBank;

        std::vector<uint8_t> chrMem;
        bool isChrRam;

        std::array<NametableOffset, 4> nametableOffsets;

    private:
        void SetDefaultPrgBanks();
        void SetChrMem(const std::vector<uint8_t> &fileContent);
        void SetDefaultNametableLayout();
};
