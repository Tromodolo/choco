//
// Created by tromo on 5/19/25.
//

#include <stdlib.h>

#include "nes.h"
#include "cartridge.h"
#include "cpu.h"
#include "nes-internal.h"

struct Nes* nes_init(char* file_path) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_file(file_path);
    nes->cpu =  nes_cpu_init(nes);

    return nes;
}

inline void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame){
    nes_cpu_tick(nes);
}

void nes_free(struct Nes* nes) {
    nes_cartridge_free(nes->cartridge);
    nes_cpu_free(nes->cpu);
    free(nes);
}

unsigned char* nes_get_addr_ptr(struct Nes* nes, unsigned short addr) {
    return nes_cartridge_get_addr_ptr(nes->cartridge, addr);
}

inline unsigned char nes_read_char(struct Nes* nes, unsigned short addr) {
    return nes_cartridge_read_char(nes->cartridge, addr);
}
inline void nes_write_char(struct Nes* nes, unsigned short addr, unsigned char val) {
    return nes_cartridge_write_char(nes->cartridge, addr, val);
}

inline unsigned short nes_read_short(struct Nes* nes, unsigned short addr) {
    unsigned char lo = nes_cartridge_read_char(nes->cartridge, addr);
    unsigned char hi = nes_cartridge_read_char(nes->cartridge, ++addr);
    return hi << 8 | lo;
}
inline void nes_write_short(struct Nes* nes, unsigned short addr, unsigned short val) {
    unsigned char lo = val & 0xFF;
    unsigned char hi = val >> 8;
    nes_cartridge_write_char(nes->cartridge, addr, lo);
    nes_cartridge_write_char(nes->cartridge, ++addr, hi);
}