#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>

#include "main.h"
#include "core.h"
#include "tests/tests.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

struct Core* core;
int framecount = 0;

void get_samples(void* buffer, const unsigned int sample_count){
    short* samples = buffer;
    for(int i = 0; i < sample_count; i++) {
        samples[i] = 0;
    }

    core_audio_callback(core, samples, sample_count);
}

int main(void) {
#ifdef TESTS
        run_tests();
        return 0;
#endif

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hallo");
    SetTargetFPS(144);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(1024);
    const AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 16, 1);

    core = get_core_for_file("/media/games/smb.nes");

    SetAudioStreamCallback(stream, get_samples);
    PlayAudioStream(stream);

    const Image blank = GenImageColor(core->buffer_width, core->buffer_height, BLACK);
    const Texture2D texture = LoadTextureFromImage(blank);
    UnloadImage(blank);

    char* title = malloc(100);
    while (!WindowShouldClose()) {
        core_read_inputs(core);

        if (core->frame_buffer_changed) {
            UpdateTexture(texture, core->frame_buffer);
            core->frame_buffer_changed = false;
            framecount++;

            sprintf(title, "%d", framecount);
            SetWindowTitle(title);
        }

        BeginDrawing();
        {
            ClearBackground(BLACK);

            const Rectangle source = {0, 0, core->buffer_width, core->buffer_height};
            const Rectangle dest = {0, 0, core->buffer_width * 2, core->buffer_height * 2};
            const Vector2 origin = {0, 0};

            DrawTexturePro(
                texture,
                source,
                dest,
                origin,
                0.0f,
                WHITE
            );
        }
        EndDrawing();
    }

    return 0;
}