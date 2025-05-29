//
// Created by tromo on 5/25/25.
//

#ifndef NESTYPES_H
#define NESTYPES_H
#include <stdint.h>

struct Cartridge;
struct CPU;

//
// NES
//
struct Nes {
    struct Cartridge* cartridge;
    struct CPU* cpu;
};

//
// Cartridge
//
enum Mapper {
    Mapper_NRom = 0,
};

enum Mirroring {
    Mirroring_Vertical = 0,
    Mirroring_Horizontal = 1,
    Mirroring_FourScreen = 2,
    // Mirroring_OneScreenLower = 3,
    // Mirroring_OneScreenUpper = 4
};

struct Cartridge {
    int prg_rom_size;
    int chr_rom_size;
    int prg_ram_size;
    uint8_t* prg_rom;
    uint8_t* chr_rom;
    uint8_t* prg_ram;
    enum Mapper mapper;
    enum Mirroring mirroring;
};

//
// CPU
//
typedef union
{
    struct
    {
        uint8_t carry : 1;
        uint8_t zero : 1;
        uint8_t interrupt_disable : 1;
        uint8_t decimal : 1;
        uint8_t _unused_1 : 1;
        uint8_t _unused_2 : 1;
        uint8_t overflow : 1;
        uint8_t negative : 1;
    } flags;
    uint8_t value;
} Flags;

struct CPU {
    uint8_t acc;
    uint8_t x;
    uint8_t y;
    uint16_t pc;
    uint16_t pc_pre;
    uint8_t sp;
    uint8_t waiting_cycles;

    uint8_t current_instruction;

    bool is_stopped;

    Flags p;
};

#endif //NESTYPES_H
