//
// Created by tromo on 11/25/25.
//

#ifndef UXROM_H
#define UXROM_H
#include <stdint.h>

void uxrom_init(struct Cartridge* cartridge);
void uxrom_free(struct Cartridge* cartridge);

uint8_t uxrom_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void uxrom_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

#endif //UXROM_H
