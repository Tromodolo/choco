#ifndef NES_H
#define NES_H
#include <raylib.h>
#include <stdint.h>

struct Nes {
    struct Cartridge* cartridge;
    struct CPU* cpu;
    struct PPU* ppu;
};

struct Nes* nes_init(const char* file_path);
struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size);
void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame);
void nes_free(struct Nes* nes);

uint8_t nes_read_char(const struct Nes* nes, uint16_t addr);
void nes_write_char(const struct Nes* nes, uint16_t addr, uint8_t val);

uint16_t nes_read_short(const struct Nes* nes, uint16_t addr);
void nes_write_short(const struct Nes* nes, uint16_t addr, uint16_t val);

bool nes_is_nmi(const struct Nes* nes);

#endif //NES_H
