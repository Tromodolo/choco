#include <stdlib.h>
#include <assert.h>

#include "ppu.h"

#include <stdio.h>

#include "cartridge.h"
#include "palette.h"

constexpr int OAM_SIZE = 0xFF;
constexpr int PALETTE_SIZE = 0x20;
constexpr int VRAM_SIZE = 0x800;

constexpr int TILE_START_ADDR = 0x2000;
constexpr int TILE_ATTR_START_ADDR = 0x23c0;

constexpr int MAX_SPRITE_COUNT = 8;

// Inline functions declaration
uint16_t get_increment(const struct PPU* ppu);
uint16_t get_background_pattern_address(const struct PPU* ppu);
uint16_t get_sprite_pattern_address(const struct PPU* ppu);
uint8_t get_sprite_size(const struct PPU* ppu);

void update_shifters(struct PPU* ppu);

void load_bg_shifters(struct PPU* ppu);
void load_bg_patterns(const struct Nes* nes, struct PPU* ppu);

void evaluate_sprites_on_line(struct PPU* ppu);
void load_sprite_patterns(const struct PPU* ppu);

void render_current_dot(struct Nes* nes, struct PPU* ppu, Color* frame_buffer);

void increment_scroll_x(struct PPU* ppu);
void increment_scroll_y(struct PPU* ppu);
void reset_x_addr(struct PPU* ppu);
void reset_y_addr(struct PPU* ppu);

struct PPU* ppu_init(const struct Nes* nes) {
    struct PPU* ppu = malloc(sizeof(struct PPU));

    ppu->chr_rom = nes->cartridge->chr_rom;
    ppu->oam = calloc(OAM_SIZE, sizeof(uint8_t));
    ppu->palette = calloc(PALETTE_SIZE, sizeof(uint8_t));
    ppu->vram = calloc(VRAM_SIZE, sizeof(uint8_t));

    ppu->oam_address = 0;
    ppu->read_buffer = 0;
    ppu->is_in_nmi = 0;

    ppu->dots_drawn = 20; // console starts with a RESET-interrupt which takes 7 cycles, so 7 * 3
    ppu->current_scanline = 0;
    ppu->total_cycles = 0;
    ppu->is_even_frame = false;
    ppu->total_frame_cycles = 0;

    ppu->loopy_temp.value = 0;
    ppu->loopy_value.value = 0;
    ppu->scroll_fine_x = 0;
    ppu->scroll_write_latch = false;

    ppu->evaluated_sprites = malloc(sizeof(struct SpriteEntry) * MAX_SPRITE_COUNT);
    ppu->sprite_count = 0;
    ppu->sprite_zero_possible = false;
    ppu->sprite_zero_rendered = false;

    ppu->sprite_shifter_pattern_lo = calloc(MAX_SPRITE_COUNT, sizeof(uint8_t));
    ppu->sprite_shifter_pattern_hi = calloc(MAX_SPRITE_COUNT, sizeof(uint8_t));

    ppu->bg_next_tile_id = 0;
    ppu->bg_next_tile_attribute = 0;
    ppu->bg_next_tile_lsb = 0;
    ppu->bg_next_tile_msb = 0;

    ppu->bg_shifter_pattern_lo = 0;
    ppu->bg_shifter_pattern_hi = 0;
    ppu->bg_shifter_attribute_lo = 0;
    ppu->bg_shifter_attribute_hi = 0;

    ppu->control_register.value = 0;
    ppu->mask_register.value = 0;
    ppu->status_register.value = 0;
    ppu->status_register.vblank = 1;

    return ppu;
}

// Horizontal:
//   [ A1 ] [ a2 ]
//   [ B1 ] [ b2 ]

// Vertical:
//   [ A1 ] [ B1 ]
//   [ a2 ] [ b2 ]
uint16_t mirror_vram_addr(const struct Nes* nes, const uint16_t addr) {
    // Mirrors values like 0x3000-0x3eff down to 0x2000-0x2eff
    const int mirroredAddr = addr & 0x2FFF;
    // Get absolute value within vram
    const uint16_t vector = mirroredAddr - 0x2000;
    const enum Mirroring current_mirroring = nes->cartridge->mirroring;

    switch (current_mirroring) {
        case Mirroring_Vertical:
            if (vector <= 0x03FF)
                return vector & 0x03FF;

            if (vector <= 0x07FF)
                return (vector & 0x03FF) + 0x400;

            if (vector <= 0x0BFF)
                return vector & 0x03FF;

            if (vector <= 0x0FFF)
                return (vector & 0x03FF) + 0x400;
            break;
        case Mirroring_Horizontal:
            if (vector <= 0x03FF)
                return vector & 0x03FF;

            if (vector <= 0x07FF)
                return vector & 0x03FF;

            if (vector <= 0x0BFF)
                return (vector & 0x03FF) + 0x400;

            if (vector <= 0x0FFF)
                return (vector & 0x03FF) + 0x400;
            break;
        case Mirroring_FourScreen:
        default:
            break;
    }
    return vector;
}

uint8_t internal_read(const struct Nes* nes, struct PPU* ppu, const uint16_t addr) {
    if (addr <= 0x1FFF) {
        const uint8_t value = ppu->read_buffer;
        ppu->read_buffer = ppu->chr_rom[addr];
        return value;
    }
    if (addr <= 0x3EFF) {
        const uint8_t value = ppu->read_buffer;
        ppu->read_buffer = ppu->vram[mirror_vram_addr(nes, addr)];
        return value;
    }
    if (addr <= 0X3FFF) {
        // 0x3F00-0x3F1F contain palette data
        // 0x3F20-0x3FFF is mirrored data
        const uint8_t mirrored_addr = addr & 0x1F;
        return ppu->palette[mirrored_addr];
    }

    assert(false);
}

void internal_write(const struct Nes* nes, const struct PPU* ppu, const uint16_t addr, const uint8_t val) {
    if (addr <= 0x1FFF) {
        ppu->chr_rom[addr] = val;
        return;
    }
    if (addr <= 0x3EFF) {
        ppu->vram[mirror_vram_addr(nes, addr)] = val;
        return;
    }
    if (addr <= 0X3FFF) {
        // 0x3F00-0x3F1F contain palette data
        // 0x3F20-0x3FFF is mirrored data
        const uint8_t mirrored_addr = addr & 0x1F;
        ppu->palette[mirrored_addr] = val;
        return;
    }

    assert(false);
}

inline void update_shifters(struct PPU* ppu) {
    if (ppu->mask_register.background_render_enable) {
        ppu->bg_shifter_attribute_lo <<= 1;
        ppu->bg_shifter_attribute_hi <<= 1;
        ppu->bg_shifter_pattern_lo <<= 1;
        ppu->bg_shifter_pattern_hi <<= 1;
    }

    if (ppu->mask_register.sprite_render_enable &&
        ppu->dots_drawn >= 1 && ppu->dots_drawn < 258) {
        for (int i = 0; i < ppu->sprite_count; i++) {
            struct SpriteEntry* sprite = &ppu->evaluated_sprites[i];

            if (sprite->x_position > 0) {
                sprite->x_position--;
            } else {
                ppu->sprite_shifter_pattern_lo[i] <<= 1;
                ppu->sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}

bool ppu_tick(struct Nes* nes, struct PPU* ppu, Color* frame_buffer, bool* is_new_frame) {
    if (ppu->current_scanline >= -1 && ppu->current_scanline < 240) {
        if (ppu->is_even_frame &&
            (ppu->mask_register.background_render_enable || ppu->mask_register.sprite_render_enable) &&
            ppu->current_scanline == 0 && ppu->dots_drawn == 0) {
            ppu->dots_drawn = 1;
        }

        if (ppu->current_scanline == -1 && ppu->dots_drawn == 1) {
            ppu->status_register.vblank = 0;
            ppu->status_register.sprite_zero_hit = 0;
            ppu->status_register.sprite_overflow = 0;

            for (int j = 0; j < MAX_SPRITE_COUNT; j++) {
                ppu->sprite_shifter_pattern_lo[j] = 0;
                ppu->sprite_shifter_pattern_hi[j] = 0;
            }
        }

        if ((ppu->dots_drawn >= 2 && ppu->dots_drawn < 258) ||
            (ppu->dots_drawn >= 321 && ppu->dots_drawn < 338)) {
            update_shifters(ppu);
            load_bg_patterns(nes, ppu);
        }

        if (ppu->dots_drawn == 256) {
            // End of line, go to next line
            increment_scroll_y(ppu);
        }

        if (ppu->dots_drawn == 257) {
            // reset back to start of line
            load_bg_shifters(ppu);
            reset_x_addr(ppu);
        }

        if (ppu->dots_drawn == 338 || ppu->dots_drawn == 340) {
            // preload next tile
            const int tile_addr = mirror_vram_addr(nes, TILE_START_ADDR | ppu->loopy_value.value & 0xFFF);
            ppu->bg_next_tile_id = ppu->vram[tile_addr];
        }

        if (ppu->current_scanline == -1 && ppu->dots_drawn >= 280 && ppu->dots_drawn < 305) {
            reset_y_addr(ppu);
        }

        // Evaluating sprites on the line
        if (ppu->dots_drawn == 257 && ppu->current_scanline >= 0) {
            evaluate_sprites_on_line(ppu);
        }

        // Loading sprite pattern depending on sprites evaluated
        if (ppu->dots_drawn == 340) {
            load_sprite_patterns(ppu);
        }
    }

    if (ppu->current_scanline == 241 && ppu->dots_drawn == 1) {
        *is_new_frame = true;
        ppu->status_register.vblank = 1;

        if (ppu->control_register.nmi_enable) {
            ppu->is_in_nmi = true;
        }
    }

    render_current_dot(nes, ppu, frame_buffer);

    ppu->dots_drawn++;
    ppu->total_cycles++;
    ppu->total_frame_cycles++;

    if (ppu->current_scanline < 240 && ppu->dots_drawn == 260) {
        if (ppu->mask_register.background_render_enable || ppu->mask_register.sprite_render_enable) {
            // Decrement scanline for mapper
        }
    }

    if (ppu->dots_drawn >= 341) {
        ppu->dots_drawn = 0;
        ppu->current_scanline++;

        if (ppu->current_scanline >= 261) {
            //printf("frame had %lu ticks\n", ppu->total_frame_cycles);

            ppu->is_even_frame = !ppu->is_even_frame;
            ppu->total_frame_cycles = 0;

            ppu->current_scanline = -1;
            ppu->status_register.vblank = 0;
            ppu->is_in_nmi = false;
        }
    }

    return is_new_frame;
}

bool ppu_get_nmi_interrupt(const struct Nes* nes, struct PPU* ppu) {
    const bool is_nmi = ppu->is_in_nmi;
    ppu->is_in_nmi = false;
    return is_nmi;
}
uint8_t ppu_get_status(const struct Nes* nes, struct PPU* ppu) {
    const uint8_t status = ppu->status_register.value;

    ppu->status_register.vblank = 0;
    ppu->scroll_write_latch = false;

    return status;
}
uint8_t ppu_get_oam_data(const struct Nes* nes, const struct PPU* ppu) {
    return ppu->oam[ppu->oam_address];
}
uint8_t ppu_get_data(const struct Nes* nes, struct PPU* ppu) {
    const uint16_t addr = ppu->loopy_value.value;
    ppu->loopy_value.value = ppu->loopy_value.value + get_increment(ppu);
    return internal_read(nes, ppu, addr);
}

void ppu_write_ctrl(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    ppu->loopy_temp.nametable_x = value & 0b01;
    ppu->loopy_temp.nametable_y = (value & 0b10) >> 1;

    const bool before_nmi = ppu->control_register.nmi_enable;
    ppu->control_register.value = value;

    if (!before_nmi && ppu->control_register.nmi_enable && ppu->status_register.vblank)
        ppu->is_in_nmi = true;
}
void ppu_write_scroll(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    if (ppu->scroll_write_latch) { // write Y
        const int fine_y = value & 0b111;
        const int coarse_y = value >> 3;

        ppu->loopy_temp.fine_y = fine_y;
        ppu->loopy_temp.coarse_y = coarse_y;
    } else { // write X
        const int scroll_fine_x = value & 0b111;
        const int coarse_x = value >> 3;

        ppu->scroll_fine_x = scroll_fine_x;
        ppu->loopy_temp.coarse_x = coarse_x;
    }

    ppu->scroll_write_latch = !ppu->scroll_write_latch;
}
void ppu_write_addr(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    if (ppu->scroll_write_latch) {
        const uint16_t loopy_temp_hi = ppu->loopy_temp.value & 0xFF00;
        ppu->loopy_temp.value = loopy_temp_hi | value;
        ppu->loopy_value.value = ppu->loopy_temp.value;
    } else {
        const uint16_t loopy_temp_lo = ppu->loopy_temp.value & 0x00FF;
        const uint16_t hi = (value & 0x3F) << 8;
        ppu->loopy_temp.value = hi | loopy_temp_lo;
    }

    ppu->scroll_write_latch = !ppu->scroll_write_latch;
}
void ppu_write_data(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    const uint16_t addr = ppu->loopy_value.value;
    ppu->loopy_value.value += get_increment(ppu);
    internal_write(nes, ppu, addr, value);
}
void ppu_write_oam_data(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    ppu->oam[ppu->oam_address] = value;
    ppu->oam_address++;
}
void ppu_write_mask(const struct Nes* nes, struct PPU* ppu, const uint8_t value) {
    ppu->mask_register.value = value;
}

inline uint16_t get_increment(const struct PPU* ppu) {
    return ppu->control_register.vram_address_increment
        ? 0x20
        : 1;
}
inline uint16_t get_background_pattern_address(const struct PPU* ppu) {
    return ppu->control_register.background_pattern_address
       ? 0x1000
       : 0;
}

inline uint16_t get_sprite_pattern_address(const struct PPU* ppu) {
    return ppu->control_register.sprite_pattern_address
       ? 0x1000
       : 0;
}

inline uint8_t get_sprite_size(const struct PPU* ppu) {
    return ppu->control_register.sprite_size
       ? 16
       : 8;
}

inline void load_bg_shifters(struct PPU* ppu) {
    ppu->bg_shifter_pattern_lo = ppu->bg_shifter_pattern_lo & 0xFF00 | ppu->bg_next_tile_lsb;
    ppu->bg_shifter_pattern_hi = ppu->bg_shifter_pattern_hi & 0xFF00 | ppu->bg_next_tile_msb;

    const int attributeLo = ppu->bg_next_tile_attribute & 0b01;
    const int attributeHi = ppu->bg_next_tile_attribute & 0b10;

    if (attributeLo) {
        ppu->bg_shifter_attribute_lo = ppu->bg_shifter_attribute_lo & 0xFF00 | 0xFF;
    } else {
        ppu->bg_shifter_attribute_lo = ppu->bg_shifter_attribute_lo & 0xFF00;
    }

    if (attributeHi) {
        ppu->bg_shifter_attribute_hi = ppu->bg_shifter_attribute_hi & 0xFF00 | 0xFF;
    } else {
        ppu->bg_shifter_attribute_hi = ppu->bg_shifter_attribute_hi & 0xFF00;
    }
}

inline void load_bg_patterns(const struct Nes* nes, struct PPU* ppu) {
    const int current_sub_tile_pixel = (ppu->dots_drawn - 1) % 8;
    // Update state depending on where we are in the tile rendered
    switch (current_sub_tile_pixel) {
        case 0:
            load_bg_shifters(ppu);
            const int tileAddr = mirror_vram_addr(nes, TILE_START_ADDR | ppu->loopy_value.value & 0xFFF);
            ppu->bg_next_tile_id = ppu->vram[tileAddr];
            break;
        case 2:
            const int attributeAddr = mirror_vram_addr(nes,
                TILE_ATTR_START_ADDR |
                ppu->loopy_value.nametable_y << 11 |
                ppu->loopy_value.nametable_x << 10 |
                (ppu->loopy_value.coarse_y >> 2) << 3 |
                ppu->loopy_value.coarse_x);

            ppu->bg_next_tile_attribute = ppu->vram[attributeAddr];

            if (ppu->loopy_value.coarse_y & 0x02) {
                ppu->bg_next_tile_attribute >>= 4;
            }

            if (ppu->loopy_value.coarse_x & 0x02) {
                ppu->bg_next_tile_attribute >>= 2;
            }

            ppu->bg_next_tile_attribute &= 0x03;
            break;
        case 4:
            const int lsb_addr = get_background_pattern_address(ppu) + (ppu->bg_next_tile_id * 16) + ppu->loopy_value.fine_y;
            ppu->bg_next_tile_lsb = ppu->chr_rom[lsb_addr];
            break;
        case 6:
            const int msb_addr = get_background_pattern_address(ppu) + (ppu->bg_next_tile_id * 16) + ppu->loopy_value.fine_y + 8;
            ppu->bg_next_tile_msb = ppu->chr_rom[msb_addr];
            break;
        case 7:
            increment_scroll_x(ppu);
            break;
        default: break;
    }
}

inline void evaluate_sprites_on_line(struct PPU* ppu) {
    ppu->sprite_count = 0;

    for (int i = 0; i < MAX_SPRITE_COUNT; i++) {
        ppu->sprite_shifter_pattern_lo[i] = 0;
        ppu->sprite_shifter_pattern_hi[i] = 0;
    }

    ppu->sprite_zero_possible = false;


    // Go through all of OAM and figure out if there are any sprites on current line
    for (int sprite_index = 0; sprite_index < 64; sprite_index++) {
        if (ppu->sprite_count > 8) {
            ppu->status_register.sprite_overflow = 1;
            break;
        }

        const uint8_t y_pos = ppu->oam[sprite_index * 4];
        const uint8_t tile_index = ppu->oam[sprite_index * 4 + 1];
        const uint8_t attribute = ppu->oam[sprite_index * 4 + 2];
        const uint8_t x_pos = ppu->oam[sprite_index * 4 + 3];

        if (!y_pos && !tile_index && !attribute && !x_pos) {
            continue;
        }

        const int y_pos_diff = ppu->current_scanline - y_pos;
        const bool is_within_range = y_pos_diff >= 0 && y_pos_diff < get_sprite_size(ppu);
        if (is_within_range) {
            struct SpriteEntry* entry = &ppu->evaluated_sprites[ppu->sprite_count];
            entry->y_position = y_pos;
            entry->x_position = x_pos;
            entry->attribute = attribute;
            entry->tile_id = tile_index;

            if (tile_index) {
                entry->sprite_zero = false;
            } else {
                entry->sprite_zero = true;
                ppu->sprite_zero_possible = true;
            }
        }
    }
}

inline void load_sprite_patterns(const struct PPU* ppu) {
    int sprite_index = 0;
    for (int i = 0; i < MAX_SPRITE_COUNT; ++i) {
        const struct SpriteEntry* sprite = &ppu->evaluated_sprites[i];

        const bool flip_horizontal = (sprite->attribute & 0b1000000) >> 6;
        const bool flip_vertical = (sprite->attribute & 0b10000000) >> 7;

        uint16_t pattern_addr_lo;
        if (get_sprite_size(ppu) == 8) {
            // Handling depending on if its flipped or not
            const uint8_t sprite_y_offset = flip_vertical
                ? 7 - (ppu->current_scanline - sprite->y_position)
                : ppu->current_scanline - sprite->y_position;

            pattern_addr_lo =
                get_sprite_pattern_address(ppu) |
                sprite->tile_id * 16 |
                sprite_y_offset;
        } else {
            // Handling depending on WHICH part you're rendering,
            // since 16x8 sprites are technically 2 8x8 sprites
            uint16_t sprite_id_offset;

            if (flip_vertical) {
                sprite_id_offset = ppu->current_scanline - sprite->y_position < 8
                    ? ((sprite->tile_id & 0xFE) + 1) * 16
                    : (sprite->tile_id & 0xFE) * 16;
            } else {
                sprite_id_offset = ppu->current_scanline - sprite->y_position < 8
                    ? (sprite->tile_id & 0xFE) * 16
                    : ((sprite->tile_id & 0xFE) + 1) * 16;
            }

            const uint8_t sprite_y_offset = flip_vertical
                ? 7 - (ppu->current_scanline - sprite->y_position) & 0b111
                : ppu->current_scanline - sprite->y_position & 0b111;

            pattern_addr_lo =
                (sprite->tile_id & 1) << 12 |
                sprite_id_offset |
                sprite_y_offset;
        }

        const uint16_t pattern_addr_hi = pattern_addr_lo + 8;
        uint8_t pattern_bits_lo = ppu->chr_rom[pattern_addr_lo];
        uint8_t pattern_bits_hi = ppu->chr_rom[pattern_addr_hi];

        if (flip_horizontal) {
            // https://stackoverflow.com/a/2602885
            // What the fuck
            pattern_bits_lo = (pattern_bits_lo & 0xF0) >> 4 | (pattern_bits_lo & 0x0F) << 4;
            pattern_bits_lo = (pattern_bits_lo & 0xCC) >> 2 | (pattern_bits_lo & 0x33) << 2;
            pattern_bits_lo = (pattern_bits_lo & 0xAA) >> 1 | (pattern_bits_lo & 0x55) << 1;

            pattern_bits_hi = (pattern_bits_hi & 0xF0) >> 4 | (pattern_bits_hi & 0x0F) << 4;
            pattern_bits_hi = (pattern_bits_hi & 0xCC) >> 2 | (pattern_bits_hi & 0x33) << 2;
            pattern_bits_hi = (pattern_bits_hi & 0xAA) >> 1 | (pattern_bits_hi & 0x55) << 1;
        }

        ppu->sprite_shifter_pattern_lo[sprite_index] = pattern_bits_lo;
        ppu->sprite_shifter_pattern_hi[sprite_index] = pattern_bits_hi;

        sprite_index++;

        if (sprite_index >= ppu->sprite_count) {
            break;
        }
    }
}

inline void render_current_dot(struct Nes* nes, struct PPU* ppu, Color* frame_buffer) {
    uint8_t bg_pixel = 0;
    uint8_t bg_palette = 0;

    uint8_t fg_pixel = 0;
    uint8_t fg_palette = 0;

    bool spritePriority = false;
    if (ppu->mask_register.background_render_enable &&
        ppu->dots_drawn > 0 && ppu->dots_drawn <= 256 && ppu->current_scanline >= 0 && ppu->current_scanline < 240) {
        const uint16_t bit_mux = 0x8000 >> ppu->scroll_fine_x;

        const uint8_t pixel_lo = (ppu->bg_shifter_pattern_lo & bit_mux) > 0 ? 1 : 0;
        const uint8_t pixel_hi = (ppu->bg_shifter_pattern_hi & bit_mux) > 0 ? 1 : 0;
        bg_pixel = pixel_hi << 1 | pixel_lo;

        const uint8_t palette_lo = (ppu->bg_shifter_attribute_lo & bit_mux) > 0 ? 1 : 0;
        const uint8_t palette_hi = (ppu->bg_shifter_attribute_hi & bit_mux) > 0 ? 1 : 0;
        bg_palette = palette_hi << 1 | palette_lo;
    }

    if (ppu->mask_register.sprite_render_enable &&
        ppu->dots_drawn > 0 && ppu->dots_drawn <= 256 && ppu->current_scanline >= 0 && ppu->current_scanline < 240) {
        ppu->sprite_zero_rendered = false;

        for (int i = 0; i < ppu->sprite_count; ++i) {
            const struct SpriteEntry* sprite = &ppu->evaluated_sprites[i];

            if (sprite->x_position == 0) {
                const int sprite_pixel_lo = (ppu->sprite_shifter_pattern_lo[i] & 0x80) > 0 ? 1 : 0;
                const int sprite_pixel_hi = (ppu->sprite_shifter_pattern_hi[i] & 0x80) > 0 ? 1 : 0;

                fg_pixel = sprite_pixel_hi << 1 | sprite_pixel_lo;
                fg_palette = (sprite->attribute & 0b11) + 0b100;
                spritePriority = (sprite->attribute & 0x20) == 0;

                if (fg_pixel != 0) {
                    if (sprite->sprite_zero) {
                        ppu->sprite_zero_rendered = true;
                    }

                    // Since sprites are sorted in priority, if we actually find a sprite for the pixel, we can just skip the rest
                    break;
                }
            }
        }
    }

    bool is_bg = false;
    uint8_t render_pixel = 0;
    uint8_t render_palette = 0;
    if (bg_pixel == 0 && fg_pixel == 0) {
        // Just continue;
    } else if (bg_pixel == 0 && fg_pixel > 0) {
        is_bg = false;
        render_pixel = fg_pixel;
        render_palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        is_bg = true;
        render_pixel = bg_pixel;
        render_palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        if (spritePriority) {
            is_bg = true;
            render_pixel = fg_pixel;
            render_palette = fg_palette;
        } else {
            is_bg = false;
            render_pixel = bg_pixel;
            render_palette = bg_palette;
        }

        if (ppu->sprite_zero_possible && ppu->sprite_zero_rendered) {
            if (ppu->mask_register.background_render_enable && ppu->mask_register.sprite_render_enable) {
                const bool backgroundLeft = ppu->mask_register.background_left_column_enable;
                const bool spriteLeft = ppu->mask_register.sprites_left_column_enable;

                if (backgroundLeft && spriteLeft) {
                    if (ppu->dots_drawn >= 1 && ppu->dots_drawn <= 258) {
                        ppu->status_register.sprite_zero_hit = 1;
                    }
                } else {
                    if (ppu->dots_drawn >= 9 && ppu->dots_drawn <= 258) {
                        ppu->status_register.sprite_zero_hit = 1;
                    }
                }
            }
        }
    }

    Color color;
    if (is_bg) {
        if (!ppu->mask_register.background_left_column_enable && ppu->dots_drawn <= 8) {
            color = PALETTE[ppu->palette[0] & 0x3f];
        } else {
            color = PALETTE[ppu->palette[(render_palette << 2) + render_pixel] & 0x3f];
        }
    } else {
        if (!ppu->mask_register.sprites_left_column_enable && ppu->dots_drawn <= 8) {
            color = PALETTE[ppu->palette[0] & 0x3f];
        } else {
            color = PALETTE[ppu->palette[(render_palette << 2) + render_pixel] & 0x3f];
        }
    }

    const int pixel_x = ppu->dots_drawn - 1;
    int pixel_y = ppu->current_scanline;
    if (pixel_x >= 0 && pixel_x < SCREEN_WIDTH && pixel_y >= SCREEN_OFFSET_TOP && pixel_y < (SCREEN_HEIGHT - SCREEN_OFFSET_BOTTOM)) {
        pixel_y -= SCREEN_OFFSET_TOP;

        frame_buffer[
            pixel_x +
            pixel_y * SCREEN_WIDTH
        ] = color;
    }
}

inline void increment_scroll_x(struct PPU* ppu) {
    if (ppu->mask_register.sprite_render_enable || ppu->mask_register.background_render_enable) {
        if (ppu->loopy_value.coarse_x == 31) {
            ppu->loopy_value.coarse_x = 0;
            ppu->loopy_value.nametable_x = !ppu->loopy_value.nametable_x;
        } else {
            ppu->loopy_value.coarse_x++;
        }
    }
}
inline void increment_scroll_y(struct PPU* ppu) {
    if (ppu->mask_register.sprite_render_enable || ppu->mask_register.background_render_enable) {
        if (ppu->loopy_value.fine_y < 7) {
            ppu->loopy_value.fine_y++;
        } else {
            ppu->loopy_value.fine_y = 0;

            if (ppu->loopy_value.coarse_y == 29) {
                ppu->loopy_value.coarse_y = 0;
                ppu->loopy_value.nametable_y = !ppu->loopy_value.nametable_y;
            } else if (ppu->loopy_value.coarse_y == 31) {
                ppu->loopy_value.coarse_y = 0;
            } else {
                ppu->loopy_value.coarse_y++;
            }
        }
    }
}
inline void reset_x_addr(struct PPU* ppu) {
    if (ppu->mask_register.sprite_render_enable || ppu->mask_register.background_render_enable) {
        ppu->loopy_value.nametable_x = ppu->loopy_temp.nametable_x;
        ppu->loopy_value.coarse_x = ppu->loopy_temp.coarse_x;
    }
}
inline void reset_y_addr(struct PPU* ppu) {
    if (ppu->mask_register.sprite_render_enable || ppu->mask_register.background_render_enable) {
        ppu->loopy_value.nametable_y = ppu->loopy_temp.nametable_y;
        ppu->loopy_value.coarse_y = ppu->loopy_temp.coarse_y;
        ppu->loopy_value.fine_y = ppu->loopy_temp.fine_y;
    }
}