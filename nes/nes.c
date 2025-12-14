#include <stdlib.h>

#include "nes.h"

#include <assert.h>
#include <stdio.h>

#include "cartridge.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"
#include "apu/apu.h"
#include "mappers/mapper.h"

uint8_t read_hw_register(struct Nes* nes, uint16_t addr, bool* is_hw_register);
void write_hw_register(struct Nes* nes, uint16_t addr, const uint8_t val, bool* is_hw_register);

struct Nes* nes_init(const char* file_path) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_file(file_path);
    nes->cpu =  nes_cpu_init(nes);
    nes->ppu = ppu_init(nes);
    nes->apu = apu_init(nes);

    nes->player_1_input.value = 0;

    nes->global_cycle_count = 0;
    nes->sample_cycle_count = 0;

    nes->has_new_sample = false;
    nes->audio_sample_out = 0;
    nes->clocks_since_last_sample = 0;

    nes->audio_sample_accumulator = 0;

    nes->realign_dma = false;

    return nes;
}

struct Nes* nes_init_from_buffer(const uint8_t* buffer) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_buffer(buffer, nullptr);
    nes->cpu =  nes_cpu_init(nes);
    nes->ppu = ppu_init(nes);

    return nes;
}

inline void nes_tick(struct Nes* nes, Color* frame_buffer, bool* is_new_frame){
    nes->global_cycle_count++;
    nes->sample_cycle_count++;

    ppu_tick(nes, nes->ppu, frame_buffer, is_new_frame);

    mapper_set_pc(nes->cartridge, nes->cpu->pc);

    nes->has_new_sample = false;
    if (nes->global_cycle_count % 3 != 0)
        return;

    nes_cpu_tick(nes);
    apu_tick(nes->apu, nes->sample_cycle_count);

    nes->cpu->dma_read_write_latch = !nes->cpu->dma_read_write_latch;
    if (nes->cpu->is_dma_active && nes->apu->dmc->dmc_dma_timer != 1 && !nes->realign_dma) {
        if (nes->cpu->dma_read_write_latch) { // Read
            nes->cpu->dma_value = nes_read_char(nes, nes->cpu->dma_page << 8 | nes->cpu->dma_addr);
        } else if (!nes->cpu->dma_just_started) { // Write
            nes->ppu->oam[nes->cpu->dma_addr] = nes->cpu->dma_value;
            nes->cpu->dma_addr++;

            // After wrapping around
            nes->cpu->is_dma_active = nes->cpu->dma_addr != 0;
        }

        nes->cpu->dma_just_started = false;
    }

    nes->realign_dma = false;
    if (nes->apu->dmc->dmc_dma_timer > 0) {
        nes->cpu->ready = false;

        // If dmc dma and normal dma happen on the same tick, the dmc dma wins out, so the normal dma has to be realigned
        if (nes->apu->dmc->dmc_dma_timer == 1) {
            nes->realign_dma = true;
        }

        // If the cpu is still doing stuff (multi-cycle-operation), then the dmc-dma cant start yet
        if (nes->cpu->waiting_cycles > 0) {
            nes->apu->dmc->dmc_dma_timer += 2;
        }

        nes->apu->dmc->dmc_dma_timer--;
        if (nes->apu->dmc->dmc_dma_timer == 0) {
            dmc_fetch_new_sample(nes, nes->apu->dmc);
            nes->apu->dmc->dmc_dma_active = false;
            nes->apu->dmc->dmc_dma_timer = -1;
        }
    }

    if (!nes->cpu->ready && !nes->apu->dmc->dmc_dma_active) {
        nes->cpu->ready = true;
    }
}

uint16_t nes_num_clocks_for_sample_count(struct Nes* nes, uint16_t sample_count) {
    return apu_num_clocks_for_sample_count(nes->apu, sample_count);
}

inline void nes_get_samples(struct Nes* nes, short* buffer, uint16_t sample_count) {
    apu_read_samples(nes->apu, buffer, sample_count, nes->sample_cycle_count);
    nes->sample_cycle_count = 0;
}

void nes_free(struct Nes* nes) {
    nes_cartridge_free(nes->cartridge);
    nes_cpu_free(nes->cpu);
    // TODO: add free for all subtypes like ppu and apu
    free(nes);
}

void nes_read_inputs(struct Nes* nes) {
    nes->player_1_input.up = IsKeyDown(KEY_W);
    nes->player_1_input.down = IsKeyDown(KEY_S);
    nes->player_1_input.left = IsKeyDown(KEY_A);
    nes->player_1_input.right = IsKeyDown(KEY_D);

    nes->player_1_input.a = IsKeyDown(KEY_J);
    nes->player_1_input.b = IsKeyDown(KEY_K);

    nes->player_1_input.start = IsKeyDown(KEY_ENTER);
    nes->player_1_input.select = IsKeyDown(KEY_RIGHT_SHIFT);
    //
    // if (nes->player_1_input.value > 0)
    //     printf("%d\n", nes->player_1_input.value);
}

inline uint8_t nes_read_char(struct Nes* nes, const uint16_t addr) {
#ifdef TESTS
    return nes->cartridge->prg_ram[addr];
#endif

    bool is_hardware_register = false;
    const uint8_t hw_value = read_hw_register(nes, addr, &is_hardware_register);
    if (is_hardware_register) {
        return hw_value;
    }

    return nes_cartridge_read_char(nes->cartridge, addr);
}
inline void nes_write_char(struct Nes* nes, const uint16_t addr, const uint8_t val) {
#ifdef TESTS
    nes->cartridge->prg_ram[addr] = val;
    return;
#endif

    bool is_hardware_register = false;
    write_hw_register(nes, addr, val, &is_hardware_register);
    if (is_hardware_register) {
        return;
    }

    return nes_cartridge_write_char(nes->cartridge, addr, val);
}

inline uint16_t nes_read_short(struct Nes* nes, uint16_t addr) {
    const uint8_t lo = nes_read_char(nes, addr);
    const uint8_t hi = nes_read_char(nes, ++addr);
    return hi << 8 | lo;
}
inline void nes_write_short(struct Nes* nes, uint16_t addr, const uint16_t val) {
    const uint8_t lo = val & 0xFF;
    const uint8_t hi = val >> 8;
    nes_write_char(nes, addr, lo);
    nes_write_char(nes, ++addr, hi);
}

inline uint8_t read_hw_register(struct Nes* nes, uint16_t addr, bool* is_hw_register){
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
            return ppu_get_data(nes, nes->ppu);
        case 0x4016: // GAMEPAD 1
            *is_hw_register = true;
            const uint8_t value = nes->current_reading_button_value & 1;
            nes->current_reading_button_value >>= 1;
            return value;
        case 0x4015: // APU Status
            *is_hw_register = true;
            return apu_read(nes->apu, addr);
        case 0x2000: // CTRL
        case 0x2001: // MASK
        case 0x2005: // SCROLL
        case 0x2006: // ADDR
        case 0x4017: // APU
        case 0x4014: // DMA, doesn't exist in read
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
        case 0x400A: // APU
        case 0x400B: // APU
        case 0x400C: // APU
        case 0x400E: // APU
        case 0x400F: // APU
            *is_hw_register = true;
            // Read-only
            return 0;
        default:
            *is_hw_register = false;
            break;
    }

    return 0;
}

inline void write_hw_register(struct Nes* nes, uint16_t addr, const uint8_t val, bool* is_hw_register){
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
            // assert(false);
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
        case 0x4014: // DMA
            *is_hw_register = true;
            nes->cpu->is_dma_active = true;
            nes->cpu->dma_just_started = true;
            nes->cpu->dma_page = val;
            nes->cpu->dma_addr = 0;
            return;
        case 0x4016: // GAMEPAD 1
            *is_hw_register = true;
            if ((val & 1) == 1) {
                nes->current_reading_button_value = nes->player_1_input.value;
            }
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
        case 0x400A: // APU
        case 0x400B: // APU
        case 0x400C: // APU
        case 0x400E: // APU
        case 0x400F: // APU
        case 0x4010: // APU
        case 0x4011: // APU
        case 0x4012: // APU
        case 0x4013: // APU
        case 0x4015: // APU
        case 0x4017: // APU
            apu_write(nes->apu, addr, val);
            *is_hw_register = true;
            break;
        default:
            *is_hw_register = false;
            break;
    }
}

bool nes_is_nmi(const struct Nes* nes) {
    return ppu_get_nmi_interrupt(nes, nes->ppu);
}

//
// Debug stuff
//
struct DebugImage {
    Texture2D texture;
    Color* buffer;
    bool isInit;
};

int draw_debug_value(const int x, const int y, const int line_height, const int font_size, const Color color, const char* label, const char* formatting, ...) {
    char buffer[64];

    va_list args;
    va_start(args, formatting);
    vsnprintf(buffer, sizeof(buffer), formatting, args);
    va_end(args);

    DrawText(label, x, y, font_size, color);
    DrawText(buffer, x + 120, y, font_size, color);

    return y + line_height;
}

void draw_debug_image(const int x, const int y, const int width, const int height, struct DebugImage* image) {
    if (!image->isInit) {
        const Image blank = GenImageColor(width, height, BLACK);
        const Texture2D texture = LoadTextureFromImage(blank);
        UnloadImage(blank);
        image->texture = texture;
        image->buffer = malloc(width * height * sizeof(Color));
        image->isInit = true;
    }

    UpdateTexture(image->texture, image->buffer);

    const Rectangle source = {0, 0, width, height};
    const Rectangle dest = {x, y, width, height};
    const Vector2 origin = {0, 0};

    DrawTexturePro(
        image->texture,
        source,
        dest,
        origin,
        0.0f,
        WHITE
    );
}

// local variables just for debug
struct DebugImage nametable = { 0 };
void nes_draw_debug_info(const struct Nes* nes) {
    constexpr int left_padding        = (SCREEN_WIDTH * 2) + 10;
    constexpr int top_padding         = 10;
    constexpr int header_x_pos        = left_padding + 15;
    constexpr int value_x_pos         = header_x_pos + 15;

    constexpr int header_font_size    = 22;
    constexpr int value_font_size     = 20;
    constexpr int line_height         = 22;

    int y = top_padding;

    DrawText("Registers", left_padding, y, 24, WHITE);
    y += 30;

    /* ================= CPU ================= */

    DrawText("CPU", header_x_pos, y, header_font_size, WHITE);
    y += 30;

    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "PC",  "%04X", nes->cpu->pc);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "A",   "%02X", nes->cpu->acc);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "X",   "%02X", nes->cpu->x);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Y",   "%02X", nes->cpu->y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "P",   "%02X", nes->cpu->p.value);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "SP",  "%02X", nes->cpu->sp);

    y += 15;

    /* ================= PPU ================= */

    DrawText("PPU", header_x_pos, y, header_font_size, WHITE);
    y += 30;

    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Dots",     "%03d", nes->ppu->dots_drawn);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Scanline", "%03d", nes->ppu->current_scanline);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "CTRL",     "%02X", nes->ppu->control_register.value);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "MASK",     "%02X", nes->ppu->mask_register.value);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "STATUS",   "%02X", nes->ppu->status_register.value);

    y += 15;

    /* ================= Loopy ================= */

    DrawText("Loopy", header_x_pos, y, header_font_size, WHITE);
    y += 30;

    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Coarse X", "%02d", nes->ppu->loopy_value.coarse_x);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Coarse Y", "%02d", nes->ppu->loopy_value.coarse_y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "N X",      "%d",  nes->ppu->loopy_value.nametable_x);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "NT Y",     "%d",  nes->ppu->loopy_value.nametable_y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Fine Y",   "%d",  nes->ppu->loopy_value.fine_y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Fine X",   "%d",  nes->ppu->scroll_fine_x);

    y += 15;

    /* ================= Loopy TMP ================= */

    DrawText("Loopy TMP", header_x_pos, y, header_font_size, WHITE);
    y += 30;

    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Coarse X", "%02d", nes->ppu->loopy_temp.coarse_x);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Coarse Y", "%02d", nes->ppu->loopy_temp.coarse_y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "NT X",     "%d",  nes->ppu->loopy_temp.nametable_x);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "NT Y",     "%d",  nes->ppu->loopy_temp.nametable_y);
    y = draw_debug_value(value_x_pos, y, line_height, value_font_size, WHITE, "Fine Y",   "%d",  nes->ppu->loopy_temp.fine_y);

    ppu_get_nametables(nes, nes->ppu, nametable.buffer);
    draw_debug_image(768, 0, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, &nametable);
}
