//
// Created by tromo on 5/25/25.
//

#ifndef CPU_H
#define CPU_H

struct Nes;
struct CPU;

struct CPU* nes_cpu_init(struct Nes* nes);
void nes_cpu_tick(struct Nes* nes);
void nes_cpu_free(struct CPU* cpu);

#endif //CPU_H
