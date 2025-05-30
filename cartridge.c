//
// Created by tromo on 5/24/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cartridge.h"
#include "nes-internal.h"

#include "stdint.h"

const int NES_TAG_1 = 0x00;
const int NES_TAG_2 = 0x01;
const int NES_TAG_3 = 0x02;
const int NES_TAG_4 = 0x03;

const int INES_DETECTION = 0x0C;

// INES
const int INES_PRG_ROM_SIZE = 0x04;
const int INES_CHR_ROM_SIZE = 0x05;
const int INES_CONTROL_1 = 0x06;
const int INES_CONTROL_2 = 0x07;
const int INES_PRG_RAM_SIZE = 0x08;

const int INES_PRG_ROM_BANK_SIZE = 0x4000;
const int INES_CHR_ROM_BANK_SIZE = 0x2000;
const int INES_PRG_RAM_BANK_SIZE = 0x2000;

// INES 2.0
const int NES2_TMP = 0x07;

const int HEADER_SIZE = 0x10;
const int TRAINER_SIZE = 0x200;


struct Cartridge* nes_cartridge_load_from_file(const char* file_path) {

    FILE* file = fopen(file_path, "r");
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = malloc(file_size * sizeof(char));

    for(int i = 0; i < file_size; i++) {
        fread(file_contents + i, 1, 1, file);
    }

    struct Cartridge* cartridge = nes_cartridge_load_from_buffer(file_contents, file_size);

    fclose(file);
    free(file_contents);

    return cartridge;
}

struct Cartridge* nes_cartridge_load_from_buffer(const uint8_t* buffer, const long size) {
    struct Cartridge* cartridge = malloc(sizeof(struct Cartridge));
    if (!(buffer[NES_TAG_1] == 'N' &&
          buffer[NES_TAG_2] == 'E' &&
          buffer[NES_TAG_3] == 'S' &&
          buffer[NES_TAG_4] == 0x1A)) {
        printf("Not a valid cartridge file!!!\n");

        return nullptr;
    }

    const bool is_ines = buffer[INES_DETECTION] == 0x00;
    const bool is_nes2 = buffer[INES_DETECTION] == 0x08;

    if (!is_ines && !is_nes2) {
        printf("Not a valid cartridge file!!!\n");
        return nullptr;
    }

    const int prg_rom_size = buffer[INES_PRG_ROM_SIZE] * INES_PRG_ROM_BANK_SIZE;
    int chr_rom_size = buffer[INES_CHR_ROM_SIZE] * INES_CHR_ROM_BANK_SIZE;
    int prg_ram_size = buffer[INES_PRG_RAM_SIZE] * INES_PRG_RAM_BANK_SIZE;

    if (!prg_ram_size)
        prg_ram_size = INES_PRG_RAM_BANK_SIZE;

    const bool uses_chr_ram = chr_rom_size == 0;
    if (!chr_rom_size)
        chr_rom_size = INES_CHR_ROM_BANK_SIZE;

    enum Mirroring mirroring = (enum Mirroring)buffer[INES_CONTROL_1] & 0b1;
    const bool battery_backed = buffer[INES_CONTROL_1] & 0b10;
    const bool skip_trainer = buffer[INES_CONTROL_1] & 0b100;

    if (buffer[INES_CONTROL_1] & 0b1000)
        mirroring = Mirroring_FourScreen;

    const int mapper_lo = (buffer[INES_CONTROL_1] & 0b11110000) >> 4;
    const int mapper_hi = (buffer[INES_CONTROL_2] & 0b11110000);

    const int mapper =  mapper_hi | mapper_lo;

    if (is_nes2) {
        printf("NES 2.0 not supported yet!!!\n");
        return nullptr;
    }

    cartridge->mapper = mapper;
    cartridge->mirroring = mirroring;

    cartridge->prg_rom_size = prg_rom_size;
    cartridge->prg_ram_size = prg_ram_size;
    cartridge->chr_rom_size = chr_rom_size;

    cartridge->prg_rom = malloc(sizeof(uint8_t) * prg_rom_size);
    cartridge->prg_ram = malloc(sizeof(uint8_t) * prg_ram_size);
    cartridge->chr_rom = malloc(sizeof(uint8_t) * chr_rom_size);

    int prg_rom_start = HEADER_SIZE;
    if (skip_trainer)
        prg_rom_start += TRAINER_SIZE;

    memcpy(cartridge->prg_rom, &buffer[prg_rom_start], prg_rom_size * sizeof(uint8_t));

    if (!uses_chr_ram) {
        const int chr_rom_start = prg_rom_start + prg_rom_size;
        memcpy(cartridge->chr_rom, &buffer[chr_rom_start], chr_rom_size * sizeof(uint8_t));
    }

    return cartridge;
}

void nes_cartridge_free(struct Cartridge* cartridge) {
    free(cartridge->prg_rom);
    free(cartridge->prg_ram);
    free(cartridge->chr_rom);
    free(cartridge);
}

const uint16_t PRG_ROM_START = 0x8000;
const uint16_t PRG_ROM_END = 0xFFFF;
const uint16_t RAM_MIRRORS_END = 0x1fff;
const uint16_t PPU_MIRRORS_END = 0x3fff;

inline uint8_t* nes_cartridge_get_addr_ptr(const struct Cartridge* cartridge, uint16_t addr) {
    if (addr <= RAM_MIRRORS_END) {
        return &cartridge->prg_ram[addr & 0x7FF];
    } else if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        return &cartridge->prg_rom[addr];
    }

    printf("Tried reading non-implemented address %04x\n", addr);
    return &cartridge->prg_ram[0];
}

inline uint8_t nes_cartridge_read_char(const struct Cartridge* cartridge, uint16_t addr) {
    if (addr <= RAM_MIRRORS_END) {
        return cartridge->prg_ram[addr & 0x7FF];
    } else if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        return cartridge->prg_rom[addr];
    }

    return 0;
}
inline void nes_cartridge_write_char(const struct Cartridge* cartridge, uint16_t addr, uint8_t val) {
    if (addr <= RAM_MIRRORS_END) {
        if (addr == 0x02 || addr == 0x03)
            printf("%d\\n", addr);

        cartridge->prg_ram[addr & 0x7FF] = val;
    } else if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        cartridge->prg_rom[addr] = val;
    }
}