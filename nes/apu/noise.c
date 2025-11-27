//
// Created by tromo on 10/24/25.
//

#include "noise.h"

#include <stdio.h>
#include <stdlib.h>

#include "apu.h"

void update_noise_timer_reset(struct Noise* noise);

const uint16_t noise_time_periods[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202,
    254, 380, 508, 762, 1016, 2034, 4068
};

struct Noise* noise_init() {
    struct Noise* noise = malloc(sizeof(struct Noise));

    noise->enabled = false;

    noise->length_counter_halt = 0;
    noise->constant = 0;

    noise->timer = 0;
    noise->timer_reset = 0;
    noise->length_counter = 0;

    noise->shift_register = 1;

    noise->mode_set = false;

    noise->envelope_divider = 0;
    noise->envelope_divider_reset = 0;
    noise->envelope_loop = false;
    noise->envelope_start = false;

    noise->pending_mute = false;

    return noise;
}

void noise_free(struct Noise* noise) {
    free(noise);
}

void noise_write_ctrl(struct Noise* noise, uint8_t val) {
    noise->length_counter_halt = (val & 0b00100000) >> 5;
    noise->constant = (val & 0b00010000) >> 4;
    noise->envelope_divider_reset = val & 0b00001111;
    noise->envelope_loop = noise->length_counter_halt;
}

void noise_write_mode_and_timer(struct Noise* noise, uint8_t val) {
    noise->mode_set = (val & 0b10000000) >> 7;
    noise->timer_reset = noise_time_periods[val & 0b00001111];
}

void noise_write_length(struct Noise* noise, uint8_t val) {
    if (noise->enabled && noise->length_counter == 0) {
        noise->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    } else {
        noise->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    }

    noise->envelope_start = true;
}

void noise_step(struct Noise* noise) {
    if (noise->timer == 0) {
        noise->timer = noise->timer_reset;

        const uint8_t selected_bit = noise->mode_set
            ? (noise->shift_register & 0b01000000) >> 6
            : (noise->shift_register & 0b00000010) >> 1;
        const uint8_t feedback = (noise->shift_register & 1) ^ selected_bit;
        noise->shift_register >>= 1;
        noise->shift_register |= (feedback << 14);
    } else {
        noise->timer--;
    }
}

void noise_step_envelope(struct Noise* noise) {
    if (noise->envelope_start) {
        if (noise->envelope_divider == 0) {
            noise->envelope_divider = noise->envelope_divider_reset;

            if (noise->envelope_decay_level == 0) {
                if (noise->envelope_loop) {
                    noise->envelope_decay_level = 15;
                }
            } else {
                noise->envelope_decay_level--;
            }
        } else {
            noise->envelope_divider--;
        }
    } else {
        noise->envelope_start = true;
        noise->envelope_decay_level = 15;
        noise->envelope_divider = noise->envelope_divider_reset;
    }
}

void noise_step_length(struct Noise* noise) {
    // The Noise unit doesn't mute immediately when disabled, it sets the length to 0 and then on the next tick mutes
    if (noise->pending_mute) {
        noise->length_counter = 0;
        noise->pending_mute = false;
    }

    if (!noise->length_counter_halt && noise->length_counter > 0) {
        noise->length_counter--;
    }
}

short noise_get_sample(const struct Noise* noise) {
    const short base_sample = noise->shift_register & 1;
    if (!noise->enabled || noise->length_counter == 0 || base_sample == 0) {
        return 0;
    }

    if (noise->constant) {
        return noise->envelope_divider_reset;
    }

    return noise->envelope_decay_level;
}
