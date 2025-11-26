//
// Created by tromo on 11/25/25.
//

#ifndef NROM_H
#define NROM_H
#include <stdint.h>

void nrom_init(struct Cartridge* cartridge);
void nrom_free(struct Cartridge* cartridge);

uint8_t nrom_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void nrom_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

#endif //NROM_H
