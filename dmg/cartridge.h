//
// Created by tromo on 3/18/26.
//

#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <stdint.h>

enum Mapper {
    Mapper_None
};

struct Cartridge {
    enum Mapper mapper_type;
    uint16_t num_rom_banks;
    uint16_t num_ram_banks;

    unsigned char* rom;
    unsigned char* wram;
};

struct Cartridge* dmg_cartridge_init();
struct Cartridge* dmg_cartridge_init_from_buffer(const uint8_t* buffer);
void dmg_cartridge_free(struct Cartridge* cartridge);

uint8_t dmg_cartridge_read_char(const struct Cartridge* cartridge, uint16_t addr);
void dmg_cartridge_write_char(const struct Cartridge* cartridge, uint16_t addr, uint8_t val);

#endif //CARTRIDGE_H
