//
// Created by tromo on 6/7/25.
//

#ifndef APU_H
#define APU_H

#include "pulse.h"

typedef union {
    struct {
        uint8_t volume : 4;
        uint8_t disable : 1;
        uint8_t loop : 1;
        uint8_t duty : 2;
    };
    uint8_t value;
} SquareRegCtrl;

typedef union {
    struct {
        uint8_t shift : 3;
        uint8_t negative : 1;
        uint8_t period : 3;
        uint8_t enable_sweep : 1;
    };
    uint8_t value;
} SquareRegSweep;

typedef union {
    struct {
        uint8_t length_index : 5;
        uint8_t period_hi : 3;
    };
    uint8_t value;
} SquareRegTimerHi;

struct APU {
    struct Pulse* pulse_one;
    struct Pulse* pulse_two;

    // State
    uint16_t frame_counter;
    bool do_tick;
    bool is_five_step;
    uint16_t last_sample;
};

struct APU* apu_init(struct Nes* nes);
void apu_free(struct APU* apu);
void apu_write(struct APU* apu, uint16_t addr, uint8_t val);
void apu_tick(struct APU* apu);
short apu_read_latest_sample(struct APU* apu);

#endif //APU_H
