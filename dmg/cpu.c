//
// Created by tromo on 3/18/26.
//

#include "cpu.h"

#include <assert.h>

#include "dmg.h"

#include <stdlib.h>

enum Instructions {
    NOP = 0x00,

    LD_B_B = 0x40,
    LD_B_C = 0x41,
    LD_B_D = 0x42,
    LD_B_E = 0x43,
    LD_B_H = 0x44,
    LD_B_L = 0x45,
    LD_B_A = 0x47,

    LD_C_B = 0x48,
    LD_C_C = 0x49,
    LD_C_D = 0x4A,
    LD_C_E = 0x4B,
    LD_C_H = 0x4C,
    LD_C_L = 0x4D,
    LD_C_A = 0x4F,

    LD_D_B = 0x50,
    LD_D_C = 0x51,
    LD_D_D = 0x52,
    LD_D_E = 0x53,
    LD_D_H = 0x54,
    LD_D_L = 0x55,
    LD_D_A = 0x57,

    LD_E_B = 0x58,
    LD_E_C = 0x59,
    LD_E_D = 0x5A,
    LD_E_E = 0x5B,
    LD_E_H = 0x5C,
    LD_E_L = 0x5D,
    LD_E_A = 0x5F,

    LD_H_B = 0x60,
    LD_H_C = 0x61,
    LD_H_D = 0x62,
    LD_H_E = 0x63,
    LD_H_H = 0x64,
    LD_H_L = 0x65,
    LD_H_A = 0x67,

    LD_L_B = 0x68,
    LD_L_C = 0x69,
    LD_L_D = 0x6A,
    LD_L_E = 0x6B,
    LD_L_H = 0x6C,
    LD_L_L = 0x6D,
    LD_L_A = 0x6F,

    LD_A_B = 0x78,
    LD_A_C = 0x79,
    LD_A_D = 0x7A,
    LD_A_E = 0x7B,
    LD_A_H = 0x7C,
    LD_A_L = 0x7D,
    LD_A_A = 0x7F,

    LD_B_HL_INDIRECT = 0x46,
    LD_C_HL_INDIRECT = 0x4E,
    LD_D_HL_INDIRECT = 0x56,
    LD_E_HL_INDIRECT = 0x5E,
    LD_H_HL_INDIRECT = 0x66,
    LD_L_HL_INDIRECT = 0x6E,
    LD_A_HL_INDIRECT = 0x7E,

    LD_HL_B_INDIRECT = 0x70,
    LD_HL_C_INDIRECT = 0x71,
    LD_HL_D_INDIRECT = 0x72,
    LD_HL_E_INDIRECT = 0x73,
    LD_HL_H_INDIRECT = 0x74,
    LD_HL_L_INDIRECT = 0x75,
    LD_HL_A_INDIRECT = 0x77,

    LD_B_D8 = 0x06,
    LD_C_D8 = 0x0E,
    LD_D_D8 = 0x16,
    LD_E_D8 = 0x1E,
    LD_H_D8 = 0x26,
    LD_L_D8 = 0x2E,
    LD_A_D8 = 0x3E,

    LD_HL_D8 = 0x36,

    LD_BC_D16 = 0x01,
    LD_DE_D16 = 0x11,
    LD_HL_D16 = 0x21,
    LD_SP_D16 = 0x31,

    LD_ABS_BC_A = 0x02,
    LD_ABS_DE_A = 0x12,
    LD_ABS_HLI_A = 0x22,
    LD_ABS_HLD_A = 0x32,

    LD_A_ABS_BC = 0x0A,
    LD_A_ABS_DE = 0x1A,
    LD_A_ABS_HLI = 0x2A,
    LD_A_ABS_HLD = 0x3A,

    LD_ABS_D16_A = 0xEA,
    LD_A_ABS_D16 = 0xFA,

    LDH_C_A_INDIRECT = 0xE2,
    LDH_A_C_INDIRECT = 0xF2,

    LDH_D8_A_DIRECT = 0xE0,
    LDH_A_D8_DIRECT = 0xF0,
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

    cpu->TMP = 0;
    cpu->TMP2 = 0;

    cpu->total_cycles = 0;
    cpu->instruction_step = 0;
    cpu->is_final_step = false;

    cpu->last_operation = Operation_None;
    cpu->last_memory_addr = 0;
    cpu->last_memory_value = 0;

    return cpu;
}

void dmg_cpu_fetch_next_instruction(struct DMG* dmg, struct CPU* cpu) {
    const auto pc = cpu->PC;
    const auto instruction = dmg_cpu_read_u8(dmg, cpu, pc);

    cpu->IR = instruction;
    cpu->instruction_step = 0;
    cpu->is_final_step = false;
    cpu->PC++;
}

void dmg_cpu_tick_m_cycle(struct DMG* dmg, struct CPU* cpu) {
    dmg_cpu_process_instruction(dmg, cpu, cpu->IR);
}

uint8_t dmg_cpu_read_u8(struct DMG* dmg, struct CPU* cpu, const uint16_t addr) {
    const auto val = dmg_read_u8(dmg, addr);
    cpu->last_operation = Operation_Read;
    cpu->last_memory_addr = addr;
    cpu->last_memory_value = val;
    return val;
}

void dmg_cpu_write_u8(struct DMG* dmg, struct CPU* cpu, const uint16_t addr, const uint8_t val) {
    cpu->last_operation = Operation_Write;
    cpu->last_memory_addr = addr;
    cpu->last_memory_value = val;
    dmg_write_u8(dmg, addr, val);
}

//
// UTIL FUNCTIONS
//
void increment_instruction_step(struct CPU* cpu, const uint8_t final_step) {
    cpu->is_final_step = cpu->instruction_step == final_step;
    cpu->instruction_step++;
}

uint8_t* get_r8(struct CPU* cpu, const uint8_t index) {
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
        default: assert(false);
    }
}

uint16_t* get_r16(struct CPU* cpu, const uint8_t index) {
    switch (index) {
        case 0:
            return &cpu->BC;
        case 1:
            return &cpu->DE;
        case 2:
            return &cpu->HL;
        case 3:
            return &cpu->SP;
        default: assert(false);
    }
}

uint16_t get_r16_stack_addr(const struct CPU* cpu, const uint8_t index) {
    switch (index) {
        case 0:
            return cpu->BC;
        case 1:
            return cpu->DE;
        case 2:
            return cpu->HL;
        case 3:
            return cpu->AF;
        default: assert(false);
    }
}

uint16_t get_r16_memaddr(struct CPU* cpu, const uint8_t index) {
    switch (index) {
        case 0:
            return cpu->BC;
        case 1:
            return cpu->DE;
        case 2:
            const auto hli = cpu->HL;
            cpu->HL++;
            return hli;
        case 3:
            const auto hld = cpu->HL;
            cpu->HL--;
            return hld;
        default: assert(false);
    }
}

//
// 8 BIT INSTRUCTIONS
//
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
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->HL);
        case 1:
            const auto target = (cpu->IR >> 3) & 0x7;
            const auto target_register = get_r8(cpu, target);

            *target_register = cpu->TMP;
            break;
        default: break;
    }

    increment_instruction_step(cpu, 1);
}

void load_hl_r8(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            const auto target = cpu->IR & 0x7;
            const auto target_register = get_r8(cpu, target);

            dmg_cpu_write_u8(dmg, cpu, cpu->HL, *target_register);
        case 1:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 1);
}

void load_r8_d8(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
        case 1:
            const auto target = (cpu->IR >> 3) & 0x7;
            const auto target_register = get_r8(cpu, target);

            *target_register = cpu->TMP;
            break;
        default: break;
    }

    increment_instruction_step(cpu, 1);
}

void load_hl_d8(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
        case 1:
            dmg_cpu_write_u8(dmg, cpu, cpu->HL, cpu->TMP);
            break;
        case 2:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 2);
}

//
// 16 BIT INSTRUCTIONS
//
void load_r16_d16(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 1:
            cpu->TMP2 = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 2:
            const auto target = (cpu->IR >> 4) & 0x3;
            const auto target_register = get_r16(cpu, target);

            *target_register = (cpu->TMP2 << 8) | cpu->TMP;
        default:
            break;
    }

    increment_instruction_step(cpu, 2);
}

void load_r16_abs_acc(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            const auto target = (cpu->IR >> 4) & 0x3;
            const auto target_register = get_r16_memaddr(cpu, target);

            dmg_cpu_write_u8(dmg, cpu, target_register, cpu->A);
            break;
        case 1:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 1);
}

void load_acc_r16_abs(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            const auto target = (cpu->IR >> 4) & 0x3;
            const auto target_register = get_r16_memaddr(cpu, target);

            cpu->A = dmg_cpu_read_u8(dmg, cpu, target_register);
            break;
        case 1:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 1);
}

void load_acc_d16_abs(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 1:
            cpu->TMP2 = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 2:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, (cpu->TMP2 << 8) | cpu->TMP);
            break;
        case 3:
            cpu->A = cpu->TMP;
            break;
        default:
            break;
    }

    increment_instruction_step(cpu, 3);
}

void load_d16_abs_acc(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 1:
            cpu->TMP2 = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 2:
            dmg_cpu_write_u8(dmg, cpu, (cpu->TMP2 << 8) | cpu->TMP, cpu->A);
            break;
        case 3:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 3);
}

void load_high_acc_c_indirect(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, 0xFF00 | cpu->C);
            break;
        case 1:
            cpu->A = cpu->TMP;
            break;
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 1);
}

void load_high_c_indirect_acc(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            dmg_cpu_write_u8(dmg, cpu, 0xFF00 | cpu->C, cpu->A);
            break;
        case 1:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 1);
}

void load_high_acc_d8_indirect(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 1:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, 0xFF00 | cpu->TMP);
            break;
        case 2:
            cpu->A = cpu->TMP;
            break;
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 2);
}

void load_high_d8_indirect_acc(struct DMG* dmg, struct CPU* cpu) {
    switch (cpu->instruction_step) {
        case 0:
            cpu->TMP = dmg_cpu_read_u8(dmg, cpu, cpu->PC);
            cpu->PC++;
            break;
        case 1:
            dmg_cpu_write_u8(dmg, cpu, 0xFF00 | cpu->TMP, cpu->A);
            break;
        case 2:
        default:
            // No operation, instruction technically runs here but does nothing
            break;
    }

    increment_instruction_step(cpu, 2);
}

void dmg_cpu_process_instruction(struct DMG* dmg, struct CPU* cpu, const uint8_t instruction) {
    cpu->total_cycles++;
    switch (instruction) {
        case LD_B_B:
        case LD_B_C:
        case LD_B_D:
        case LD_B_E:
        case LD_B_H:
        case LD_B_L:
        case LD_B_A:

        case LD_C_B:
        case LD_C_C:
        case LD_C_D:
        case LD_C_E:
        case LD_C_H:
        case LD_C_L:
        case LD_C_A:

        case LD_D_B:
        case LD_D_C:
        case LD_D_D:
        case LD_D_E:
        case LD_D_H:
        case LD_D_L:
        case LD_D_A:

        case LD_E_B:
        case LD_E_C:
        case LD_E_D:
        case LD_E_E:
        case LD_E_H:
        case LD_E_L:
        case LD_E_A:

        case LD_H_B:
        case LD_H_C:
        case LD_H_D:
        case LD_H_E:
        case LD_H_H:
        case LD_H_L:
        case LD_H_A:

        case LD_L_B:
        case LD_L_C:
        case LD_L_D:
        case LD_L_E:
        case LD_L_H:
        case LD_L_L:
        case LD_L_A:

        case LD_A_B:
        case LD_A_C:
        case LD_A_D:
        case LD_A_E:
        case LD_A_H:
        case LD_A_L:
        case LD_A_A:
            load_r8_r8(dmg, cpu);
            break;

        case LD_B_HL_INDIRECT:
        case LD_C_HL_INDIRECT:
        case LD_D_HL_INDIRECT:
        case LD_E_HL_INDIRECT:
        case LD_H_HL_INDIRECT:
        case LD_L_HL_INDIRECT:
        case LD_A_HL_INDIRECT:
            load_r8_hl(dmg, cpu);
            break;

        case LD_HL_B_INDIRECT:
        case LD_HL_C_INDIRECT:
        case LD_HL_D_INDIRECT:
        case LD_HL_E_INDIRECT:
        case LD_HL_H_INDIRECT:
        case LD_HL_L_INDIRECT:
        case LD_HL_A_INDIRECT:
            load_hl_r8(dmg, cpu);
            break;

        case LD_B_D8:
        case LD_C_D8:
        case LD_D_D8:
        case LD_E_D8:
        case LD_H_D8:
        case LD_L_D8:
        case LD_A_D8:
            load_r8_d8(dmg, cpu);
            break;

        case LD_HL_D8:
            load_hl_d8(dmg, cpu);
            break;

        case LD_BC_D16:
        case LD_DE_D16:
        case LD_HL_D16:
        case LD_SP_D16:
            load_r16_d16(dmg, cpu);
            break;

        case LD_ABS_BC_A:
        case LD_ABS_DE_A:
        case LD_ABS_HLI_A:
        case LD_ABS_HLD_A:
            load_r16_abs_acc(dmg, cpu);
            break;

        case LD_A_ABS_BC:
        case LD_A_ABS_DE:
        case LD_A_ABS_HLI:
        case LD_A_ABS_HLD:
            load_acc_r16_abs(dmg, cpu);
            break;

        case LD_A_ABS_D16:
            load_acc_d16_abs(dmg, cpu);
            break;

        case LD_ABS_D16_A:
            load_d16_abs_acc(dmg, cpu);
            break;

        case LDH_C_A_INDIRECT:
            load_high_c_indirect_acc(dmg, cpu);
            break;
        case LDH_A_C_INDIRECT:
            load_high_acc_c_indirect(dmg, cpu);
            break;

        case LDH_D8_A_DIRECT:
            load_high_d8_indirect_acc(dmg, cpu);
            break;
        case LDH_A_D8_DIRECT:
            load_high_acc_d8_indirect(dmg, cpu);
            break;

        default:
        case NOP:
            cpu->is_final_step = true;
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
