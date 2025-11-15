//
// Created by tromo on 10/24/25.
//

#ifndef PULSE_H
#define PULSE_H
#include <stdint.h>

struct Pulse {
    uint8_t duty_cycle;
    uint8_t halt;
    uint8_t constant;
    uint8_t volume;

    uint8_t sweep_enabled;
    uint8_t sweep_period;
    uint8_t sweep_negate;
    uint8_t sweep_shift;

    uint16_t timer;
    uint8_t length;

    uint8_t sample;
};

struct Pulse* pulse_init();
void pulse_free(struct Pulse* pulse);

#endif //PULSE_H
