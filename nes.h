//
// Created by tromo on 5/19/25.
//

#ifndef NES_H
#define NES_H
#include <raylib.h>
#include <stdint.h>

struct Nes* nes_init(char* file_path);
struct Nes* nes_init_from_buffer(const uint8_t* buffer, const long size);
void nes_get_samples(void* buffer_data, unsigned int frames, struct Nes* nes, Color* frame_buffer, bool* is_new_frame);
void nes_free(struct Nes* nes);

uint8_t* nes_get_addr_ptr(struct Nes* nes, uint16_t addr);

uint8_t nes_read_char(struct Nes* nes, uint16_t addr);
void nes_write_char(struct Nes* nes, uint16_t addr, uint8_t val);

uint16_t nes_read_short(struct Nes* nes, uint16_t addr);
void nes_write_short(struct Nes* nes, uint16_t addr, uint16_t val);

#endif //NES_H
