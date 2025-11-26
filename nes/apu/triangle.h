#ifndef TRIANGLE_H
#define TRIANGLE_H
#include <stdint.h>

struct Triangle {
    bool enabled;

    uint8_t control_flag : 1;

    uint8_t timer_lo : 8;
    uint8_t timer_hi : 3;

    uint16_t timer;
    uint16_t timer_reset;

    uint8_t length_counter;

    uint8_t linear_counter;
    uint8_t linear_counter_reset;
    bool linear_counter_reload;

    bool pending_mute;

    uint8_t step_cycle_idx : 5;
};

struct Triangle* triangle_init();
void triangle_free(struct Triangle* triangle);

void triangle_write_linear_counter(struct Triangle* triangle, const uint8_t val); // 4008
void triangle_write_timer_lo(struct Triangle* triangle, const uint8_t val); // 400A
void triangle_write_timer_hi_and_length(struct Triangle* triangle, const uint8_t val); // 400B

void triangle_step(struct Triangle* triangle);
void triangle_step_length(struct Triangle* triangle);
void triangle_step_linear_counter(struct Triangle* triangle);
short triangle_get_sample(const struct Triangle* triangle);

#endif //TRIANGLE_H
