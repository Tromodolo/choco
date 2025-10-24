//
// Created by tromo on 6/7/25.
//

#ifndef APU_H
#define APU_H

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
    // Channels
    // Square 1
    SquareRegCtrl square_1_ctrl;
    SquareRegSweep square_1_sweep;
    uint8_t square_1_period_lo;
    SquareRegTimerHi square_1_period_hi;

    uint16_t square_1_sample;

    // State
    uint16_t last_sample;
};

struct APU* apu_init(struct Nes* nes);
void apu_write(struct APU* apu, uint16_t addr, uint8_t val);
void apu_tick(struct APU* apu);
short apu_read_latest_sample(struct APU* apu);

#endif //APU_H
