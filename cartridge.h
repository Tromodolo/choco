//
// Created by tromo on 5/24/25.
//

#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <stdint.h>

struct Cartridge* nes_cartridge_load_from_file(const char* file_path);
struct Cartridge* nes_cartridge_load_from_buffer(const uint8_t* buffer, const long size);
void nes_cartridge_free(struct Cartridge* cartridge);

uint8_t* nes_cartridge_get_addr_ptr(const struct Cartridge* cartridge, uint16_t addr);

uint8_t nes_cartridge_read_char(const struct Cartridge* cartridge, uint16_t addr);
void nes_cartridge_write_char(const struct Cartridge* cartridge, uint16_t addr, uint8_t val);

#endif //CARTRIDGE_H
