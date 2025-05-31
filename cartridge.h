#ifndef CARTRIDGE_H
#define CARTRIDGE_H
#include <stdint.h>

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

struct Cartridge* nes_cartridge_load_from_file(const char* file_path);
struct Cartridge* nes_cartridge_load_from_buffer(const uint8_t* buffer, const long size);
void nes_cartridge_free(struct Cartridge* cartridge);

uint8_t* nes_cartridge_get_addr_ptr(const struct Cartridge* cartridge, uint16_t addr);

uint8_t nes_cartridge_read_char(const struct Cartridge* cartridge, uint16_t addr);
void nes_cartridge_write_char(const struct Cartridge* cartridge, uint16_t addr, uint8_t val);

#endif //CARTRIDGE_H
