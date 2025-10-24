//
// Created by tromo on 6/7/25.
//

#include "../nes.h"
#include "apu.h"

#include <stdlib.h>

constexpr uint8_t square_duty_cycles[] = {
    0b01000000,
    0b01100000,
    0b01111000,
    0b10011111,
};

struct APU* apu_init(struct Nes* nes) {
    struct APU* apu = malloc(sizeof(struct APU));

    apu->square_1_ctrl.value = 0;
    apu->square_1_period_hi.value = 0;
    apu->square_1_sweep.value = 0;
    apu->square_1_period_lo = 0;

    apu->square_1_sample = 0;

    apu->last_sample = 0;

    return apu;
}

void apu_write(struct APU* apu, const uint16_t addr, const uint8_t val) {
    switch (addr) {
        case 0x4000:
            apu->square_1_ctrl.value = val;
            break;
        case 0x4001:
            apu->square_1_sweep.value = val;
            break;
        case 0x4002:
            apu->square_1_period_lo = val;
            break;
        case 0x4003:
            apu->square_1_period_hi.value = val;
            break;
    }
}

void apu_tick(struct APU* apu) {

}

short apu_read_latest_sample(struct APU* apu) {
    // Using linear approximation because it is simpler
    // https://www.nesdev.org/wiki/APU_Mixer
    // Possiblility to use more accurate mixer logic in the future but this will have to do

    const float pulse1 = apu->square_1_sample;
    constexpr float pulse2 = 0;
    constexpr float triangle = 0;
    constexpr float noise = 0;
    constexpr float dmc = 0;

    const float pulse_out = 0.00752f * (pulse1 + pulse2);
    constexpr float tnd_out = 0.00851f * triangle + 0.00494f * noise + 0.00335f * dmc;
    const short output = (short)(pulse_out + tnd_out);
    return output;
}
