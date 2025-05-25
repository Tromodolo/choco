//
// Created by tromo on 5/25/25.
//

#include <stdlib.h>

#include "cpu.h"
#include "nes.h"
#include "nes-internal.h"

struct CPU* nes_cpu_init(struct Nes* nes) {
    struct CPU* cpu = malloc(sizeof(struct CPU));

    cpu->acc = 0;
    cpu->x = 0;
    cpu->y = 0;

    cpu->pc = nes_read_short(nes, 0xFFFC);
    cpu->sp = 0xFD;

    cpu->p.value = 0;
    cpu->p.flags.interrupt_disable = 1;

    return cpu;
}

void nes_cpu_tick(struct Nes* nes) {

}

void nes_cpu_free(struct CPU* cpu) {
    free(cpu);
}