//
// Created by tromo on 11/25/25.
//

#ifndef MMC1_H
#define MMC1_H
#include <stdint.h>

void mmc1_init(struct Cartridge* cartridge);
void mmc1_free(struct Cartridge* cartridge);

uint8_t mmc1_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mmc1_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

uint8_t mmc1_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mmc1_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

void mmc1_set_pc(const struct Cartridge* cartridge, uint16_t pc);
enum Mirroring mmc1_get_mirroring(const struct Cartridge* cartridge);

#endif //MMC1_H
