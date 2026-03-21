//
// Created by tromo on 3/18/26.
//

#ifndef DMG_H
#define DMG_H
#include <raylib.h>
#include <stdint.h>

struct DMG {
    struct Cartridge* cartridge;
    struct CPU* cpu;
};

struct DMG* dmg_init();
struct DMG* dmg_init_from_buffer(const uint8_t* buffer);
void dmg_free(struct DMG* dmg);

void dmg_tick(struct DMG* dmg, Color* frame_buffer, bool* is_new_frame);

uint8_t dmg_read_u8(struct DMG* dmg, uint16_t addr);
void dmg_write_u8(struct DMG* dmg, uint16_t addr, uint8_t val);
#endif //DMG_H
