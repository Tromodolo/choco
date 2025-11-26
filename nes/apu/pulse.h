//
// Created by tromo on 10/24/25.
//

#ifndef PULSE_H
#define PULSE_H
#include <stdint.h>

struct Pulse {
    bool enabled;

    uint8_t duty_cycle : 2;
    uint8_t length_counter_halt : 1;
    uint8_t constant : 1;

    uint8_t sweep_enabled : 1;
    uint8_t sweep_divider_reset : 3;
    uint8_t sweep_negate : 1;
    uint8_t sweep_shift : 3;
    bool sweep_reload;
    int sweep_target;
    uint8_t sweep_divider;

    uint8_t timer_lo : 8;
    uint8_t timer_hi : 3;

    uint8_t envelope_decay_level : 4;
    uint8_t envelope_divider; // Volume
    uint8_t envelope_divider_reset : 4; // Volume
    bool envelope_loop;
    bool envelope_start;

    uint16_t timer;
    uint16_t timer_reset;
    uint8_t length_counter;

    bool pending_mute;

    uint8_t duty_cycle_idx : 3;

    bool is_pulse_one;
};

struct Pulse* pulse_init(bool is_pulse_one);
void pulse_free(struct Pulse* pulse);

void pulse_write_ctrl_one(struct Pulse* pulse, const uint8_t val); // 4000
void pulse_write_sweep(struct Pulse* pulse, const uint8_t val); // 4001
void pulse_write_timer_lo(struct Pulse* pulse, const uint8_t val); // 4002
void pulse_write_ctrl_two(struct Pulse* pulse, const uint8_t val); // 4003

void pulse_step(struct Pulse* pulse);
void pulse_step_envelope(struct Pulse* pulse);
void pulse_step_length(struct Pulse* pulse);
void pulse_step_sweep(struct Pulse* pulse);
short pulse_get_sample(const struct Pulse* pulse);

#endif //PULSE_H
