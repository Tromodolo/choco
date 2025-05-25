//
// Created by tromo on 5/24/25.
//

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

struct Cartridge* nes_cartridge_load_from_file(const char* file_path);
void nes_cartridge_free(struct Cartridge* cartridge);

unsigned char nes_cartridge_read_char(const struct Cartridge* cartridge, unsigned short addr);
void nes_cartridge_write_char(const struct Cartridge* cartridge, unsigned short addr, unsigned char val);

#endif //CARTRIDGE_H
