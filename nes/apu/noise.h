//
// Created by tromo on 10/24/25.
//

#ifndef NOISE_H
#define NOISE_H
#include <stdint.h>

struct Noise {
    bool enabled;

    uint8_t length_counter_halt : 1;
    uint8_t constant : 1;

    uint8_t envelope_decay_level : 4;
    uint8_t envelope_divider; // Volume
    uint8_t envelope_divider_reset : 4; // Volume
    bool envelope_loop;
    bool envelope_start;

    bool mode_set;

    uint16_t shift_register : 16;

    uint16_t timer;
    uint16_t timer_reset;
    uint8_t length_counter;

    bool pending_mute;
};

struct Noise* noise_init();
void noise_free(struct Noise* noise);

void noise_write_ctrl(struct Noise* noise, uint8_t val);
void noise_write_mode_and_timer(struct Noise* noise, uint8_t val);
void noise_write_length(struct Noise* noise, uint8_t val);

void noise_step(struct Noise* noise);
void noise_step_envelope(struct Noise* noise);
void noise_step_length(struct Noise* noise);
short noise_get_sample(const struct Noise* noise);

#endif //NOISE_H
