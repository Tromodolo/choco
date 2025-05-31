//
// Created by tromo on 5/25/25.
//

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include <stdint.h>
#include "nes.h"

void nes_cpu_handle_instruction(struct Nes* nes, struct CPU* cpu, uint8_t opcode);

#endif //INSTRUCTIONS_H
