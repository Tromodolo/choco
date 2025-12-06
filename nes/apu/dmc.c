//
// Created by tromo on 10/24/25.
//

#include "dmc.h"

#include <stdlib.h>

#include "../nes.h"

void update_sample(struct DMC* dmc);

const uint16_t sample_timer_rates[] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106,  84,  72,  54
};

struct DMC* dmc_init() {
    struct DMC* dmc = malloc(sizeof(struct DMC));

    dmc->enabled = false;

    dmc->irq_enabled = false;
    dmc->irq_pending = false;
    dmc->sample_loop = false;

    dmc->dmc_dma_active = false;
    dmc->dmc_dma_timer = -1;

    dmc->output_level = 0;

    dmc->sample_address = 0;
    dmc->sample_length = 0;

    dmc->sample_address_current = 0;
    dmc->sample_length_current = 0;

    dmc->sample_current = 0;
    dmc->sample_bits_remaining = 0;

    dmc->sample_buffer = 0;
    dmc->sample_buffer_filled = false;

    dmc->should_delay_sample_load = false;
    dmc->sample_load_delay = 0;

    dmc->sample_empty = false;
    dmc->silenced = false;

    dmc->timer = 0;
    dmc->timer_reset = 0;

    dmc->pending_mute = false;

    return dmc;
}

void dmc_free(struct DMC* dmc) {
    free(dmc);
}

void dmc_set_enabled(struct DMC* dmc, bool enabled) {
    if (enabled) {
        if (dmc->sample_length_current == 0) {
            dmc->sample_length_current = dmc->sample_length;
        }

        // Cpu write has delayed loading compared to an automatic reload
        if (!dmc->sample_buffer_filled) {
            dmc->should_delay_sample_load = true;
        }
    } else {
        dmc->pending_mute = true;
    }

    dmc->irq_pending = false;
}

void dmc_write_flags_and_rate(struct DMC* dmc, uint8_t val) {
    dmc->irq_enabled = (val & 0b10000000) >> 7;
    if (!dmc->irq_enabled)
        dmc->irq_pending = false;

    dmc->sample_loop = (val & 0b01000000) >> 6;
    dmc->timer_reset = sample_timer_rates[val & 0b1111];
}

void dmc_write_direct_load(struct DMC* dmc, uint8_t val) {
    dmc->output_level = val & 0b01111111;
}

void dmc_write_sample_address(struct DMC* dmc, uint8_t val) {
    dmc->sample_address = 0xC000 + val * 64;
    dmc->sample_address_current = dmc->sample_address;
}

void dmc_write_sample_length(struct DMC* dmc, uint8_t val) {
    dmc->sample_length = val * 16 + 1;
    dmc->sample_length_current = dmc->sample_length;
}

void dmc_fetch_new_sample(struct Nes* nes, struct DMC* dmc) {
    if (dmc->sample_length_current > 0) {
        dmc->sample_buffer = nes_read_char(nes, dmc->sample_address_current);
        dmc->sample_buffer_filled = true;
        dmc->sample_address_current++;

        // Handling wraparound
        if (dmc->sample_address_current == 0) {
            dmc->sample_address_current = 0x8000;
        }

        dmc->sample_length_current--;

        if (dmc->sample_length_current == 0) {
            if (dmc->sample_loop) {
                dmc->sample_address_current = dmc->sample_address;
                dmc->sample_length_current = dmc->sample_length;
            } else if (dmc->irq_enabled) {
                dmc->irq_pending = true;
            }
        }

        if (dmc->pending_mute) {
            dmc->sample_length = 0;
            dmc->pending_mute = false;
        }
    }
}

void dmc_step(struct DMC* dmc) {
    if (dmc->timer == 0) {
        dmc->timer = dmc->timer_reset;
        update_sample(dmc);
    } else {
        dmc->timer--;
    }

    if (!dmc->sample_buffer_filled && dmc->sample_length_current > 0 && dmc->sample_load_delay == 0 && dmc->dmc_dma_timer == -1) {
        if (!dmc->should_delay_sample_load) {
            dmc->dmc_dma_active = true;
            dmc->dmc_dma_timer = 3;
        } else {
            if (dmc->timer % 2 == 0) {
                dmc->sample_load_delay = 3;
            } else {
                dmc->sample_load_delay = 2;
            }
        }
    }

    if (dmc->sample_load_delay != 0) {
        dmc->sample_load_delay--;
    }
}

void update_sample(struct DMC* dmc) {
    if (!dmc->silenced) {
        if (dmc->sample_current & 1) {
            if (dmc->output_level < 126) {
                dmc->output_level += 2;
            }
        } else {
            if (dmc->output_level > 1) {
                dmc->output_level -= 2;
            }
        }
    }

    dmc->sample_current >>= 1;

    if (dmc->sample_bits_remaining == 0) {
        dmc->sample_bits_remaining = 7;

        if (dmc->sample_buffer_filled) {
            dmc->silenced = false;
            dmc->sample_current = dmc->sample_buffer;
            dmc->sample_buffer_filled = false;
        } else {
            dmc->silenced = true;
        }
    } else {
        dmc->sample_bits_remaining--;
    }
}

short dmc_get_sample(const struct DMC* dmc) {
    return dmc->output_level;
}
