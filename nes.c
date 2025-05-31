#include <stdlib.h>

#include "nes.h"
#include "cartridge.h"
#include "cpu.h"

struct Nes* nes_init(const char* file_path) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_file(file_path);
    nes->cpu =  nes_cpu_init(nes);

    return nes;
}

struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_buffer(buffer, size);
    nes->cpu =  nes_cpu_init(nes);

    return nes;
}

inline void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame){
    for (;;)
        nes_cpu_tick(nes);
}

void nes_free(struct Nes* nes) {
    nes_cartridge_free(nes->cartridge);
    nes_cpu_free(nes->cpu);
    free(nes);
}

uint8_t* nes_get_addr_ptr(const struct Nes* nes, const uint16_t addr) {
    return nes_cartridge_get_addr_ptr(nes->cartridge, addr);
}

inline uint8_t nes_read_char(const struct Nes* nes, const uint16_t addr) {
    return nes_cartridge_read_char(nes->cartridge, addr);
}
inline void nes_write_char(const struct Nes* nes, const uint16_t addr, const uint8_t val) {
    return nes_cartridge_write_char(nes->cartridge, addr, val);
}

inline uint16_t nes_read_short(const struct Nes* nes, uint16_t addr) {
    uint8_t lo = nes_cartridge_read_char(nes->cartridge, addr);
    uint8_t hi = nes_cartridge_read_char(nes->cartridge, ++addr);
    return hi << 8 | lo;
}
inline void nes_write_short(const struct Nes* nes, uint16_t addr, const uint16_t val) {
    uint8_t lo = val & 0xFF;
    uint8_t hi = val >> 8;
    nes_cartridge_write_char(nes->cartridge, addr, lo);
    nes_cartridge_write_char(nes->cartridge, ++addr, hi);
}