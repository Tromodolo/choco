#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>

#include "core.h"
#include "tests/tests.h"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

struct Core* core;
int framecount = 0;

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

    char* title = malloc(100);

    while (!WindowShouldClose()) {
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