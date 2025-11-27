//
// Created by tromo on 6/7/25.
//

#include "../nes.h"
#include "apu.h"

#include <stdio.h>
#include <stdlib.h>

void do_frame_counter(struct APU* apu);
void step_envelope_and_triangle(struct APU* apu);
void step_length_and_sweep(struct APU* apu);
void apu_update_samples(struct APU* apu, uint64_t cycle_count);

constexpr int emulator_output_volume = 1000;

float pulse_lookup_table[31] = {0};

float tnd_lookup_table[203] = {0};

struct APU* apu_init(struct Nes* nes) {
    struct APU* apu = malloc(sizeof(struct APU));

    // Populates the lookup tables while we're at it
    for (int i = 0; i < 31; ++i) {
        pulse_lookup_table[i] = 95.52f / (8128.0f / i + 100);
    }

    for (int i = 0; i < 203; ++i) {
        tnd_lookup_table[i] = 163.67f / (24329.0f / i + 100);
    }

    apu->pulse_one = pulse_init(true);
    apu->pulse_two = pulse_init(false);
    apu->triangle = triangle_init();

    apu->frame_counter = 0;
    apu->do_tick = true;

    apu->last_pulse_sample = 0;
    apu->last_tnd_sample = 0;

    apu->is_five_step = false;
    apu->irq_inhibit = false;

    apu->blip_pulse = blip_new(4096);
    blip_set_rates(apu->blip_pulse, GLOBAL_CLOCKS_PER_SECOND, AUDIO_SAMPLE_RATE);

    apu->blip_tnd = blip_new(4096);
    blip_set_rates(apu->blip_tnd, GLOBAL_CLOCKS_PER_SECOND, AUDIO_SAMPLE_RATE);

    return apu;
}

void apu_free(struct APU* apu) {
    pulse_free(apu->pulse_one);
    pulse_free(apu->pulse_two);
    triangle_free(apu->triangle);
    free(apu);
}

void apu_write(struct APU* apu, const uint16_t addr, const uint8_t val) {
    switch (addr) {
        case 0x4000:
            pulse_write_ctrl_one(apu->pulse_one, val);
            break;
        case 0x4001:
            pulse_write_sweep(apu->pulse_one, val);
            break;
        case 0x4002:
            pulse_write_timer_lo(apu->pulse_one, val);
            break;
        case 0x4003:
            pulse_write_ctrl_two(apu->pulse_one, val);
            break;
        case 0x4004:
            pulse_write_ctrl_one(apu->pulse_two, val);
            break;
        case 0x4005:
            pulse_write_sweep(apu->pulse_two, val);
            break;
        case 0x4006:
            pulse_write_timer_lo(apu->pulse_two, val);
            break;
        case 0x4007:
            pulse_write_ctrl_two(apu->pulse_two, val);
            break;
        case 0x4008:
            triangle_write_linear_counter(apu->triangle, val);
            break;
        case 0x400A:
            triangle_write_timer_lo(apu->triangle, val);
            break;
        case 0x400B:
            triangle_write_timer_hi_and_length(apu->triangle, val);
            break;
        case 0x4015:
            apu->pulse_one->enabled = val & 0b1;
            apu->pulse_two->enabled = (val & 0b10) >> 1;
            apu->triangle->enabled = (val & 0b100) >> 2;

            if (!apu->pulse_one->enabled) {
                apu->pulse_one->pending_mute = true;
            }

            if (!apu->pulse_one->enabled) {
                apu->pulse_two->pending_mute = true;
            }

            if (!apu->triangle->enabled) {
                apu->triangle->pending_mute = true;
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
            step_envelope_and_triangle(apu);
            break;
        case 7456:  // clock envelopes, triangle, length, sweep
            step_envelope_and_triangle(apu);
            step_length_and_sweep(apu);
            break;
        case 11185: // clock envelopes, triangle
            step_envelope_and_triangle(apu);
            break;
        case 14914: // 4-Step final envelopes, triangle, length, sweep
            if (!apu->is_five_step) {
                step_envelope_and_triangle(apu);
                step_length_and_sweep(apu);
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
                step_envelope_and_triangle(apu);
                step_length_and_sweep(apu);
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

inline void step_envelope_and_triangle(struct APU* apu) {
    pulse_step_envelope(apu->pulse_one);
    pulse_step_envelope(apu->pulse_two);
    triangle_step_linear_counter(apu->triangle);
}

inline void step_length_and_sweep(struct APU* apu) {
    pulse_step_length(apu->pulse_one);
    pulse_step_sweep(apu->pulse_one);

    pulse_step_length(apu->pulse_two);
    pulse_step_sweep(apu->pulse_two);

    triangle_step_length(apu->triangle);
}

void apu_tick(struct APU* apu, uint64_t global_cycle_count) {
    if (apu->do_tick) {
        do_frame_counter(apu);

        pulse_step(apu->pulse_one);
        pulse_step(apu->pulse_two);
    }
    triangle_step(apu->triangle);

    apu->do_tick = !apu->do_tick;

    apu_update_samples(apu, global_cycle_count);
}

void apu_update_samples(struct APU* apu, uint64_t cycle_count) {
    const short pulse1 = pulse_get_sample(apu->pulse_one);
    const short pulse2 = pulse_get_sample(apu->pulse_two);
    const short triangle = triangle_get_sample(apu->triangle);
    constexpr short noise = 0;
    constexpr short dmc = 0;

    const short pulse_out = (short)(pulse1 + pulse2) * 500;
    const short pulse_diff = pulse_out - apu->last_pulse_sample;
    if (pulse_diff != 0) {
        apu->last_pulse_sample = pulse_out;
        blip_add_delta(apu->blip_pulse, cycle_count, pulse_diff);
    }

    const short tnd_out = (short)(triangle + noise + dmc) * 400;
    const short tnd_diff = tnd_out - apu->last_tnd_sample;
    if (tnd_out != 0) {
        apu->last_tnd_sample = tnd_out;
        blip_add_delta(apu->blip_tnd, cycle_count, tnd_diff);
    }
}

uint16_t apu_num_clocks_for_sample_count(struct APU* apu, uint16_t sample_count) {
    return blip_clocks_needed(apu->blip_pulse, sample_count);
}

void apu_read_samples(struct APU* apu, short* buffer, uint16_t sample_count, uint64_t cycle_count) {
    blip_end_frame(apu->blip_pulse, cycle_count);
    blip_end_frame(apu->blip_tnd, cycle_count);

    short pulse_temp[sample_count];
    const uint16_t num_blip_samples = blip_read_samples(apu->blip_pulse, pulse_temp, sample_count, 0);

    short tnd_temp[sample_count];
    blip_read_samples(apu->blip_tnd, tnd_temp, sample_count, 0);

    if (num_blip_samples < sample_count) {
        printf("Lagging behind on samples");
    }

    for (int i = 0; i < num_blip_samples; ++i) {
        buffer[i] = pulse_temp[i] + tnd_temp[i];
    }
}
