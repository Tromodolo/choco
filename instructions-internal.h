//
// Created by tromo on 5/25/25.
//

#ifndef INSTRUCTIONS_INTERNAL_H
#define INSTRUCTIONS_INTERNAL_H
#include <stdint.h>

// A bit silly but this is needed to be able to test in ./tests

enum AddressingMode {
    Addressing_Immediate,
    Addressing_ZeroPage,
    Addressing_ZeroPageX,
    Addressing_ZeroPageY,
    Addressing_Accumulator,
    Addressing_Absolute,
    Addressing_AbsoluteX,
    Addressing_AbsoluteY,
    Addressing_Indirect,
    Addressing_IndirectX,
    Addressing_IndirectY,
    Addressing_Relative,
    Addressing_NoneAddressing,
};

void set_zero_and_negative(struct Nes* nes, struct CPU* cpu, uint8_t value);
bool is_page_cross(uint16_t base, uint16_t addr);
uint8_t* get_address(struct Nes* nes, struct CPU* cpu, enum AddressingMode mode);

void brk(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void ora(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void kil(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void slo(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void asl(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void php(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void aac(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bpl(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void clc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void jsr(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void and(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void rla(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bit(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void rol(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void plp(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bmi(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sec(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void rti(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void eor(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sre(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void lsr(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void pha(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void asr(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void jmp(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bvc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void cli(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void rts(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void adc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void rra(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void ror(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void pla(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void arr(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bvs(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sei(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sta(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sax(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sty(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void stx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void dey(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void txa(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void xaa(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bcc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void axa(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void tya(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void txs(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void xas(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sya(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sxa(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void ldy(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void lda(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void ldx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void lax(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void tay(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void tax(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void atx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bcs(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void clv(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void tsx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void lar(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void cpy(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void cmp(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void dcp(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void dec(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void iny(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void dex(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void axs(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void bne(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void cld(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void cpx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sbc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void isb(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void inc(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void inx(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void nop(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void beq(struct Nes* nes, struct CPU* cpu, uint8_t* addr);
void sed(struct Nes* nes, struct CPU* cpu, uint8_t* addr);

#endif //INSTRUCTIONS_INTERNAL_H
