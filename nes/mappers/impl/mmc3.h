//
// Created by tromo on 11/25/25.
//

#ifndef MMC3_H
#define MMC3_H
#include <stdint.h>

void mmc3_init(struct Cartridge* cartridge, const char* file_path);
void mmc3_free(struct Cartridge* cartridge);

uint8_t mmc3_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mmc3_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

uint8_t mmc3_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mmc3_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

void mmc3_decrement_scanline(const struct Cartridge* cartridge);
bool mmc3_get_irq(const struct Cartridge* cartridge);
void mmc3_set_irq(const struct Cartridge* cartridge, bool set);
enum Mirroring mmc3_get_mirroring(const struct Cartridge* cartridge);

#endif //MMC3_H
