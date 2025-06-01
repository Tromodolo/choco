#include <stdlib.h>

#include "nes.h"

#include <assert.h>

#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"

uint8_t read_hw_register(const struct Nes* nes, uint16_t addr, bool* is_hw_register);
void write_hw_register(const struct Nes* nes, uint16_t addr, const uint8_t val, bool* is_hw_register);

struct Nes* nes_init(const char* file_path) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_file(file_path);
    nes->cpu =  nes_cpu_init(nes);
    nes->ppu = ppu_init(nes);

    return nes;
}

struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_buffer(buffer, size);
    nes->cpu =  nes_cpu_init(nes);

    return nes;
}

inline void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame){
    for (;;) {
        ppu_tick(nes, nes->ppu, frame_buffer, is_new_frame);
        nes_cpu_tick(nes);
    }
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
    bool is_hardware_register = false;
    const uint8_t hw_value = read_hw_register(nes, addr, &is_hardware_register);
    if (is_hardware_register) {
        return hw_value;
    }

    return nes_cartridge_read_char(nes->cartridge, addr);
}
inline void nes_write_char(const struct Nes* nes, const uint16_t addr, const uint8_t val) {
    bool is_hardware_register = false;
    write_hw_register(nes, addr, val, &is_hardware_register);
    if (is_hardware_register) {
        return;
    }

    return nes_cartridge_write_char(nes->cartridge, addr, val);
}

inline uint16_t nes_read_short(const struct Nes* nes, uint16_t addr) {
    const uint8_t lo = nes_cartridge_read_char(nes->cartridge, addr);
    const uint8_t hi = nes_cartridge_read_char(nes->cartridge, ++addr);
    return hi << 8 | lo;
}
inline void nes_write_short(const struct Nes* nes, uint16_t addr, const uint16_t val) {
    const uint8_t lo = val & 0xFF;
    const uint8_t hi = val >> 8;
    nes_cartridge_write_char(nes->cartridge, addr, lo);
    nes_cartridge_write_char(nes->cartridge, ++addr, hi);
}

inline uint8_t read_hw_register(const struct Nes* nes, uint16_t addr, bool* is_hw_register){
    if (addr >= RAM_MIRRORS_END && addr <= PPU_MIRRORS_END) {
        addr &= 0x2007;
    }

    switch (addr) {
        case 0x2002: // STATUS
            *is_hw_register = true;
            return ppu_get_status(nes, nes->ppu);
        case 0x2003: // OAM ADDR
            *is_hw_register = true;
            return nes->ppu->oam_address;
        case 0x2004: // OAM DATA
            *is_hw_register = true;
            return ppu_get_oam_data(nes, nes->ppu);
        case 0x2007: // DATA
            *is_hw_register = true;
            return ppu_get_data(nes, nes->ppu);;
            break;
        case 0x4016: // GAMEPAD 1
            *is_hw_register = true;
            break;
        case 0x2000: // CTRL
        case 0x2001: // MASK
        case 0x2005: // SCROLL
        case 0x2006: // ADDR
        case 0x4017: // GAMEPAD 2
        case 0x4014: // DMA
        case 0x4000: // APU
        case 0x4001: // APU
        case 0x4002: // APU
        case 0x4003: // APU
        case 0x4004: // APU
        case 0x4005: // APU
        case 0x4006: // APU
        case 0x4007: // APU
        case 0x4008: // APU
        case 0x4010: // APU
        case 0x4011: // APU
        case 0x4012: // APU
        case 0x4013: // APU
        case 0x4015: // APU
            *is_hw_register = true;
            return 0;
        default:
            *is_hw_register = false;
            break;
    }

    return 0;
}

inline void write_hw_register(const struct Nes* nes, uint16_t addr, const uint8_t val, bool* is_hw_register){
    if (addr >= RAM_MIRRORS_END && addr <= PPU_MIRRORS_END) {
        addr &= 0x2007;
    }

    switch (addr) {
        case 0x2000: // CTRL
            *is_hw_register = true;
            ppu_write_ctrl(nes, nes->ppu, val);
            break;
        case 0x2001: // MASK
            *is_hw_register = true;
            ppu_write_mask(nes, nes->ppu, val);
            break;
        case 0x2002: // STATUS
            *is_hw_register = true;
            assert(false);
            break;
        case 0x2003: // OAM ADDR
            *is_hw_register = true;
            nes->ppu->oam_address = val;
            break;
        case 0x2004: // OAM DATA
            *is_hw_register = true;
            ppu_write_oam_data(nes, nes->ppu, val);
            break;
        case 0x2005: // SCROLL
            *is_hw_register = true;
            ppu_write_scroll(nes, nes->ppu, val);
            break;
        case 0x2006: // ADDR
            *is_hw_register = true;
            ppu_write_addr(nes, nes->ppu, val);
            break;
        case 0x2007: // DATA
            *is_hw_register = true;
            ppu_write_data(nes, nes->ppu, val);
            break;
        case 0x4016: // GAMEPAD 1
            *is_hw_register = true;
            break;
        case 0x4000: // APU
        case 0x4001: // APU
        case 0x4002: // APU
        case 0x4003: // APU
        case 0x4004: // APU
        case 0x4005: // APU
        case 0x4006: // APU
        case 0x4007: // APU
        case 0x4008: // APU
        case 0x4010: // APU
        case 0x4011: // APU
        case 0x4012: // APU
        case 0x4013: // APU
        case 0x4015: // APU
        case 0x4017: // GAMEPAD 2
        case 0x4014: // DMA
            *is_hw_register = true;
            return;
        default:
            *is_hw_register = false;
            break;
    }
}