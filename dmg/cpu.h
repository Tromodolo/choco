//
// Created by tromo on 3/18/26.
//

#ifndef CPU_H
#define CPU_H
#include "stdint.h"

struct DMG;

enum CPU_Flags {
    Flag_Carry          = 1 << 4,
    Flag_Subtraction    = 1 << 5,
    Flag_Half_Carry     = 1 << 6,
    Flag_Zero           = 1 << 7
};

enum LastOperation {
    Operation_None = 0,
    Operation_Read = 1,
    Operation_Write = 2
};

struct CPU {
    union {
        struct {
            uint8_t F;
            uint8_t A;
        };
        uint16_t AF;
    };

    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };

    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };

    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };

    uint16_t SP;
    uint16_t PC;
    uint8_t IR;
    uint8_t TMP;

    enum LastOperation last_operation;
    unsigned long long total_cycles;
    unsigned int instruction_step;
    bool is_final_step;
};

struct CPU* dmg_cpu_init(struct DMG* dmg);
void dmg_cpu_fetch_next_instruction(struct DMG* dmg, struct CPU* cpu);
void dmg_cpu_tick_m_cycle(struct DMG* dmg, struct CPU* cpu);
void dmg_cpu_process_instruction(struct DMG* dmg, struct CPU* cpu, uint8_t instruction);
void dmg_cpu_free(struct CPU* cpu);

#endif //CPU_H
