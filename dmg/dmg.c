//
// Created by tromo on 3/18/26.
//
#include <stdlib.h>

#include "dmg.h"
#include "cartridge.h"
#include "cpu.h"

struct DMG* dmg_init() {
    const auto dmg = (struct DMG*)malloc(sizeof(struct DMG));

    dmg->cpu = dmg_cpu_init(dmg);

    return dmg;
}
struct DMG* dmg_init_from_buffer(const uint8_t* buffer) {
    const auto dmg = (struct DMG*)malloc(sizeof(struct DMG));

    dmg->cartridge = dmg_cartridge_init_from_buffer(buffer);
    dmg->cpu =  dmg_cpu_init(dmg);

    return dmg;
}
void dmg_free(struct DMG* dmg) {
    dmg_cartridge_free(dmg->cartridge);
    dmg_cpu_free(dmg->cpu);
    free(dmg);
}

void dmg_tick(struct DMG* dmg, Color* frame_buffer, bool* is_new_frame) {

}

uint8_t dmg_read_u8(struct DMG* dmg, uint16_t addr) {
#if TESTS
    return dmg->cartridge->rom[addr];
#endif

    return 0;
}
void dmg_write_u8(struct DMG* dmg, uint16_t addr, uint8_t val) {
#if TESTS
    dmg->cartridge->rom[addr] = val;
#endif
}

uint16_t dmg_read_u16(struct DMG* dmg, uint16_t addr) {

}
void dmg_write_u16(struct DMG* dmg, uint16_t addr, uint16_t val) {

}
