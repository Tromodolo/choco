//
// Created by tromo on 11/25/25.
//

#include "../../cartridge.h"
#include "uxrom.h"

#include <stdlib.h>

struct UxRom {
    uint8_t current_bank;
    uint8_t* bank_one;
    uint8_t* bank_two;
};

constexpr int BANK_SIZE = 0x4000;

void uxrom_init(struct Cartridge* cartridge) {
    struct UxRom* uxrom = malloc(sizeof(struct UxRom));

    const int last_bank_addr = cartridge->prg_rom_size - BANK_SIZE;

    uxrom->current_bank = 0;
    uxrom->bank_one = &cartridge->prg_rom[0];
    uxrom->bank_two = &cartridge->prg_rom[last_bank_addr];

    cartridge->mapper = uxrom;
}

void uxrom_free(struct Cartridge* cartridge) {
    free(cartridge->mapper);
    cartridge->mapper = nullptr;
}

uint8_t uxrom_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct UxRom* uxrom = cartridge->mapper;
    if (addr >= 0x8000 && addr < 0xC000) {
        addr -= 0x8000;

        *is_mapped = true;
        return uxrom->bank_one[addr];
    } else if (addr >= 0xC000 && addr <= 0xFFFF) {
        addr -= 0xC000;

        *is_mapped = true;
        return uxrom->bank_two[addr];
    }

    return 0;
}
void uxrom_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    struct UxRom* uxrom = cartridge->mapper;
    if (addr >= 0x8000 && addr <= 0xFFFF) {
        *is_mapped = true;
        uxrom->current_bank = val & 0b1111;

        const int bank_pos = uxrom->current_bank * BANK_SIZE;
        uxrom->bank_one = &cartridge->prg_rom[bank_pos];
    }
}