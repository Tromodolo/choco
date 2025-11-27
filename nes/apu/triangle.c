//
// Created by tromo on 10/24/25.
//

#include "triangle.h"

#include <stdio.h>
#include <stdlib.h>

#include "apu.h"

void update_triangle_timer_reset(struct Triangle* triangle);

constexpr uint8_t triangle_cycle[32] = {
    15, 14, 13, 12, 11, 10, 9,  8,  7,  6, 5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

struct Triangle* triangle_init() {
    struct Triangle* triangle = malloc(sizeof(struct Triangle));

    triangle->enabled = false;

    triangle->control_flag = 0;

    triangle->timer_lo = 0;
    triangle->timer_hi = 0;

    triangle->timer = 0;
    triangle->timer_reset= 0;
    triangle->length_counter = 0;

    triangle->linear_counter = 0;
    triangle->linear_counter_reset = 0;
    triangle->linear_counter_reload = false;

    triangle->pending_mute = false;

    triangle->step_cycle_idx = 0;

    return triangle;
}

void triangle_free(struct Triangle* triangle) {
    free(triangle);
}

void triangle_write_linear_counter(struct Triangle* triangle, const uint8_t val) {
    triangle->control_flag = (val & 0b10000000) >> 7;
    triangle->linear_counter_reset = val & 0b011111111;
}

void triangle_write_timer_lo(struct Triangle* triangle, const uint8_t val) {
    triangle->timer_lo = val;
    update_triangle_timer_reset(triangle);
}

void triangle_write_timer_hi_and_length(struct Triangle* triangle, const uint8_t val) {
    if (triangle->enabled && triangle->length_counter == 0) {
        triangle->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    } else {
        triangle->length_counter = apu_length_lookup_table[(val & 0b11111000) >> 3];
    }

    triangle->timer_hi = val & 0b00000111;
    triangle->linear_counter_reload = true;
    update_triangle_timer_reset(triangle);
}

inline void update_triangle_timer_reset(struct Triangle* triangle) {
    triangle->timer_reset = triangle->timer_hi << 8 | triangle->timer_lo;
}

void triangle_step(struct Triangle* triangle) {
    if (!triangle->enabled || triangle->length_counter == 0 || triangle->linear_counter == 0) {
        return;
    }

    if (triangle->timer == 0) {
        triangle->timer = triangle->timer_reset;
        triangle->step_cycle_idx--;
    } else {
        triangle->timer--;
    }
}

void triangle_step_length(struct Triangle* triangle) {
    // The Triangle unit doesn't mute immediately when disabled, it sets the length to 0 and then on the next tick mutes
    if (triangle->pending_mute) {
        triangle->linear_counter = 0;
        triangle->pending_mute = false;
    }

    if (!triangle->control_flag && triangle->length_counter > 0) {
        triangle->length_counter--;
    }
}

void triangle_step_linear_counter(struct Triangle* triangle) {
    if (triangle->linear_counter_reload) {
        triangle->linear_counter = triangle->linear_counter_reset;
    } else if (triangle->linear_counter != 0) {
        triangle->linear_counter--;
    }

    if (!triangle->control_flag) {
        triangle->linear_counter_reload = false;
    }
}

short triangle_get_sample(const struct Triangle* triangle) {
    const short sample_step = triangle_cycle[triangle->step_cycle_idx];
    if (!triangle->enabled || sample_step == 0) {
        return 0;
    }

    return sample_step;
}
