//
// Created by tromo on 5/25/25.
//

#include <stdlib.h>

#include "../nes-internal.h"
#include "cpu-addressing.h"
#include "../cpu.h"
#include "../nes.h"

const int TEST_HEADER_SIZE = 0x10;
const int PRG_ROM_BANK_END = 0x4000;
const int PRG_CNT_START = 0x8000;

#define MAP_TO_PRG_ROM(addr) (addr % PRG_ROM_BANK_END + TEST_HEADER_SIZE)

struct Nes* get_test_object() {
    // using calloc instead of malloc here to make sure its initialized to 0
    unsigned char* rom = calloc(0xFFFF, sizeof(unsigned char));

    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;

    rom[0x04] = 0x01; // PRG rom size

    rom[MAP_TO_PRG_ROM(0xFFFC)] = PRG_CNT_START & 0xFF; // Lo PrgCounter, 0xFFFC
    rom[MAP_TO_PRG_ROM(0xFFFD)] = (PRG_CNT_START & 0xFF00) >> 8; // HI PrgCounter, 0xFFFD

    struct Nes* nes = nes_init_from_buffer(rom, 0xFFFF);

    free(rom);
    return nes;
}
void clear_test_object(struct Nes* nes) {
    nes_free(nes);
}

bool test_none_addressing() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xCA; // DEX
    nes_cpu_tick(nes);

    const int instruction_byte_size = 1;
    success &= nes->cpu->x == 0xFF;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_immediate_addressing() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cpu->acc = 0b11000000;
    nes->cartridge->prg_rom[0x0000] = 0x0A; // ASL A
    nes_cpu_tick(nes);

    const int instruction_byte_size = 1;
    success &= nes->cpu->acc == 0b10000000;
    success &= nes->cpu->p.flags.carry == 1;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_acc_addressing() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xA0; // LDY #
    nes->cartridge->prg_rom[0x0001] = 0xCB; // #
    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->y == 0xCB;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_cpu_addressing() {
    bool success = test_none_addressing();
    success &= test_immediate_addressing();
    success &= test_acc_addressing();
    return success;
}