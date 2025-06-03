#ifndef PPU_H
#define PPU_H
#include "nes.h"

constexpr int SCREEN_OFFSET_TOP = 8;
constexpr int SCREEN_OFFSET_BOTTOM = 8;

constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int EFFECTIVE_SCREEN_HEIGHT = SCREEN_HEIGHT - SCREEN_OFFSET_TOP - SCREEN_OFFSET_BOTTOM;

struct SpriteEntry {
    uint8_t y_position;
    uint8_t x_position;
    uint8_t tile_id;
    uint8_t attribute;
    bool sprite_zero;
};

typedef union
{
    struct {
        uint16_t coarse_x : 5;
        uint16_t coarse_y : 5;
        uint16_t nametable_x : 1;
        uint16_t nametable_y : 1;
        uint16_t fine_y : 3;
        uint16_t _unused : 1;
    };
    uint16_t value;
} Loopy;

typedef union {
    struct {
        uint8_t base_nametable_address : 2;
        uint8_t vram_address_increment : 1;
        uint8_t sprite_pattern_address : 1;
        uint8_t background_pattern_address : 1;
        uint8_t sprite_size : 1;
        uint8_t master_slave_select : 1;
        uint8_t nmi_enable : 1;
    };
    uint8_t value;
} ControlRegister;

typedef union {
    struct {
        uint8_t greyscale : 1;
        uint8_t background_left_column_enable : 1;
        uint8_t sprites_left_column_enable : 1;
        uint8_t background_render_enable : 1;
        uint8_t sprite_render_enable : 1;
        uint8_t emphasis_red : 1;
        uint8_t emphasis_green : 1;
        uint8_t emphasis_blue : 1;
    };
    uint8_t value;
} MaskRegister;

typedef union {
    struct {
        uint8_t _unused : 5;
        uint8_t sprite_overflow : 1;
        uint8_t sprite_zero_hit : 1;
        uint8_t vblank : 1;
    };
    uint8_t value;
} StatusRegister;

struct PPU {
    uint8_t* chr_rom;
    uint8_t* palette;
    uint8_t* vram;
    uint8_t* oam;

    uint8_t oam_address;
    uint8_t read_buffer;
    bool is_in_nmi;

    int dots_drawn;
    int current_scanline;
    uint64_t total_cycles;

    Loopy loopy_temp;
    Loopy loopy_value;
    uint8_t scroll_fine_x;
    bool scroll_write_latch;

    struct SpriteEntry* evaluated_sprites;
    uint8_t sprite_count;
    bool sprite_zero_possible;
    bool sprite_zero_rendered;

    uint8_t* sprite_shifter_pattern_lo;
    uint8_t* sprite_shifter_pattern_hi;

    uint8_t bg_next_tile_id;
    uint8_t bg_next_tile_attribute;
    uint8_t bg_next_tile_lsb;
    uint8_t bg_next_tile_msb;

    uint16_t bg_shifter_pattern_lo;
    uint16_t bg_shifter_pattern_hi;
    uint16_t bg_shifter_attribute_lo;
    uint16_t bg_shifter_attribute_hi;

    ControlRegister control_register;
    MaskRegister mask_register;
    StatusRegister status_register;
};

struct PPU* ppu_init(const struct Nes* nes);

bool ppu_tick(struct Nes* nes, struct PPU* ppu, Color* frame_buffer, bool* is_new_frame);

bool ppu_get_nmi_interrupt(const struct Nes* nes, struct PPU* ppu);
uint8_t ppu_get_status(const struct Nes* nes, struct PPU* ppu);
uint8_t ppu_get_oam_data(const struct Nes* nes, const struct PPU* ppu);
uint8_t ppu_get_data(const struct Nes* nes, struct PPU* ppu);

void ppu_write_ctrl(const struct Nes* nes, struct PPU* ppu, uint8_t value);
void ppu_write_scroll(const struct Nes* nes, struct PPU* ppu, uint8_t value);
void ppu_write_addr(const struct Nes* nes, struct PPU* ppu, uint8_t value);
void ppu_write_oam_data(const struct Nes* nes, struct PPU* ppu, uint8_t value);
void ppu_write_mask(const struct Nes* nes, struct PPU* ppu, uint8_t value);
void ppu_write_data(const struct Nes* nes, struct PPU* ppu, uint8_t value);

#endif //PPU_H
