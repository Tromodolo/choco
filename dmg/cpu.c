//
// Created by tromo on 3/18/26.
//

#include "cpu.h"
#include "dmg.h"

#include <stdlib.h>

enum Instructions {
    Instruction_NOP = 0x00,
};

struct CPU* dmg_cpu_init(struct DMG* dmg) {
    const auto cpu = (struct CPU*)malloc(sizeof(struct CPU));

    cpu->A = 0x01;
    cpu->F = Flag_Zero; // TODO: Set the cpu flags depending on the checksum
    cpu->B = 0x00;
    cpu->C = 0x13;
    cpu->D = 0x00;
    cpu->E = 0xD8;
    cpu->H = 0x01;
    cpu->L = 0x4D;

    cpu->SP = 0xFFFE;
    cpu->PC = 0x100; // Setting to 0x100 skips the BIOS

    cpu->IR = 0x00;

    cpu->total_cycles = 0;

    return cpu;
}

void dmg_cpu_fetch_next_instruction(struct DMG* dmg, struct CPU* cpu) {
    const auto pc = cpu->PC;
    const auto instruction = dmg_read_u8(dmg, pc);
    cpu->IR = instruction;
    cpu->PC++;
}

void dmg_cpu_tick_m_cycle(struct DMG* dmg, struct CPU* cpu) {
    dmg_cpu_process_instruction(dmg, cpu, cpu->IR);
}

void do_nop(struct DMG* dmg, struct CPU* cpu) {
    // nothing
}

void dmg_cpu_process_instruction(struct DMG* dmg, struct CPU* cpu, uint8_t instruction) {
    cpu->total_cycles++;
    switch (instruction) {
        case Instruction_NOP:
            do_nop(dmg, cpu);
            return;
    }
}

void dmg_cpu_free(struct CPU* cpu) {
    free(cpu);
}
