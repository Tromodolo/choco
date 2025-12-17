#include <stdlib.h>

#include "core.h"
#include "nes/nes.h"
#include "nes/ppu/ppu.h"

inline struct Core* get_core_for_file(const char* file_path, enum CoreType type) {
    struct Core* core = malloc(sizeof(struct Core));

    core->type = type;
    switch (core->type) {
        case Core_Nes:
            core->emu = nes_init(file_path);
            break;
        default: break;
    }

    core->buffer_height = EFFECTIVE_SCREEN_HEIGHT;
    core->buffer_width = SCREEN_WIDTH;
    core->frame_buffers = malloc(2 * sizeof(Color*));
    core->frame_buffers[0] = malloc(core->buffer_width * core->buffer_height * sizeof(Color)),
    core->frame_buffers[1] = malloc(core->buffer_width * core->buffer_height * sizeof(Color)),
    core->frame_buffer_changed = true;
    core->active_buffer = 1;

    for (int i = 0; i < core->buffer_width * core->buffer_height; i++) {
        core->frame_buffers[0][i] = DARKBLUE;
        core->frame_buffers[1][i] = DARKBLUE;
    }

    return core;
}

void core_audio_callback(struct Core* core, short* samples, const unsigned int sample_count) {
    bool is_new_frame = false;

    uint16_t num_clocks_for_samples = 0;
    switch (core->type) {
        case Core_Nes:
            num_clocks_for_samples = nes_num_clocks_for_sample_count(core->emu, sample_count);
            break;
        default: break;
    }

    for (int i = 0; i < num_clocks_for_samples; ++i) {
        is_new_frame = false;

        switch (core->type) {
            case Core_Nes:
                nes_tick(core->emu, core->frame_buffers[core->active_buffer], &is_new_frame);
                break;
            default: break;
        }

        if (is_new_frame) {
            core->frame_buffer_changed = true;
            core->active_buffer = !core->active_buffer;
        }
    }

    switch (core->type) {
        case Core_Nes:
            nes_get_samples(core->emu, samples, sample_count);
            break;
        default: break;
    }
}

void core_read_inputs(const struct Core* core) {
    switch (core->type) {
        case Core_Nes:
            nes_read_inputs(core->emu);
            break;
        default: break;
    }
}

inline void core_clear_frame_buffer_changed(struct Core* core) {
    core->frame_buffer_changed = false;
}

void core_draw_debug_info(const struct Core* core) {
    switch (core->type) {
        case Core_Nes:
            nes_draw_debug_info(core->emu);
            break;
        default: break;
    }
}

void core_free(struct Core* core) {
    switch (core->type) {
        case Core_Nes:
            nes_free(core->emu);
            break;
        default: break;
    }

    free(core);
}