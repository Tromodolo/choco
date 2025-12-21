//
// Created by tromo on 11/25/25.
//

#include "../../cartridge.h"
#include "mmc3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr int PRG_BANK_SIZE = 0x2000;
constexpr int CHR_BANK_SIZE = 0x400;

void update_banks(const struct Cartridge* cartridge);

struct Mmc3 {
    bool has_prg_ram;

    uint8_t prg_mode;
    uint8_t chr_mode;

    uint8_t next_bank_update;
    uint8_t last_bank;

    uint8_t* bank_registers;
    uint8_t** prg_banks;
    uint8_t** chr_banks;

    uint16_t irq_latch;
    uint16_t irq_counter;
    bool irq_enabled;
    bool irq_reload;
    bool irq_pending;

    enum Mirroring current_mirroring;

    FILE* save_file;
};

void mmc3_init(struct Cartridge* cartridge, const char* file_path) {
    struct Mmc3* mmc3 = malloc(sizeof(struct Mmc3));

    mmc3->has_prg_ram = cartridge->prg_ram_size > 0;

    mmc3->prg_mode = 0;
    mmc3->chr_mode = 0;

    mmc3->next_bank_update = 0;
    mmc3->last_bank = (cartridge->prg_rom_size - PRG_BANK_SIZE) / PRG_BANK_SIZE;

    mmc3->bank_registers = malloc(8 * sizeof(uint8_t));
    mmc3->bank_registers[0] = 0;
    mmc3->bank_registers[1] = 0;
    mmc3->bank_registers[2] = 0;
    mmc3->bank_registers[3] = 0;
    mmc3->bank_registers[4] = 0;
    mmc3->bank_registers[5] = 0;
    mmc3->bank_registers[6] = 0;
    mmc3->bank_registers[7] = 0;

    mmc3->prg_banks = malloc(4 * sizeof(uint8_t*));
    mmc3->prg_banks[0] = nullptr;
    mmc3->prg_banks[1] = nullptr;
    mmc3->prg_banks[2] = nullptr;
    mmc3->prg_banks[3] = nullptr;

    mmc3->chr_banks = malloc(8 * sizeof(uint8_t*));
    mmc3->chr_banks[0] = nullptr;
    mmc3->chr_banks[1] = nullptr;
    mmc3->chr_banks[2] = nullptr;
    mmc3->chr_banks[3] = nullptr;
    mmc3->chr_banks[4] = nullptr;
    mmc3->chr_banks[5] = nullptr;
    mmc3->chr_banks[6] = nullptr;
    mmc3->chr_banks[7] = nullptr;

    mmc3->irq_latch = 0;
    mmc3->irq_counter = 0;
    mmc3->irq_enabled = false;
    mmc3->irq_reload = false;
    mmc3->irq_pending = false;

    mmc3->current_mirroring = cartridge->mirroring;

    cartridge->mapper = mmc3;

    char* file_path_new = malloc(1024 * sizeof(char));
    strcpy(file_path_new, file_path);
    strcat(file_path_new, ".sav");

    mmc3->save_file = fopen(file_path_new, "r+");
    if (mmc3->save_file == nullptr) {
        mmc3->save_file = fopen(file_path_new, "w+");
    }
    //
    // fseek(mmc3->save_file, 0, SEEK_END);
    // const long file_size = ftell(mmc3->save_file);
    // fseek(mmc3->save_file, 0, SEEK_SET);
    //
    // for(int i = 0; i < file_size; i++) {
    //     fread(cartridge->prg_ram + i, 1, 1, mmc3->save_file);
    // }

    free(file_path_new);

    update_banks(cartridge);
}

void mmc3_free(struct Cartridge* cartridge) {
    const struct Mmc3* mmc3 = cartridge->mapper;

    // Flush the entire prg_ram to disk
    fwrite(cartridge->prg_ram, cartridge->prg_ram_size, 1, mmc3->save_file);
    fflush(mmc3->save_file);

    fclose(mmc3->save_file);
    free(cartridge->mapper);
    cartridge->mapper = nullptr;
}

uint8_t mmc3_cpu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc3* mmc3 = cartridge->mapper;
    if (addr >= 0x6000 && addr < 0x8000 && mmc3->has_prg_ram) {
        *is_mapped = true;
        addr -= 0x6000;
        return cartridge->prg_ram[addr];
    } else if (addr >= 0x8000) {
        *is_mapped = true;
        const int bank_idx = (addr - 0x8000) / PRG_BANK_SIZE;
        const uint8_t* bank = mmc3->prg_banks[bank_idx];
        return bank[addr % PRG_BANK_SIZE];
    }

    return 0;
}
void mmc3_cpu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    struct Mmc3* mmc3 = cartridge->mapper;
    if (addr >= 0x6000 && addr < 0x8000 && mmc3->has_prg_ram) {
        *is_mapped = true;
        addr -= 0x6000;
        cartridge->prg_ram[addr] = val;
    } else if (addr >= 0x8000) {
        *is_mapped = true;

        const bool is_even = addr % 2 == 0;
        if (addr < 0xA000) {
            if (is_even) {
                mmc3->next_bank_update = val & 0b111;
                mmc3->prg_mode = (val & 0b01000000) >> 6;
                mmc3->chr_mode = (val & 0b10000000) >> 7;
            } else {
                mmc3->bank_registers[mmc3->next_bank_update] = val;
            }
        } else if (addr < 0xC000) {
            if (is_even) { // Mirroring
                if (mmc3->current_mirroring == Mirroring_FourScreen) {
                    return;
                }
                mmc3->current_mirroring = (val & 1) == 0
                    ? Mirroring_Vertical
                    : Mirroring_Horizontal;
            } else { // Prg Ram Flags
                //7  bit  0
                //--------
                //RWXX xxxx
                //||||
                //|| ++------Nothing on the MMC3, see MMC6
                //| +--------Write protection(0: allow writes; 1: deny writes)
                //+---------PRG RAM chip enable(0: disable; 1: enable)
                //Disabling PRG RAM through bit 7 causes reads from the PRG RAM region to return open bus.
                //Though these bits are functional on the MMC3, their main purpose is to write - protect save RAM during power-off.Many emulators choose not to implement them as part of iNES Mapper 4 to avoid an incompatibility with the MMC6.
                //See iNES Mapper 004 and MMC6 below.
            }
        } else if (addr < 0xE000) {
            if (is_even) {
                mmc3->irq_latch = val;
            } else {
                mmc3->irq_counter = 0;
                mmc3->irq_reload = true;
            }
        } else if (addr <= 0xFFFF) {
            if (is_even) {
                mmc3->irq_enabled = false;
                mmc3->irq_pending = false;
            } else {
                mmc3->irq_enabled = true;
            }
        }
    }

    if (*is_mapped) {
        update_banks(cartridge);
    }
}

uint8_t mmc3_ppu_read(const struct Cartridge* cartridge, uint16_t addr, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc3* mmc3 = cartridge->mapper;
    if (addr < 0x2000) {
        *is_mapped = true;
        const int bank_idx = addr / CHR_BANK_SIZE;
        const uint8_t* bank = mmc3->chr_banks[bank_idx];
        return bank[addr % CHR_BANK_SIZE];
    }

    return 0;
}

void mmc3_ppu_write(const struct Cartridge* cartridge, uint16_t addr, uint8_t val, bool* is_mapped) {
    *is_mapped = false;

    const struct Mmc3* mmc3 = cartridge->mapper;
    if (addr < 0x2000) {
        *is_mapped = true;
        const int bank_idx = addr / CHR_BANK_SIZE;
        uint8_t* bank = mmc3->chr_banks[bank_idx];
        bank[addr % CHR_BANK_SIZE] = val;
    }
}

void mmc3_decrement_scanline(const struct Cartridge* cartridge) {
    struct Mmc3* mmc3 = cartridge->mapper;
    if (mmc3->irq_counter == 0 || mmc3->irq_reload) {
        mmc3->irq_counter = mmc3->irq_latch;
    } else {
        mmc3->irq_counter--;
    }

    if (mmc3->irq_counter == 0 && mmc3->irq_enabled) {
        mmc3->irq_pending = true;
    }

    mmc3->irq_reload = false;
}

bool mmc3_get_irq(const struct Cartridge* cartridge) {
    const struct Mmc3* mmc3 = cartridge->mapper;
    return mmc3->irq_pending;
}

void mmc3_set_irq(const struct Cartridge* cartridge, bool set) {
    struct Mmc3* mmc3 = cartridge->mapper;
    mmc3->irq_pending = set;
}

enum Mirroring mmc3_get_mirroring(const struct Cartridge* cartridge) {
    const struct Mmc3* mmc3 = cartridge->mapper;
    return mmc3->current_mirroring;
}

static inline void set_prg_bank(const struct Cartridge* cartridge, struct Mmc3* mmc3, uint8_t idx, uint8_t val) {
    const int addr = (val * PRG_BANK_SIZE) % cartridge->prg_rom_size;
    mmc3->prg_banks[idx] = &cartridge->prg_rom[addr];
}

static inline void set_chr_bank(const struct Cartridge* cartridge, struct Mmc3* mmc3, uint8_t idx, uint8_t val) {
    const int addr = (val * CHR_BANK_SIZE) % cartridge->chr_rom_size;
    mmc3->chr_banks[idx] = &cartridge->chr_rom[addr];
}

void update_banks(const struct Cartridge* cartridge) {
    struct Mmc3* mmc3 = cartridge->mapper;

    if (mmc3->prg_mode) {
        set_prg_bank(cartridge, mmc3, 0, mmc3->last_bank - 1);
        set_prg_bank(cartridge, mmc3, 1, mmc3->bank_registers[7]);
        set_prg_bank(cartridge, mmc3, 2, mmc3->bank_registers[6]);
        set_prg_bank(cartridge, mmc3, 3, mmc3->last_bank);
    } else {
        set_prg_bank(cartridge, mmc3, 0, mmc3->bank_registers[6]);
        set_prg_bank(cartridge, mmc3, 1, mmc3->bank_registers[7]);
        set_prg_bank(cartridge, mmc3, 2, mmc3->last_bank - 1);
        set_prg_bank(cartridge, mmc3, 3, mmc3->last_bank);
    }

    if (mmc3->chr_mode) {
        set_chr_bank(cartridge, mmc3, 0, mmc3->bank_registers[2]);
        set_chr_bank(cartridge, mmc3, 1, mmc3->bank_registers[3]);
        set_chr_bank(cartridge, mmc3, 2, mmc3->bank_registers[4]);
        set_chr_bank(cartridge, mmc3, 3, mmc3->bank_registers[5]);
        set_chr_bank(cartridge, mmc3, 4, mmc3->bank_registers[0] & ~1);
        set_chr_bank(cartridge, mmc3, 5, mmc3->bank_registers[0] | 1);
        set_chr_bank(cartridge, mmc3, 6, mmc3->bank_registers[1] & ~1);
        set_chr_bank(cartridge, mmc3, 7, mmc3->bank_registers[1] | 1);
    } else {
        set_chr_bank(cartridge, mmc3, 0, mmc3->bank_registers[0] & ~1);
        set_chr_bank(cartridge, mmc3, 1, mmc3->bank_registers[0] | 1);
        set_chr_bank(cartridge, mmc3, 2, mmc3->bank_registers[1] & ~1);
        set_chr_bank(cartridge, mmc3, 3, mmc3->bank_registers[1] | 1);
        set_chr_bank(cartridge, mmc3, 4, mmc3->bank_registers[2]);
        set_chr_bank(cartridge, mmc3, 5, mmc3->bank_registers[3]);
        set_chr_bank(cartridge, mmc3, 6, mmc3->bank_registers[4]);
        set_chr_bank(cartridge, mmc3, 7, mmc3->bank_registers[5]);
    }
}