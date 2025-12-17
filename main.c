#include <stdio.h>
#include <raylib.h>
#include <stdlib.h>

#include "main.h"
#include "core.h"
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
    enum CoreType type = Core_Undefined;
    Texture2D texture = { 0 };
    bool should_hard_reset = false;

    while (!WindowShouldClose()) {

        if (file_dialog_state.SelectFilePressed || should_hard_reset)
        {
            if (core != nullptr) {
                StopAudioStream(stream);
                core_free(core);
                core = nullptr;
            }

            if (file_dialog_state.SelectFilePressed && IsFileExtension(file_dialog_state.fileNameText, ".nes"))
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
            should_hard_reset = false;
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

            if (GuiButton((Rectangle){ 140, 0, 140, MENU_HEIGHT }, GuiIconText(ICON_RESTART, "Hard Reset"))) {
                should_hard_reset = true;
            }

            GuiUnlock();
            GuiWindowFileDialog(&file_dialog_state);
        }
        EndDrawing();
    }

    StopAudioStream(stream);
    core_free(core);

    return 0;
}