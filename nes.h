//
// Created by tromo on 5/19/25.
//

#ifndef NES_H
#define NES_H
#include <raylib.h>

struct Nes* nes_init(char* file_path);
void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame);
void nes_free(struct Nes* nes);

unsigned char* nes_get_addr_ptr(struct Nes* nes, unsigned short addr);

unsigned char nes_read_char(struct Nes* nes, unsigned short addr);
void nes_write_char(struct Nes* nes, unsigned short addr, unsigned char val);

unsigned short nes_read_short(struct Nes* nes, unsigned short addr);
void nes_write_short(struct Nes* nes, unsigned short addr, unsigned short val);

#endif //NES_H
