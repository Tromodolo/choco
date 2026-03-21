#ifndef CORE_H
#define CORE_H
#include <raylib.h>

enum CoreType {
    Core_Undefined,
    Core_Nes,
    Core_DMG
};

struct Core {
    int buffer_width;
    int buffer_height;
    Color** frame_buffers;
    int active_buffer;
    bool frame_buffer_changed;
    AudioCallback audio_callback;
    enum CoreType type;
    void* emu;
};

struct Core* get_core_for_file(const char* file_path, enum CoreType type);
void core_audio_callback(struct Core* core, short* samples, unsigned int sample_count);
void core_clear_frame_buffer_changed(struct Core* core);
void core_free(struct Core* core);
void core_read_inputs(const struct Core* core);
void core_draw_debug_info(const struct Core* core);

#endif //CORE_H
