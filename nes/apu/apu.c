//
// Created by tromo on 6/7/25.
//

#include "../nes.h"
#include "apu.h"

#include <stdio.h>
#include <stdlib.h>

void do_frame_counter(struct APU* apu);

constexpr uint8_t length_lookup_table[] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

constexpr int emulator_output_volume = 1000;

float pulse_lookup_table[31] = {0};

float tnd_lookup_table[203] = {0};

struct APU* apu_init(struct Nes* nes) {
    struct APU* apu = malloc(sizeof(struct APU));

    // Populates the lookup tables while we're at it
    for (int i = 0; i < 31; ++i) {
        pulse_lookup_table[i] = 95.52f / (8128.0f / (float)(i + 100));
    }

    for (int i = 0; i < 203; ++i) {
        tnd_lookup_table[i] = 163.67f / (24329.0f / (float)(i + 100));
    }

    apu->pulse_one = pulse_init(true);
    apu->pulse_two = pulse_init(false);

    apu->frame_counter = 0;
    apu->last_sample = 0;
    apu->do_tick = true;

    apu->is_five_step = false;
    apu->irq_inhibit = false;

    return apu;
}

void apu_free(struct APU* apu) {
    pulse_free(apu->pulse_one);
    pulse_free(apu->pulse_two);
    free(apu);
}

void apu_write(struct APU* apu, const uint16_t addr, const uint8_t val) {
    switch (addr) {
        case 0x4000:
            apu->pulse_one->duty_cycle = (val & 0b11000000) >> 6;
            apu->pulse_one->length_counter_halt = (val & 0b00100000) >> 5;
            apu->pulse_one->constant = (val & 0b00010000) >> 4;
            apu->pulse_one->envelope_divider_reset = val & 0b00001111;
            apu->pulse_one->envelope_loop = apu->pulse_one->length_counter_halt;
            break;
        case 0x4001:
            apu->pulse_one->sweep_enabled = (val & 0b10000000) >> 7;
            apu->pulse_one->sweep_divider_reset = (val & 0b01110000) >> 4;
            apu->pulse_one->sweep_negate = (val & 0b00001000) >> 3;
            apu->pulse_one->sweep_shift = val & 0b00000111;
            apu->pulse_one->sweep_reload = true;
            break;
        case 0x4002:
            apu->pulse_one->timer_lo = val;
            break;
        case 0x4003:
            if (apu->pulse_one->enabled && apu->pulse_one->length_counter == 0) {
                apu->pulse_one->length_counter = length_lookup_table[(val & 0b11111000) >> 3];
            } else {
                apu->pulse_one->length_counter = length_lookup_table[(val & 0b11111000) >> 3];
            }

            apu->pulse_one->timer_hi = val & 0b00000111;
            apu->pulse_one->duty_cycle_idx = 0;
            apu->pulse_one->envelope_start = false;
            pulse_update_timer_reset(apu->pulse_one);
            break;
        case 0x4004:
            apu->pulse_two->duty_cycle = (val & 0b11000000) >> 6;
            apu->pulse_two->length_counter_halt = (val & 0b00100000) >> 5;
            apu->pulse_two->constant = (val & 0b00010000) >> 4;
            apu->pulse_two->envelope_divider_reset = val & 0b00001111;
            apu->pulse_two->envelope_loop = apu->pulse_two->length_counter_halt;
            break;
        case 0x4005:
            apu->pulse_two->sweep_enabled = (val & 0b10000000) >> 7;
            apu->pulse_two->sweep_divider_reset = (val & 0b01110000) >> 4;
            apu->pulse_two->sweep_negate = (val & 0b00001000) >> 3;
            apu->pulse_two->sweep_shift = val & 0b00000111;
            apu->pulse_two->sweep_reload = true;
            break;
        case 0x4006:
            apu->pulse_two->timer_lo = val;
            break;
        case 0x4007:
            if (apu->pulse_two->enabled && apu->pulse_two->length_counter == 0) {
                apu->pulse_two->length_counter = length_lookup_table[(val & 0b11111000) >> 3];
            } else {
                apu->pulse_two->length_counter = length_lookup_table[(val & 0b11111000) >> 3];
            }

            apu->pulse_two->timer_hi = val & 0b00000111;
            apu->pulse_two->duty_cycle_idx = 0;
            apu->pulse_two->envelope_start = false;
            pulse_update_timer_reset(apu->pulse_two);
            break;
        case 0x4015:
            apu->pulse_one->enabled = val & 0b1;
            apu->pulse_two->enabled = (val & 0b10) >> 1;

            if (!apu->pulse_one->enabled) {
                apu->pulse_one->length_counter = 0;
            }

            if (!apu->pulse_one->enabled) {
                apu->pulse_two->length_counter = 0;
            }

            break;
        case 0x4017:
            apu->is_five_step = (val & 0b10000000) >> 7;
            apu->irq_inhibit = (val & 0b01000000) >> 6;
            break;
    }
}

inline void do_frame_counter(struct APU* apu) {
    switch (apu->frame_counter) {
        case 3728:  // clock envelopes, triangle
            pulse_step_envelope(apu->pulse_one);

            pulse_step_envelope(apu->pulse_two);
            break;
        case 7456:  // clock envelopes, triangle, length, sweep
            pulse_step_envelope(apu->pulse_one);
            pulse_step_length(apu->pulse_one);
            pulse_step_sweep(apu->pulse_one);

            pulse_step_envelope(apu->pulse_two);
            pulse_step_length(apu->pulse_two);
            pulse_step_sweep(apu->pulse_two);
            break;
        case 11185: // clock envelopes, triangle
            pulse_step_envelope(apu->pulse_one);

            pulse_step_envelope(apu->pulse_two);
            break;
        case 14914: // 4-Step final envelopes, triangle, length, sweep
            if (!apu->is_five_step) {
                pulse_step_envelope(apu->pulse_one);
                pulse_step_length(apu->pulse_one);
                pulse_step_sweep(apu->pulse_one);

                pulse_step_envelope(apu->pulse_two);
                pulse_step_length(apu->pulse_two);
                pulse_step_sweep(apu->pulse_two);
            }
            break;
        case 14915: // 4-Step 0-frame
            if (!apu->is_five_step) {
                apu->frame_counter = 0;
                // TODO: Handle frame interrupt
            }
            break;
        case 18640: // 5-Step final envelopes, triangle, length, sweep
            if (apu->is_five_step) {
                pulse_step_envelope(apu->pulse_one);
                pulse_step_length(apu->pulse_one);
                pulse_step_sweep(apu->pulse_one);

                pulse_step_envelope(apu->pulse_two);
                pulse_step_length(apu->pulse_two);
                pulse_step_sweep(apu->pulse_two);
            }
            break;
        case 18641: // 5-step 0-frame
            if (apu->is_five_step) {
                apu->frame_counter = 0;
                // TODO: Handle frame interrupt
            }
            break;
        default: break;
    }

    apu->frame_counter++;
}

void apu_tick(struct APU* apu) {
    if (apu->do_tick) {
        do_frame_counter(apu);

        pulse_step(apu->pulse_one);
        pulse_step(apu->pulse_two);
    }

    apu->do_tick = !apu->do_tick;
}

short apu_read_latest_sample(struct APU* apu) {
    const short pulse1 = pulse_get_sample(apu->pulse_one);
    const short pulse2 = pulse_get_sample(apu->pulse_two);
    constexpr short triangle = 0;
    constexpr short noise = 0;
    constexpr short dmc = 0;

    const short pulse_out = pulse1 + pulse2;
    constexpr short tnd_out = triangle * 3 + noise * 2 + dmc;
    const float result = pulse_lookup_table[pulse_out] + tnd_lookup_table[tnd_out];

    return (short)(result * 32767.0f);
}
