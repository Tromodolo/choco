//
// Created by tromo on 11/25/25.
//

#ifndef MAPPER_H
#define MAPPER_H
#include <stdint.h>

void mapper_init(struct Cartridge* cartridge);
void mapper_free(struct Cartridge* cartridge);

enum Mirroring mapper_get_mirroring(struct Cartridge* cartridge);

bool mapper_get_irq(struct Cartridge* cartridge);
void mapper_set_irq(struct Cartridge* cartridge, bool irq);

void mapper_set_pc(struct Cartridge* cartridge, uint16_t pc);

void mapper_decrement_scanline(struct Cartridge* cartridge);

uint8_t mapper_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mapper_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);

uint8_t mapper_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped);
void mapper_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped);
#endif //MAPPER_H
