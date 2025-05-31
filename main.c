#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>

#include "core.h"
#include "tests/tests.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

struct Core* core;

void get_samples(void *bufferData, const unsigned int frames){
    short* samples = bufferData;
    for(int i = 0; i < frames; i++) {
        samples[i] = 0;
    }

    core_audio_callback(core, bufferData, frames);
}

int main(void) {
    if(getenv("TESTS")) {
        run_tests();
        return 0;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Hallo");
    SetTargetFPS(144);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(1024);
    const AudioStream stream = LoadAudioStream(41000, 16, 1);

    core = get_core_for_file("/media/games/nestest.nes");

    SetAudioStreamCallback(stream, get_samples);
    PlayAudioStream(stream);

    const Image blank = GenImageColor(core->buffer_width, core->buffer_height, BLACK);
    const Texture2D texture = LoadTextureFromImage(blank);
    UnloadImage(blank);

    while (!WindowShouldClose()) {
        if (core->frame_buffer_changed) {
            UpdateTexture(texture, core->frame_buffer);
            core->frame_buffer_changed = false;
        }

        BeginDrawing();
        {
            ClearBackground(BLACK);
            DrawTexture(texture, (SCREEN_WIDTH - texture.width) / 2, (SCREEN_HEIGHT - texture.height) / 2, WHITE);
        }
        EndDrawing();
    }

    return 0;
}