#ifndef CPU_H
#define CPU_H
#include "../nes.h"

enum AddressingMode {
    Addressing_Immediate,
    Addressing_ZeroPage,
    Addressing_ZeroPageX,
    Addressing_ZeroPageY,
    Addressing_Accumulator,
    Addressing_Absolute,
    Addressing_AbsoluteX,
    Addressing_AbsoluteY,
    Addressing_Indirect,
    Addressing_IndirectX,
    Addressing_IndirectY,
    Addressing_Relative,
    Addressing_NoneAddressing,
};

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

    uint8_t current_instruction;
    enum AddressingMode current_addressing_mode;
    bool can_page_cross;
    uint8_t read_tmp;
    bool did_branch;

    uint64_t total_cycles;

    bool is_stopped;
    bool ready;

    Flags p;

    bool dma_read_write_latch;
    bool is_dma_active;
    bool dma_just_started;
    uint8_t dma_page;
    uint8_t dma_addr;
    uint8_t dma_value;
};

struct CPU* nes_cpu_init(struct Nes* nes);
void nes_cpu_tick(struct Nes* nes);
void nes_cpu_free(struct CPU* cpu);

#endif //CPU_H
