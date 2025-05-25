//
// Created by tromo on 5/24/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cartridge.h"
#include "nes-internal.h"

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
    struct Cartridge* cartridge = malloc(sizeof(struct Cartridge));

    FILE* file = fopen(file_path, "r");
    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = malloc(file_size * sizeof(char));

    for(int i = 0; i < file_size; i++) {
        fread(file_contents + i, 1, 1, file);
    }

    if (!(file_contents[NES_TAG_1] == 'N' &&
          file_contents[NES_TAG_2] == 'E' &&
          file_contents[NES_TAG_3] == 'S' &&
          file_contents[NES_TAG_4] == 0x1A)) {
        printf("Not a valid cartridge file!!!\n");

        fclose(file);
        free(file_contents);
        return nullptr;
    }

    const bool is_ines = file_contents[INES_DETECTION] == 0x00;
    const bool is_nes2 = file_contents[INES_DETECTION] == 0x08;

    if (!is_ines && !is_nes2) {
        printf("Not a valid cartridge file!!!\n");

        fclose(file);
        free(file_contents);
        return nullptr;
    }

    const int prg_rom_size = file_contents[INES_PRG_ROM_SIZE] * INES_PRG_ROM_BANK_SIZE;
    int chr_rom_size = file_contents[INES_CHR_ROM_SIZE] * INES_CHR_ROM_BANK_SIZE;
    int prg_ram_size = file_contents[INES_PRG_RAM_SIZE] * INES_PRG_RAM_BANK_SIZE;

    if (!prg_ram_size)
        prg_ram_size = INES_PRG_RAM_BANK_SIZE;

    const bool uses_chr_ram = chr_rom_size == 0;
    if (!chr_rom_size)
        chr_rom_size = INES_CHR_ROM_BANK_SIZE;

    enum Mirroring mirroring = (enum Mirroring)file_contents[INES_CONTROL_1] & 0b1;
    const bool battery_backed = file_contents[INES_CONTROL_1] & 0b10;
    const bool skip_trainer = file_contents[INES_CONTROL_1] & 0b100;

    if (file_contents[INES_CONTROL_1] & 0b1000)
        mirroring = Mirroring_FourScreen;

    const int mapper_lo = (file_contents[INES_CONTROL_1] & 0b11110000) >> 4;
    const int mapper_hi = (file_contents[INES_CONTROL_2] & 0b11110000);

    const int mapper =  mapper_hi | mapper_lo;

    if (is_nes2) {
        printf("NES 2.0 not supported yet!!!\n");

        fclose(file);
        free(file_contents);
        return nullptr;
    }

    cartridge->mapper = mapper;
    cartridge->mirroring = mirroring;

    cartridge->prg_rom_size = prg_rom_size;
    cartridge->prg_ram_size = prg_ram_size;
    cartridge->chr_rom_size = chr_rom_size;

    cartridge->prg_rom = malloc(sizeof(unsigned char) * prg_rom_size);
    cartridge->prg_ram = malloc(sizeof(unsigned char) * prg_ram_size);
    cartridge->chr_rom = malloc(sizeof(unsigned char) * chr_rom_size);

    int prg_rom_start = HEADER_SIZE;
    if (skip_trainer)
        prg_rom_start += TRAINER_SIZE;

    memcpy(cartridge->prg_rom, &file_contents[prg_rom_start], prg_rom_size * sizeof(unsigned char));

    if (!uses_chr_ram) {
        const int chr_rom_start = prg_rom_start + prg_rom_size;
        memcpy(cartridge->chr_rom, &file_contents[chr_rom_start], chr_rom_size * sizeof(unsigned char));
    }

    fclose(file);
    free(file_contents);

    return cartridge;
}

void nes_cartridge_free(struct Cartridge* cartridge) {
    free(cartridge->prg_rom);
    free(cartridge->prg_ram);
    free(cartridge->chr_rom);
    free(cartridge);
}

const unsigned short PRG_ROM_START = 0x8000;
const unsigned short PRG_ROM_END = 0xFFFF;
const unsigned short RAM_MIRRORS_END = 0x1fff;
const unsigned short PPU_MIRRORS_END = 0x3fff;

inline unsigned char* nes_cartridge_get_addr_ptr(const struct Cartridge* cartridge, unsigned short addr) {
    if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        return &cartridge->prg_rom[addr];
    }

    return nullptr;
}

inline unsigned char nes_cartridge_read_char(const struct Cartridge* cartridge, unsigned short addr) {
    if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        return cartridge->prg_rom[addr];
    }

    return 0;
}
inline void nes_cartridge_write_char(const struct Cartridge* cartridge, unsigned short addr, unsigned char val) {
    if (addr >= PRG_ROM_START && addr <= PRG_ROM_END) {
        addr -= PRG_ROM_START;
        addr %= cartridge->prg_rom_size;
        cartridge->prg_rom[addr] = val;
    }
}