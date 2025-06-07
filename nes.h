#ifndef NES_H
#define NES_H
#include <raylib.h>
#include <stdint.h>

#include "main.h"

typedef union {
    struct {
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t select : 1;
        uint8_t start : 1;
        uint8_t up : 1;
        uint8_t down : 1;
        uint8_t left : 1;
        uint8_t right : 1;
    };
    uint8_t value;
} PlayerInput;

struct Nes {
    struct Cartridge* cartridge;
    struct CPU* cpu;
    struct PPU* ppu;
    struct APU* apu;

    uint64_t global_cycle_count;

    PlayerInput player_1_input;
    uint8_t current_reading_button_value;

    bool has_new_sample;
    uint16_t audio_sample_out;
    float clocks_since_last_sample;
};

constexpr float CPU_CLOCKS_PER_SECOND = 1789772.67f;
constexpr float CLOCKS_PER_SAMPLE = CPU_CLOCKS_PER_SECOND / AUDIO_SAMPLE_RATE;

struct Nes* nes_init(const char* file_path);
struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size);
bool nes_tick_until_sample(struct Nes* nes, Color* frame_buffer, bool* is_new_frame);
uint16_t nes_get_sample(struct Nes* nes);
void nes_free(struct Nes* nes);

void nes_read_inputs(struct Nes* nes);

uint8_t nes_read_char(struct Nes* nes, uint16_t addr);
void nes_write_char(struct Nes* nes, uint16_t addr, uint8_t val);

uint16_t nes_read_short(struct Nes* nes, uint16_t addr);
void nes_write_short(struct Nes* nes, uint16_t addr, uint16_t val);

bool nes_is_nmi(const struct Nes* nes);

#endif //NES_H
