#include <stdlib.h>
#include <stdint.h>

#include "cpu.h"

#include "nes.h"
#include "instructions.h"
#include "nes-logging.h"

struct CPU* nes_cpu_init(const struct Nes* nes) {
    struct CPU* cpu = malloc(sizeof(struct CPU));

    cpu->acc = 0;
    cpu->x = 0;
    cpu->y = 0;

    cpu->pc = nes_read_short(nes, 0xFFFC);
    cpu->sp = 0xFD;

    cpu->p.value = 0;
    cpu->p.interrupt_disable = 1;
    cpu->p.break2 = 1;

    cpu->total_cycles = 7;
    cpu->waiting_cycles = 0;
    cpu->is_stopped = false;

    cpu->current_instruction = 0;
    cpu->read_tmp = 0;
    cpu->can_page_cross = false;
    cpu->current_addressing_mode = Addressing_NoneAddressing;

    // if(getenv("NESTEST")) {
         //cpu->pc = 0xC000;
    // }

    return cpu;
}

void nes_cpu_tick(struct Nes* nes) {
    if (!nes->cpu->is_stopped && nes->cpu->waiting_cycles == 0) {
        // if (nes->cpu->total_cycles <= 26554)
            // write_current_status_log(nes);

        // if (nes->cpu->pc == 0xC28F) {
        //     int x = 5;
        //     x++;
        // }

        const uint8_t opcode = nes_read_char(nes, nes->cpu->pc++);
        nes_cpu_handle_instruction(nes, nes->cpu, opcode);

        nes->cpu->total_cycles += nes->cpu->waiting_cycles;
    }
    nes->cpu->waiting_cycles--;
}

void nes_cpu_free(struct CPU* cpu) {
    free(cpu);
}