//
// Created by tromo on 3/18/26.
//

#include "cpu.h"

#include <assert.h>

#include "dmg.h"

#include <stdlib.h>

enum Instructions {
    Instruction_NOP = 0x00,

    Instruction_LD_B_B = 0x40,
    Instruction_LD_B_C = 0x41,
    Instruction_LD_B_D = 0x42,
    Instruction_LD_B_E = 0x43,
    Instruction_LD_B_H = 0x44,
    Instruction_LD_B_L = 0x45,
    Instruction_LD_B_A = 0x47,

    Instruction_LD_C_B = 0x48,
    Instruction_LD_C_C = 0x49,
    Instruction_LD_C_D = 0x4A,
    Instruction_LD_C_E = 0x4B,
    Instruction_LD_C_H = 0x4C,
    Instruction_LD_C_L = 0x4D,
    Instruction_LD_C_A = 0x4F,

    Instruction_LD_D_B = 0x50,
    Instruction_LD_D_C = 0x51,
    Instruction_LD_D_D = 0x52,
    Instruction_LD_D_E = 0x53,
    Instruction_LD_D_H = 0x54,
    Instruction_LD_D_L = 0x55,
    Instruction_LD_D_A = 0x57,

    Instruction_LD_E_B = 0x58,
    Instruction_LD_E_C = 0x59,
    Instruction_LD_E_D = 0x5A,
    Instruction_LD_E_E = 0x5B,
    Instruction_LD_E_H = 0x5C,
    Instruction_LD_E_L = 0x5D,
    Instruction_LD_E_A = 0x5F,

    Instruction_LD_H_B = 0x60,
    Instruction_LD_H_C = 0x61,
    Instruction_LD_H_D = 0x62,
    Instruction_LD_H_E = 0x63,
    Instruction_LD_H_H = 0x64,
    Instruction_LD_H_L = 0x65,
    Instruction_LD_H_A = 0x67,

    Instruction_LD_L_B = 0x68,
    Instruction_LD_L_C = 0x69,
    Instruction_LD_L_D = 0x6A,
    Instruction_LD_L_E = 0x6B,
    Instruction_LD_L_H = 0x6C,
    Instruction_LD_L_L = 0x6D,
    Instruction_LD_L_A = 0x6F,

    Instruction_LD_A_B = 0x78,
    Instruction_LD_A_C = 0x79,
    Instruction_LD_A_D = 0x7A,
    Instruction_LD_A_E = 0x7B,
    Instruction_LD_A_H = 0x7C,
    Instruction_LD_A_L = 0x7D,
    Instruction_LD_A_A = 0x7F,

    Instruction_LD_B_HL_INDIRECT = 0x46,
    Instruction_LD_C_HL_INDIRECT = 0x4E,
    Instruction_LD_D_HL_INDIRECT = 0x56,
    Instruction_LD_E_HL_INDIRECT = 0x5E,
    Instruction_LD_H_HL_INDIRECT = 0x66,
    Instruction_LD_L_HL_INDIRECT = 0x6E,
    Instruction_LD_A_HL_INDIRECT = 0x7E,

    Instruction_LD_HL_B_INDIRECT = 0x70,
    Instruction_LD_HL_C_INDIRECT = 0x71,
    Instruction_LD_HL_D_INDIRECT = 0x72,
    Instruction_LD_HL_E_INDIRECT = 0x73,
    Instruction_LD_HL_H_INDIRECT = 0x74,
    Instruction_LD_HL_L_INDIRECT = 0x75,
    Instruction_LD_HL_A_INDIRECT = 0x77,
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
    cpu->instruction_step = 0;
    cpu->is_final_step = false;
    cpu->last_operation = Operation_None;

    return cpu;
}

void dmg_cpu_fetch_next_instruction(struct DMG* dmg, struct CPU* cpu) {
    const auto pc = cpu->PC;
    const auto instruction = dmg_read_u8(dmg, pc);
    cpu->IR = instruction;
    cpu->instruction_step = 0;
    cpu->is_final_step = false;
    cpu->PC++;
}

void dmg_cpu_tick_m_cycle(struct DMG* dmg, struct CPU* cpu) {
    dmg_cpu_process_instruction(dmg, cpu, cpu->IR);
}

uint8_t* get_r8(struct CPU* cpu, uint8_t index) {
    switch (index) {
        case 0:
            return &cpu->B;
        case 1:
            return &cpu->C;
        case 2:
            return &cpu->D;
        case 3:
            return &cpu->E;
        case 4:
            return &cpu->H;
        case 5:
            return &cpu->L;
        case 6:
            assert(false); // This is [HL] and should be handled separately
            return &cpu->B;
        case 7:
            return &cpu->A;
    }
}

void load_r8_r8(struct DMG* dmg, struct CPU* cpu) {
    const auto target = (cpu->IR >> 3) & 0x7;
    const auto destination = cpu->IR & 0x7;

    const auto target_register = get_r8(cpu, target);
    const auto destination_register = get_r8(cpu, destination);
    *target_register = *destination_register;

    cpu->is_final_step = true;
}

void load_r8_hl(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_read_u8(dmg, cpu->HL);
        case 1:
            const auto target = (cpu->IR >> 3) & 0x7;
            const auto target_register = get_r8(cpu, target);

            *target_register = cpu->TMP;
            break;
        default: break;
    }

    cpu->is_final_step = cpu->instruction_step == 1;
    cpu->instruction_step++;
}

void load_hl_r8(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            const auto target = cpu->IR & 0x7;
            const auto target_register = get_r8(cpu, target);

            dmg_write_u8(dmg, cpu->HL, *target_register);
        case 1:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    cpu->is_final_step = cpu->instruction_step == 1;
    cpu->instruction_step++;
}

void dmg_cpu_process_instruction(struct DMG* dmg, struct CPU* cpu, uint8_t instruction) {
    cpu->total_cycles++;
    switch (instruction) {
        case Instruction_NOP:
            cpu->is_final_step = true;
            break;
        case Instruction_LD_B_B:
        case Instruction_LD_B_C:
        case Instruction_LD_B_D:
        case Instruction_LD_B_E:
        case Instruction_LD_B_H:
        case Instruction_LD_B_L:
        case Instruction_LD_B_A:

        case Instruction_LD_C_B:
        case Instruction_LD_C_C:
        case Instruction_LD_C_D:
        case Instruction_LD_C_E:
        case Instruction_LD_C_H:
        case Instruction_LD_C_L:
        case Instruction_LD_C_A:

        case Instruction_LD_D_B:
        case Instruction_LD_D_C:
        case Instruction_LD_D_D:
        case Instruction_LD_D_E:
        case Instruction_LD_D_H:
        case Instruction_LD_D_L:
        case Instruction_LD_D_A:

        case Instruction_LD_E_B:
        case Instruction_LD_E_C:
        case Instruction_LD_E_D:
        case Instruction_LD_E_E:
        case Instruction_LD_E_H:
        case Instruction_LD_E_L:
        case Instruction_LD_E_A:

        case Instruction_LD_H_B:
        case Instruction_LD_H_C:
        case Instruction_LD_H_D:
        case Instruction_LD_H_E:
        case Instruction_LD_H_H:
        case Instruction_LD_H_L:
        case Instruction_LD_H_A:

        case Instruction_LD_L_B:
        case Instruction_LD_L_C:
        case Instruction_LD_L_D:
        case Instruction_LD_L_E:
        case Instruction_LD_L_H:
        case Instruction_LD_L_L:
        case Instruction_LD_L_A:

        case Instruction_LD_A_B:
        case Instruction_LD_A_C:
        case Instruction_LD_A_D:
        case Instruction_LD_A_E:
        case Instruction_LD_A_H:
        case Instruction_LD_A_L:
        case Instruction_LD_A_A:
            load_r8_r8(dmg, cpu);
            break;

        case Instruction_LD_B_HL_INDIRECT:
        case Instruction_LD_C_HL_INDIRECT:
        case Instruction_LD_D_HL_INDIRECT:
        case Instruction_LD_E_HL_INDIRECT:
        case Instruction_LD_H_HL_INDIRECT:
        case Instruction_LD_L_HL_INDIRECT:
        case Instruction_LD_A_HL_INDIRECT:
            load_r8_hl(dmg, cpu);
            break;

        case Instruction_LD_HL_B_INDIRECT:
        case Instruction_LD_HL_C_INDIRECT:
        case Instruction_LD_HL_D_INDIRECT:
        case Instruction_LD_HL_E_INDIRECT:
        case Instruction_LD_HL_H_INDIRECT:
        case Instruction_LD_HL_L_INDIRECT:
        case Instruction_LD_HL_A_INDIRECT:
            load_hl_r8(dmg, cpu);
            break;
    }

    // final m-step for every instruction overlaps with m1, so the next instruction should be fetched while its on the last step
    if (cpu->is_final_step) {
        dmg_cpu_fetch_next_instruction(dmg, cpu);
    }
}

void dmg_cpu_free(struct CPU* cpu) {
    free(cpu);
}
