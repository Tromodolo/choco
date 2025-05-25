//
// Created by tromo on 5/25/25.
//

#ifndef NESTYPES_H
#define NESTYPES_H

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
    unsigned char* prg_rom;
    unsigned char* chr_rom;
    unsigned char* prg_ram;
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
        unsigned char carry : 1;
        unsigned char zero : 1;
        unsigned char interrupt_disable : 1;
        unsigned char decimal : 1;
        unsigned char _unused_1 : 1;
        unsigned char _unused_2 : 1;
        unsigned char overflow : 1;
        unsigned char negative : 1;
    } flags;
    unsigned char value;
} Flags;

struct CPU {
    unsigned char acc;
    unsigned char x;
    unsigned char y;
    unsigned short pc;
    unsigned char sp;
    Flags p;
};

#endif //NESTYPES_H
