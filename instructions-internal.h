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
uint8_t get_address(const struct Nes* nes, struct CPU* cpu, enum AddressingMode mode);
void write_address(const struct Nes* nes, struct CPU* cpu, const uint8_t val, const enum AddressingMode mode);

void brk(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void ora(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void kil(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void asl(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void php(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bpl(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void clc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void jsr(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void and(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bit(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void rol(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void plp(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bmi(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sec(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void rti(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void eor(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void lsr(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void pha(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void jmp(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bvc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void cli(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void rts(const struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void adc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void ror(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void pla(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bvs(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sei(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sta(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sty(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void stx(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void dey(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void txa(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bcc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void tya(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void txs(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void ldy(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void lda(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void ldx(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void tay(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void tax(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bcs(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void clv(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void tsx(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void cpy(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void cmp(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void dec(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void iny(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void dex(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void bne(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void cld(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void cpx(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sbc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void inc(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void inx(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void nop(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void beq(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);
void sed(struct Nes* nes, struct CPU* cpu, const enum AddressingMode addressing);

#endif //INSTRUCTIONS_INTERNAL_H
