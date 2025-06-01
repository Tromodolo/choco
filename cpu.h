#ifndef CPU_H
#define CPU_H
#include "nes.h"

typedef union
{
    struct {
        uint8_t carry : 1;
        uint8_t zero : 1;
        uint8_t interrupt_disable : 1;
        uint8_t decimal : 1;
        uint8_t break1 : 1;
        uint8_t break2 : 1;
        uint8_t overflow : 1;
        uint8_t negative : 1;
    };
    uint8_t value;
} Flags;

struct CPU {
    uint8_t acc;
    uint8_t x;
    uint8_t y;
    uint16_t pc;
    uint16_t pc_pre;
    uint8_t sp;
    uint8_t waiting_cycles;

    uint8_t read_tmp;

    uint8_t current_instruction;

    uint64_t total_cycles;

    bool is_stopped;

    Flags p;
};

struct CPU* nes_cpu_init(const struct Nes* nes);
void nes_cpu_tick(struct Nes* nes);
void nes_cpu_free(struct CPU* cpu);

#endif //CPU_H
