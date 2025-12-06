#ifndef DMC_Hr
#define DMC_H
#include <stdint.h>
#include "../nes.h"

struct DMC {
    bool enabled;

    bool irq_enabled;
    bool irq_pending;
    bool sample_loop;

    bool dmc_dma_active;
    short dmc_dma_timer;

    uint8_t output_level;

    uint16_t sample_address;
    uint16_t sample_length;

    uint16_t sample_address_current;
    uint16_t sample_length_current;

    uint8_t sample_buffer;
    bool sample_buffer_filled;

    bool should_delay_sample_load;
    uint8_t sample_load_delay;

    uint8_t sample_current;
    uint8_t sample_bits_remaining;

    bool sample_empty;
    bool silenced;

    uint16_t timer;
    uint16_t timer_reset;

    bool pending_mute;

};

struct DMC* dmc_init();
void dmc_free(struct DMC* dmc);

void dmc_set_enabled(struct DMC* dmc, bool enabled);

void dmc_write_flags_and_rate(struct DMC* dmc, uint8_t val);
void dmc_write_direct_load(struct DMC* dmc, uint8_t val);
void dmc_write_sample_address(struct DMC* dmc, uint8_t val);
void dmc_write_sample_length(struct DMC* dmc, uint8_t val);

void dmc_fetch_new_sample(struct Nes* nes, struct DMC* dmc);

void dmc_step(struct DMC* dmc);
short dmc_get_sample(const struct DMC* dmc);

#endif //DMC_H
