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

struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size) {
    struct Nes* nes = malloc(sizeof(struct Nes));

    nes->cartridge = nes_cartridge_load_from_buffer(buffer, size);
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


// local variables just for logging debug info
char* cpu_reg = nullptr;
char* ppu_reg_1 = nullptr;
char* ppu_reg_2 = nullptr;
char* ppu_reg_3 = nullptr;
void nes_draw_debug_info(const struct Nes* nes) {
    if (!cpu_reg) {
        cpu_reg = malloc(sizeof(char) * 34);
    }
    if (!ppu_reg_1) {
        ppu_reg_1 = malloc(sizeof(char) * 35);
    }
    if (!ppu_reg_2) {
        ppu_reg_2 = malloc(sizeof(char) * 58);
    }
    if (!ppu_reg_3) {
        ppu_reg_3 = malloc(sizeof(char) * 58);
    }

    const int left_padding = (SCREEN_WIDTH * 2) + 10;
    const int top_padding = 10;

    const int header_padding = left_padding + 15;
    const int subheader_padding = header_padding + 15;

    DrawText("Registers", left_padding, top_padding, 24, WHITE);
    DrawText("CPU", header_padding, top_padding + 25, 22, WHITE);

    sprintf(
        cpu_reg,
        "PC: %04X A:%02X X:%02X Y:%02X P:%02X SP:%02X",
        nes->cpu->pc,
        nes->cpu->acc,
        nes->cpu->x,
        nes->cpu->y,
        nes->cpu->p.value,
        nes->cpu->sp
    );
    DrawText(cpu_reg, subheader_padding, top_padding + 50, 20, WHITE);

    DrawText("PPU", header_padding, top_padding + 75, 22, WHITE);

    sprintf(
        ppu_reg_1,
        "X:%03d Y:%03d CTRL:%02X MASK:%02X STATUS:%02X",
        nes->ppu->dots_drawn,
        nes->ppu->current_scanline,
        nes->ppu->control_register.value,
        nes->ppu->mask_register.value,
        nes->ppu->status_register.value
    );
    DrawText(ppu_reg_1, subheader_padding, top_padding + 100, 20, WHITE);

    DrawText("Loopy", header_padding + 7, top_padding + 125, 22, WHITE);
    sprintf(
        ppu_reg_2,
        "Coarse X:%02d Coarse Y:%02d Na_X:%d NT_Y:%d Fine Y:%d Fine X:%d",
        nes->ppu->loopy_value.coarse_x,
        nes->ppu->loopy_value.coarse_y,
        nes->ppu->loopy_value.nametable_x,
        nes->ppu->loopy_value.nametable_y,
        nes->ppu->loopy_value.fine_y,
        nes->ppu->scroll_fine_x
    );
    DrawText(ppu_reg_2, subheader_padding, top_padding + 150, 20, WHITE);

    DrawText("Loopy TMP", header_padding + 7, top_padding + 175, 22, WHITE);
    sprintf(
        ppu_reg_3,
        "Coarse X:%02d Coarse Y:%02d Na_X:%d NT_Y:%d Fine Y:%d",
        nes->ppu->loopy_temp.coarse_x,
        nes->ppu->loopy_temp.coarse_y,
        nes->ppu->loopy_temp.nametable_x,
        nes->ppu->loopy_temp.nametable_y,
        nes->ppu->loopy_temp.fine_y
    );
    DrawText(ppu_reg_3, subheader_padding, top_padding + 200, 20, WHITE);
}
