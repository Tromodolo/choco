#ifndef INSTRUCTIONS_INTERNAL_H
#define INSTRUCTIONS_INTERNAL_H
#include <stdint.h>
#include "cpu.h"

// A bit silly but this is needed to be able to test in ./tests


void set_zero_and_negative(struct Nes* nes, struct CPU* cpu, uint8_t value);
bool is_page_cross(uint16_t base, uint16_t addr);

uint8_t get_address(const struct Nes* nes, struct CPU* cpu, enum AddressingMode mode, bool can_page_cross);
void write_address(const struct Nes* nes, struct CPU* cpu, const uint8_t val, const enum AddressingMode mode);

void fetch_addressed_value(struct Nes* nes, struct CPU* cpu);
void update_addressed_value(struct Nes* nes, struct CPU* cpu);

void brk(const struct Nes* nes, struct CPU* cpu);
void ora(struct Nes* nes, struct CPU* cpu);
void kil(struct Nes* nes, struct CPU* cpu);
void asl(struct Nes* nes, struct CPU* cpu);
void php(const struct Nes* nes, struct CPU* cpu);
void bpl(struct Nes* nes, struct CPU* cpu);
void clc(struct Nes* nes, struct CPU* cpu);
void jsr(const struct Nes* nes, struct CPU* cpu);
void and(struct Nes* nes, struct CPU* cpu);
void bit(struct Nes* nes, struct CPU* cpu);
void rol(struct Nes* nes, struct CPU* cpu);
void plp(const struct Nes* nes, struct CPU* cpu);
void bmi(struct Nes* nes, struct CPU* cpu);
void sec(struct Nes* nes, struct CPU* cpu);
void rti(const struct Nes* nes, struct CPU* cpu);
void eor(struct Nes* nes, struct CPU* cpu);
void lsr(struct Nes* nes, struct CPU* cpu);
void pha(const struct Nes* nes, struct CPU* cpu);
void jmp(const struct Nes* nes, struct CPU* cpu);
void bvc(struct Nes* nes, struct CPU* cpu);
void cli(struct Nes* nes, struct CPU* cpu);
void rts(const struct Nes* nes, struct CPU* cpu);
void adc(struct Nes* nes, struct CPU* cpu);
void ror(struct Nes* nes, struct CPU* cpu);
void pla(struct Nes* nes, struct CPU* cpu);
void bvs(struct Nes* nes, struct CPU* cpu);
void sei(struct Nes* nes, struct CPU* cpu);
void sta(struct Nes* nes, struct CPU* cpu);
void sty(struct Nes* nes, struct CPU* cpu);
void stx(struct Nes* nes, struct CPU* cpu);
void dey(struct Nes* nes, struct CPU* cpu);
void txa(struct Nes* nes, struct CPU* cpu);
void bcc(struct Nes* nes, struct CPU* cpu);
void tya(struct Nes* nes, struct CPU* cpu);
void txs(struct Nes* nes, struct CPU* cpu);
void ldy(struct Nes* nes, struct CPU* cpu);
void lda(struct Nes* nes, struct CPU* cpu);
void ldx(struct Nes* nes, struct CPU* cpu);
void tay(struct Nes* nes, struct CPU* cpu);
void tax(struct Nes* nes, struct CPU* cpu);
void bcs(struct Nes* nes, struct CPU* cpu);
void clv(struct Nes* nes, struct CPU* cpu);
void tsx(struct Nes* nes, struct CPU* cpu);
void cpy(struct Nes* nes, struct CPU* cpu);
void cmp(struct Nes* nes, struct CPU* cpu);
void dec(struct Nes* nes, struct CPU* cpu);
void iny(struct Nes* nes, struct CPU* cpu);
void dex(struct Nes* nes, struct CPU* cpu);
void bne(struct Nes* nes, struct CPU* cpu);
void cld(struct Nes* nes, struct CPU* cpu);
void cpx(struct Nes* nes, struct CPU* cpu);
void sbc(struct Nes* nes, struct CPU* cpu);
void inc(struct Nes* nes, struct CPU* cpu);
void inx(struct Nes* nes, struct CPU* cpu);
void nop(struct Nes* nes, struct CPU* cpu);
void beq(struct Nes* nes, struct CPU* cpu);
void sed(struct Nes* nes, struct CPU* cpu);

#endif //INSTRUCTIONS_INTERNAL_H
