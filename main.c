#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>

#include "main.h"
#include "nes/core.h"
#include "tests/tests.h"

#define RAYGUI_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "raylib/gui_window_file_dialog.h"

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 768;

struct Core* core = nullptr;
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
    SetTargetFPS(60);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(1024);
    const AudioStream stream = LoadAudioStream(AUDIO_SAMPLE_RATE, 16, 1);

    SetAudioStreamCallback(stream, get_samples);

    GuiWindowFileDialogState file_dialog_state = InitGuiWindowFileDialog(GetWorkingDirectory());

    char file_path[512] = { 0 };
    char title[100] = { 0 };
    enum CoreType type = Core_Undefined;
    Texture2D texture = { 0 };

    while (!WindowShouldClose()) {

        if (file_dialog_state.SelectFilePressed)
        {
            if (core != nullptr) {
                StopAudioStream(stream);
                core_free(core);
                core = nullptr;
            }

            if (IsFileExtension(file_dialog_state.fileNameText, ".nes"))
            {
                strcpy(file_path, TextFormat("%s" PATH_SEPERATOR "%s", file_dialog_state.dirPathText, file_dialog_state.fileNameText));
                type = Core_Nes;
            }

            core = get_core_for_file(file_path, type);

            const Image blank = GenImageColor(core->buffer_width, core->buffer_height, BLACK);
            texture = LoadTextureFromImage(blank);
            UnloadImage(blank);

            PlayAudioStream(stream);

            file_dialog_state.SelectFilePressed = false;
        }

        if (core != nullptr) {
            if (core->frame_buffer_changed) {
                UpdateTexture(texture, core->frame_buffers[!core->active_buffer]);
                core->frame_buffer_changed = false;
                framecount++;

                const auto frametime = GetFrameTime();
                SetWindowTitle(TextFormat("%d %f", framecount, frametime));
            }

            core_read_inputs(core);
        }

        BeginDrawing();
        {
            ClearBackground(BLACK);

            if (core != nullptr) {
                const Rectangle source = {0, 0, core->buffer_width, core->buffer_height};
                const Rectangle dest = {0, MENU_HEIGHT, core->buffer_width * 2, core->buffer_height * 2};
                const Vector2 origin = {0, 0};

                DrawTexturePro(
                    texture,
                    source,
                    dest,
                    origin,
                    0.0f,
                    WHITE
                );

                core_draw_debug_info(core);
            }

            if (file_dialog_state.windowActive) {
                GuiLock();
            }

            DrawRectangle(0, 0, SCREEN_WIDTH, MENU_HEIGHT, GRAY);
            if (GuiButton((Rectangle){ 0, 0, 140, MENU_HEIGHT }, GuiIconText(ICON_FILE_OPEN, "Open File"))) {
                file_dialog_state.windowActive = true;
            }

            GuiUnlock();
            GuiWindowFileDialog(&file_dialog_state);
        }
        EndDrawing();
    }

    free(title);
    StopAudioStream(stream);
    core_free(core);

    return 0;
}