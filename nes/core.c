#include <stdlib.h>

#include "core.h"
#include "nes.h"
#include "ppu/ppu.h"

inline struct Core* get_core_for_file(const char* file_path) {
    struct Core* core = malloc(sizeof(struct Core));

    core->emu = nes_init(file_path);
    core->buffer_height = EFFECTIVE_SCREEN_HEIGHT;
    core->buffer_width = SCREEN_WIDTH;
    core->frame_buffer = malloc(core->buffer_width * core->buffer_height * sizeof(Color));
    core->frame_buffer_changed = true;

    for (int i = 0; i < core->buffer_width * core->buffer_height; i++) {
        core->frame_buffer[i] = DARKBLUE;
    }

    return core;
}

void core_audio_callback(struct Core* core, short* samples, const unsigned int sample_count) {
    bool is_new_frame = false;

    const uint16_t num_clocks_for_samples = nes_num_clocks_for_sample_count(core->emu, sample_count);
    for (int i = 0; i < num_clocks_for_samples; ++i) {
        is_new_frame = false;

        nes_tick(core->emu, core->frame_buffer, &is_new_frame);

        if (is_new_frame) {
            core->frame_buffer_changed = true;
        }
    }
    nes_get_samples(core->emu, samples, sample_count);
}

void core_read_inputs(const struct Core* core) {
    nes_read_inputs(core->emu);
}

inline void core_clear_frame_buffer_changed(struct Core* core) {
    core->frame_buffer_changed = false;
}

void core_free(struct Core* core) {
    nes_free(core->emu);
    free(core);
}