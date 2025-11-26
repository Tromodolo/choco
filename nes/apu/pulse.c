//
// Created by tromo on 10/24/25.
//

#include "pulse.h"

#include <stdio.h>
#include <stdlib.h>

#include "apu.h"

void update_timer_reset(struct Pulse* pulse);

constexpr uint8_t square_duty_cycles[4][8] = {
    { 0, 1, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 1, 1, 1, 0, 0, 0 },
    { 1, 0, 0, 1, 1, 1, 1, 1 }
};

struct Pulse* pulse_init(bool is_pulse_one) {
    struct Pulse* pulse = malloc(sizeof(struct Pulse));

    pulse->enabled = false;

    pulse->duty_cycle = 0;
    pulse->length_counter_halt = 0;
    pulse->constant = 0;

    pulse->sweep_enabled = 0;
    pulse->sweep_divider_reset = 0;
    pulse->sweep_negate = 0;
    pulse->sweep_shift = 0;
    pulse->sweep_reload = false;
    pulse->sweep_divider = 0;

    pulse->timer = 0;
    pulse->timer_reset = 0;
    pulse->length_counter = 0;

    pulse->envelope_divider = 0;
    pulse->envelope_divider_reset = 0;
    pulse->envelope_loop = false;
    pulse->envelope_start = false;

    pulse->pending_mute = false;

    pulse->duty_cycle_idx = 0;

    pulse->is_pulse_one = is_pulse_one;

    return pulse;
}

void pulse_free(struct Pulse* pulse) {
    free(pulse);
}

void pulse_write_ctrl_one(struct Pulse* pulse, const uint8_t val) {
    pulse->duty_cycle = (val & 0b11000000) >> 6;
    pulse->length_counter_halt = (val & 0b00100000) >> 5;
    pulse->constant = (val & 0b00010000) >> 4;
    pulse->envelope_divider_reset = val & 0b00001111;
    pulse->envelope_loop = pulse->length_counter_halt;
}

void pulse_write_sweep(struct Pulse* pulse, const uint8_t val) {
    pulse->sweep_enabled = (val & 0b10000000) >> 7;
    pulse->sweep_divider_reset = (val & 0b01110000) >> 4;
    pulse->sweep_negate = (val & 0b00001000) >> 3;
    pulse->sweep_shift = val & 0b00000111;
    pulse->sweep_reload = true;
}

void pulse_write_timer_lo(struct Pulse* pulse, const uint8_t val) {
    pulse->timer_lo = val;
    update_timer_reset(pulse);
}

void pulse_write_ctrl_two(struct Pulse* pulse, const uint8_t val) {
    if (pulse->enabled && pulse->length_counter == 0) {
        pulse->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    } else {
        pulse->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    }

    pulse->timer_hi = val & 0b00000111;
    pulse->duty_cycle_idx = 0;
    pulse->envelope_start = false;
    update_timer_reset(pulse);
}

inline void update_timer_reset(struct Pulse* pulse) {
    pulse->timer_reset = pulse->timer_hi << 8 | pulse->timer_lo;
}

void pulse_step(struct Pulse* pulse) {
    int target_change = pulse->timer_reset >> pulse->sweep_shift;
    if (pulse->sweep_negate) {
        // Pulse 1 and Pulse 2 are wired differently
        // Pulse 1 adds ones-complement, pulse 2 adds twos-complement
        if (pulse->is_pulse_one) {
            target_change = -target_change - 1;
        } else {
            target_change = -target_change;
        }
    }

    pulse->sweep_target = pulse->timer_reset + target_change;
    if (pulse-> sweep_target < 0) {
        pulse-> sweep_target = 0;
    }

    if (pulse->timer == 0) {
        pulse->timer = pulse->timer_reset;
        pulse->duty_cycle_idx--;
    } else {
        pulse->timer--;
    }
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
        pulse->envelope_start = true;
        pulse->envelope_decay_level = 15;
        pulse->envelope_divider = pulse->envelope_divider_reset;
    }
}

void pulse_step_length(struct Pulse* pulse) {
    // The Pulse unit doesn't mute immediately when disabled, it sets the length to 0 and then on the next tick mutes
    if (pulse->pending_mute) {
        pulse->length_counter = 0;
        pulse->pending_mute = false;
    }

    if (!pulse->length_counter_halt && pulse->length_counter > 0) {
        pulse->length_counter--;
    }
}

void pulse_step_sweep(struct Pulse* pulse) {
    // Tick sweep divider, if 0 set timer reset to target period
    if (!pulse->sweep_enabled || pulse->sweep_shift == 0) {
        return;
    }

    if (pulse->sweep_divider == 0) {
        pulse->timer_reset = pulse->sweep_target;
        pulse->sweep_divider = pulse->sweep_divider_reset + 1;
    }

    if (pulse->sweep_reload) {
        pulse->sweep_divider = pulse->sweep_divider_reset + 1;
        pulse->sweep_reload = false;
    }

    pulse->sweep_divider--;
}

short pulse_get_sample(const struct Pulse* pulse) {
    const short base_duty_sample = square_duty_cycles[pulse->duty_cycle][pulse->duty_cycle_idx];
    if (!pulse->enabled || pulse->length_counter == 0 || base_duty_sample == 0 ||
        pulse->timer_reset < 8 || pulse->sweep_target >= 0x800) {
        return 0;
    }

    if (pulse->constant) {
        return pulse->envelope_divider_reset;
    }

    return pulse->envelope_decay_level;
}
