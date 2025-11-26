//
// Created by tromo on 11/25/25.
//

#include "../../cartridge.h"
#include "nrom.h"

#include <stdlib.h>

struct UxRom {
    bool is_256;
};

void nrom_init(struct Cartridge* cartridge) {
    struct UxRom* nrom = malloc(sizeof(struct UxRom));

    nrom->is_256 = cartridge->prg_rom_size > 0x4000;

    cartridge->mapper = nrom;
}

void nrom_free(struct Cartridge* cartridge) {
    free(cartridge->mapper);
    cartridge->mapper = nullptr;
}

uint8_t nrom_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct UxRom* nrom = cartridge->mapper;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        addr -= 0x8000;

        if (!nrom->is_256 && addr >= 0x4000) {
            addr -= 0x4000;
        }

        *is_mapped = true;
        return cartridge->prg_rom[addr];
    }

    return 0;
}
void nrom_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    const struct UxRom* nrom = cartridge->mapper;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        addr -= 0x8000;

        if (!nrom->is_256 && addr >= 0x4000) {
            addr -= 0x4000;
        }

        *is_mapped = true;
        cartridge->prg_rom[addr] = val;
    }
}