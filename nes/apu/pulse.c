//
// Created by tromo on 10/24/25.
//

#include "pulse.h"

#include <stdlib.h>

constexpr uint8_t square_duty_cycles[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

struct Pulse* pulse_init() {
    struct Pulse* pulse = malloc(sizeof(struct Pulse));

    pulse->enabled = false;

    pulse->duty_cycle = 0;
    pulse->length_counter_halt = 0;
    pulse->constant = 0;

    pulse->sweep_enabled = 0;
    pulse->sweep_period = 0;
    pulse->sweep_negate = 0;
    pulse->sweep_shift = 0;

    pulse->timer = 0;
    pulse->timer_reset = 0;
    pulse->length_counter = 0;

    pulse->envelope_divider = 0;
    pulse->envelope_divider_reset = 0;
    pulse->envelope_loop = false;
    pulse->envelope_start = false;

    pulse->duty_cycle_idx = 0;

    return pulse;
}

void pulse_free(struct Pulse* pulse) {
    free(pulse);
}

void pulse_update_timer_reset(struct Pulse* pulse) {
    pulse->timer_reset = pulse->timer_hi << 8 | pulse->timer_lo;
}

void pulse_step(struct Pulse* pulse) {
    if (pulse->timer <= 0) {
        pulse->timer = pulse->timer_reset;
        pulse->duty_cycle_idx--;
    }

    pulse->timer--;
}

void pulse_step_envelope(struct Pulse* pulse) {
    if (pulse->envelope_start) {
        if (pulse->envelope_divider == 0) {
            pulse->envelope_divider = pulse->envelope_divider_reset;

            if (pulse->envelope_decay_level == 0) {
                if (pulse->envelope_loop) {
                    pulse->envelope_decay_level = 15;
                }
            } else {
                pulse->envelope_decay_level--;
            }
        } else {
            pulse->envelope_divider--;
        }
    } else {
        pulse->envelope_start = false;
        pulse->envelope_decay_level = 15;
        pulse->envelope_divider = pulse->envelope_divider_reset;
    }
}

void pulse_step_length(struct Pulse* pulse) {
    if (!pulse->length_counter_halt && pulse->length_counter > 0) {
        pulse->length_counter--;
    }
}

short pulse_get_sample(const struct Pulse* pulse) {
    const short base_duty_sample = square_duty_cycles[pulse->duty_cycle][pulse->duty_cycle_idx];
    if (!pulse->enabled || pulse->length_counter == 0 || base_duty_sample == 0) {
        return 0;
    }

    if (pulse->constant) {
        return pulse->envelope_divider_reset;
    }

    return pulse->envelope_decay_level;
}
