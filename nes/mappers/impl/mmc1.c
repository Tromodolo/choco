//
// Created by tromo on 11/25/25.
//

#include "../../cartridge.h"
#include "mmc1.h"

#include <stdio.h>
#include <stdlib.h>

constexpr int PRG_BANK_SIZE = 0x4000;
constexpr int CHR_BANK_SIZE = 0x1000;

void update_prg_banks(const struct Cartridge* cartridge, uint8_t val);
void update_chr_banks(const struct Cartridge* cartridge);
void set_control(const struct Cartridge* cartridge, uint8_t val);
void persist_save_file(const struct Cartridge* cartridge, uint32_t updated_pos);

struct Mmc1 {
    bool has_prg_ram;

    uint8_t prg_rom_bank_raw;

    uint8_t prg_rom_bank_idx_0;
    uint8_t prg_rom_bank_idx_1;

    uint8_t chr_rom_bank_idx_0;
    uint8_t chr_rom_bank_idx_1;

    uint8_t* prg_rom_bank_0;
    uint8_t* prg_rom_bank_1;

    uint8_t* chr_rom_bank_0;
    uint8_t* chr_rom_bank_1;

    uint8_t prg_mode;
    uint8_t chr_mode;

    uint8_t shift_register;
    uint8_t shift_count;

    uint16_t current_pc;
    uint16_t last_pc;

    uint8_t last_control;

    enum Mirroring current_mirroring;

    FILE* save_file;
};

void mmc1_init(struct Cartridge* cartridge) {
    struct Mmc1* mmc1 = malloc(sizeof(struct Mmc1));

    mmc1->has_prg_ram = cartridge->prg_ram_size > 0;

    mmc1->prg_rom_bank_idx_0 = 0;
    mmc1->prg_rom_bank_idx_1 = (cartridge->prg_rom_size - PRG_BANK_SIZE) / PRG_BANK_SIZE;

    mmc1->chr_rom_bank_idx_0 = 0;
    mmc1->chr_rom_bank_idx_1 = 1;

    mmc1->prg_mode = 0;
    mmc1->chr_mode = 0;

    mmc1->shift_register = 0;
    mmc1->shift_count = 0;

    mmc1->current_pc = 0;
    mmc1->last_pc = 0;

    mmc1->last_control = 0;

    mmc1->current_mirroring = cartridge->mirroring;

    cartridge->mapper = mmc1;

    mmc1->prg_rom_bank_0 = &cartridge->prg_rom[mmc1->prg_rom_bank_idx_0 * PRG_BANK_SIZE];
    mmc1->prg_rom_bank_1 = &cartridge->prg_rom[mmc1->prg_rom_bank_idx_1 * PRG_BANK_SIZE];

    mmc1->chr_rom_bank_0 = &cartridge->chr_rom[mmc1->chr_rom_bank_idx_0 * CHR_BANK_SIZE];
    mmc1->chr_rom_bank_1 = &cartridge->chr_rom[mmc1->chr_rom_bank_idx_1 * CHR_BANK_SIZE];

    mmc1->save_file = fopen("test.sav", "r+");
    fseek(mmc1->save_file, 0, SEEK_END);
    const long file_size = ftell(mmc1->save_file);
    fseek(mmc1->save_file, 0, SEEK_SET);

    for(int i = 0; i < file_size; i++) {
        fread(cartridge->prg_ram + i, 1, 1, mmc1->save_file);
    }
}

void mmc1_free(struct Cartridge* cartridge) {
    const struct Mmc1* mmc1 = cartridge->mapper;
    fflush(mmc1->save_file);
    fclose(mmc1->save_file);
    free(cartridge->mapper);
    cartridge->mapper = nullptr;
}

void persist_save_file(const struct Cartridge* cartridge, const uint32_t updated_pos) {
    const struct Mmc1* mmc1 = cartridge->mapper;
    fseek(mmc1->save_file, updated_pos, SEEK_SET);
    fwrite(&cartridge->prg_ram[updated_pos], 1, 1, mmc1->save_file);
}

uint8_t mmc1_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc1* mmc1 = cartridge->mapper;
    if (addr >= 0x6000 && addr < 0x8000 && mmc1->has_prg_ram) {
        *is_mapped = true;
        addr -= 0x6000;
        return cartridge->prg_ram[addr];
    } else if (addr >= 0x8000 && addr < 0xC000) {
        *is_mapped = true;
        addr -= 0x8000;
        return mmc1->prg_rom_bank_0[addr];
    } else if (addr >= 0xC000) {
        *is_mapped = true;
        addr -= 0xC000;
        return mmc1->prg_rom_bank_1[addr];
    }

    return 0;
}
void mmc1_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    struct Mmc1* mmc1 = cartridge->mapper;
    if (addr >= 0x6000 && addr < 0x8000 && mmc1->has_prg_ram) {
        *is_mapped = true;
        addr -= 0x6000;
        cartridge->prg_ram[addr] = val;
        persist_save_file(cartridge, addr);
    } else if (addr >= 0x8000) {
        if (mmc1->last_pc == mmc1->current_pc) {
            return;
        }
        *is_mapped = true;
        mmc1->last_pc = mmc1->current_pc;

        // If control bit is set, reset shift register and control
        if (val & 0x80) {
            mmc1->shift_register = 0;
            mmc1->shift_count = 0;

            // This will reset PRG mode to 3 at a very minimum
            // Everything else will be set as it was before
            set_control(cartridge, mmc1->last_control | 0x0c);
        } else {
            mmc1->shift_register >>= 1;
            mmc1->shift_register |= (val & 1) << 4;

            if (mmc1->shift_count != 4) {
                mmc1->shift_count++;
            } else {
                val = mmc1->shift_register & 0b11111;

                mmc1->shift_register = 0;
                mmc1->shift_count = 0;

                if (addr >= 0x8000 && addr <= 0x9FFF)  { // Control
                    set_control(cartridge, val);
                } else if (addr >= 0xA000 && addr <= 0xBFFF) { // CHR bank 0
                    mmc1->chr_rom_bank_idx_0 = val;
                } else if (addr >= 0xC000 && addr <= 0xDFFF) { // CHR bank 1
                    mmc1->chr_rom_bank_idx_1 = val;
                } else if (addr >= 0xE000 && addr <= 0xFFFF) { // PRG bank
                    update_prg_banks(cartridge, val);
                }
            }
        }
    }

    update_chr_banks(cartridge);
}

uint8_t mmc1_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc1* mmc1 = cartridge->mapper;
    if (addr < 0x1000) {
        *is_mapped = true;
        return mmc1->chr_rom_bank_0[addr];
    } else if (addr >= 0x1000 && addr < 0x2000) {
        *is_mapped = true;
        addr -= 0x1000;
        return mmc1->chr_rom_bank_1[addr];
    }

    return 0;
}

void mmc1_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc1* mmc1 = cartridge->mapper;
    if (addr < 0x1000) {
        *is_mapped = true;
        mmc1->chr_rom_bank_0[addr] = val;
    } else if (addr >= 0x1000 && addr < 0x2000) {
        *is_mapped = true;
        addr -= 0x1000;
        mmc1->chr_rom_bank_1[addr] = val;
    }
}

void mmc1_set_pc(const struct Cartridge* cartridge, uint16_t pc) {
    struct Mmc1* mmc1 = cartridge->mapper;
    mmc1->current_pc = pc;
}

enum Mirroring mmc1_get_mirroring(const struct Cartridge* cartridge) {
    const struct Mmc1* mmc1 = cartridge->mapper;
    return mmc1->current_mirroring;
}

void update_prg_banks(const struct Cartridge* cartridge, uint8_t val) {
    struct Mmc1* mmc1 = cartridge->mapper;

    mmc1->has_prg_ram = !((val & 0b00010000) >> 4);
    mmc1->prg_rom_bank_raw = val;

    val &= 0b00011111;
    switch (mmc1->prg_mode) {
        case 0:
        case 1:
            mmc1->prg_rom_bank_idx_0 = val;
            mmc1->prg_rom_bank_idx_1 = val + 1;
            break;
        case 2:
            mmc1->prg_rom_bank_idx_0 = 0;
            mmc1->prg_rom_bank_idx_1 = val;
            break;
        case 3:
            mmc1->prg_rom_bank_idx_0 = val;
            mmc1->prg_rom_bank_idx_1 = (cartridge->prg_rom_size - PRG_BANK_SIZE) / PRG_BANK_SIZE;
            break;
        default:
            break;
    }

    mmc1->prg_rom_bank_0 = &cartridge->prg_rom[mmc1->prg_rom_bank_idx_0 * PRG_BANK_SIZE];
    mmc1->prg_rom_bank_1 = &cartridge->prg_rom[mmc1->prg_rom_bank_idx_1 * PRG_BANK_SIZE];
}

void update_chr_banks(const struct Cartridge* cartridge) {
    struct Mmc1* mmc1 = cartridge->mapper;

    if (mmc1->chr_mode == 0) {
        uint8_t bank_1_addr = mmc1->chr_rom_bank_idx_0;
        bank_1_addr &= ~1;
        bank_1_addr *= CHR_BANK_SIZE;

        mmc1->chr_rom_bank_0 = &cartridge->chr_rom[bank_1_addr];
        mmc1->chr_rom_bank_1 = &cartridge->chr_rom[bank_1_addr + CHR_BANK_SIZE];
    } else {
        mmc1->chr_rom_bank_0 = &cartridge->chr_rom[mmc1->chr_rom_bank_idx_0 * CHR_BANK_SIZE];
        mmc1->chr_rom_bank_1 = &cartridge->chr_rom[mmc1->chr_rom_bank_idx_1 * CHR_BANK_SIZE];
    }
}

void set_control(const struct Cartridge* cartridge, uint8_t val) {
    struct Mmc1* mmc1 = cartridge->mapper;

    mmc1->last_control = val;
    switch (val & 0b11) {
        case 0:
            mmc1->current_mirroring = Mirroring_OneScreenLower;
            break;
        case 1:
            mmc1->current_mirroring = Mirroring_OneScreenUpper;
            break;
        case 2:
            mmc1->current_mirroring = Mirroring_Vertical;
            break;
        case 3:
            mmc1->current_mirroring = Mirroring_Horizontal;
            break;
        default:
            break;
    }
    mmc1->prg_mode = (val & 0b00001100) >> 2;
    mmc1->chr_mode = (val & 0b00010000) >> 4;

    update_chr_banks(cartridge);
    update_prg_banks(cartridge, mmc1->prg_rom_bank_raw);
}