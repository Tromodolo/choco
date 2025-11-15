//
// Created by tromo on 10/24/25.
//

#include "pulse.h"

constexpr uint8_t square_duty_cycles[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

struct Pulse* pulse_init() {

}

void pulse_free(struct Pulse* pulse) {

}