//
// Created by tromo on 5/25/25.
//

#include <stdlib.h>

#include "../nes-internal.h"
#include "cpu-addressing.h"

#include <stdio.h>

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

bool test_relative_addressing_condition_true() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cpu->p.flags.carry = 0;
    nes->cartridge->prg_rom[0x0000] = 0x90; // Branch on Carry Clear
    nes->cartridge->prg_rom[0x0001] = (int8_t)40; // Branch on Carry Clear
    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size + 40;

    clear_test_object(nes);
    return success;
}

bool test_relative_addressing_condition_false() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cpu->p.flags.carry = 1;
    nes->cartridge->prg_rom[0x0000] = 0x90; // Branch on Carry Clear
    nes->cartridge->prg_rom[0x0001] = (int8_t)40; // Branch on Carry Clear
    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_addressing() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xA5; // LDA zpg $40
    nes->cartridge->prg_rom[0x0001] = 0x32;

    nes->cartridge->prg_ram[0x0032] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->acc == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_addressing_x() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xB5; // LDA zpg $,X
    nes->cartridge->prg_rom[0x0001] = 0x32;
    nes->cpu->x = 8;

    nes->cartridge->prg_ram[0x003A] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->acc == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_addressing_y() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xB6; // LDX zpg $,Y
    nes->cartridge->prg_rom[0x0001] = 0x32;
    nes->cpu->y = 8;

    nes->cartridge->prg_ram[0x003A] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->x == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_absolute() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xAC; // LDY abs
    nes->cartridge->prg_rom[0x0001] = 0x32;
    nes->cartridge->prg_rom[0x0002] = 0x03;

    nes->cartridge->prg_ram[0x0332] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 3;
    success &= nes->cpu->y == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_absolute_x() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xBC; // LDY abs, X
    nes->cartridge->prg_rom[0x0001] = 0x32;
    nes->cartridge->prg_rom[0x0002] = 0x03;
    nes->cpu->x = 3;

    nes->cartridge->prg_ram[0x0335] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 3;
    success &= nes->cpu->y == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_absolute_y() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cartridge->prg_rom[0x0000] = 0xBE; // LDX abs, Y
    nes->cartridge->prg_rom[0x0001] = 0x32;
    nes->cartridge->prg_rom[0x0002] = 0x03;
    nes->cpu->y = 3;

    nes->cartridge->prg_ram[0x0335] = 0x7C;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 3;
    success &= nes->cpu->x == 0x7C;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_indirect_x() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cpu->x = 5;
    nes->cartridge->prg_rom[0x0000] = 0xA1; // LDA ind, X
    nes->cartridge->prg_rom[0x0001] = 0x32;

    nes->cartridge->prg_ram[0x0037] = 0x25;
    nes->cartridge->prg_ram[0x0038] = 0x12;

    nes->cartridge->prg_ram[0x225] = 0x33;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->acc == 0x33;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

bool test_zero_page_indirect_y() {
    struct Nes* nes = get_test_object();
    bool success = true;

    nes->cpu->y = 5;
    nes->cartridge->prg_rom[0x0000] = 0xB1; // LDA ind, Y
    nes->cartridge->prg_rom[0x0001] = 0x32;

    nes->cartridge->prg_ram[0x0032] = 0x25;
    nes->cartridge->prg_ram[0x0033] = 0x12;

    nes->cartridge->prg_ram[0x22A] = 0x33;

    nes_cpu_tick(nes);

    const int instruction_byte_size = 2;
    success &= nes->cpu->acc == 0x33;
    success &= nes->cpu->pc == PRG_CNT_START + instruction_byte_size;

    clear_test_object(nes);
    return success;
}

#define BEGIN_TESTS() bool success = true;
#define ADD_TEST(test) \
    success &= test(); \
    if (!success) {\
        fprintf(stderr, #test " failed!\n");\
    }
#define END_TESTS() return success;

bool test_cpu_addressing() {
    BEGIN_TESTS();
    ADD_TEST(test_none_addressing);
    ADD_TEST(test_immediate_addressing);
    ADD_TEST(test_acc_addressing);
    ADD_TEST(test_relative_addressing_condition_true);
    ADD_TEST(test_relative_addressing_condition_false);
    ADD_TEST(test_zero_page_addressing);
    ADD_TEST(test_zero_page_addressing_x);
    ADD_TEST(test_zero_page_addressing_y);
    ADD_TEST(test_zero_page_absolute);
    ADD_TEST(test_zero_page_absolute_x);
    ADD_TEST(test_zero_page_absolute_y);
    ADD_TEST(test_zero_page_indirect_x);
    ADD_TEST(test_zero_page_indirect_y);
    END_TESTS();
}