//
// Created by tromo on 3/18/26.
//

#include "cartridge.h"

#include <stdlib.h>
#include <string.h>

const uint16_t ROM_SIZE = 0xFFFF;
const uint16_t WRAM_SIZE = 0x4000;

struct Cartridge* dmg_cartridge_init() {
    const auto cartridge = (struct Cartridge*)malloc(sizeof(struct Cartridge));

    cartridge->rom = malloc(sizeof(unsigned char) * ROM_SIZE);
    cartridge->wram = malloc(sizeof(unsigned char) * WRAM_SIZE);

    return cartridge;
}

struct Cartridge* dmg_cartridge_init_from_buffer(const uint8_t* buffer) {
    const auto cartridge = (struct Cartridge*)malloc(sizeof(struct Cartridge));

    cartridge->num_rom_banks = 2;
    cartridge->num_ram_banks = 0;

    cartridge->rom = malloc(ROM_SIZE * sizeof(uint8_t));
    memcpy(cartridge->rom, buffer, ROM_SIZE);

    return cartridge;
}

void dmg_cartridge_free(struct Cartridge* cartridge) {
    free(cartridge->rom);
    free(cartridge);
}

uint8_t dmg_cartridge_read_char(const struct Cartridge* cartridge, uint16_t addr) {
    return 0;
}

void dmg_cartridge_write_char(const struct Cartridge* cartridge, uint16_t addr, uint8_t val) {

}