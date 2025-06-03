#ifndef CORE_H
#define CORE_H
#include <raylib.h>

struct Core {
    int buffer_width;
    int buffer_height;
    Color* frame_buffer;
    bool frame_buffer_changed;
    void* emu;
    AudioCallback audio_callback;
};

struct Core* get_core_for_file(const char* file_path);
void core_audio_callback(struct Core* core, void *buffer_data, unsigned int frames);
void core_clear_frame_buffer_changed(struct Core* core);
void core_free(struct Core* core);
void core_read_inputs(const struct Core* core);

#endif //CORE_H
