//
// Created by tromo on 5/25/25.
//

#ifndef INSTRUCTIONS_INTERNAL_H
#define INSTRUCTIONS_INTERNAL_H

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

bool is_page_cross(unsigned short base, unsigned short addr);
unsigned char* get_address(struct Nes* nes, struct CPU* cpu, enum AddressingMode mode);

void brk(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void ora(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void kil(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void slo(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void asl(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void php(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void aac(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bpl(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void clc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void jsr(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void and(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void rla(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bit(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void rol(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void plp(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bmi(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sec(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void rti(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void eor(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sre(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void lsr(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void pha(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void asr(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void jmp(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bvc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void cli(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void rts(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void adc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void rra(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void ror(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void pla(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void arr(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bvs(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sei(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sta(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sax(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sty(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void stx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void dey(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void txa(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void xaa(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bcc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void axa(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void tya(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void txs(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void xas(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sya(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sxa(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void ldy(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void lda(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void ldx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void lax(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void tay(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void tax(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void atx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bcs(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void clv(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void tsx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void lar(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void cpy(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void cmp(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void dcp(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void dec(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void iny(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void dex(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void axs(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void bne(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void cld(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void cpx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sbc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void isb(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void inc(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void inx(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void nop(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void beq(struct Nes* nes, struct CPU* cpu, unsigned char* addr);
void sed(struct Nes* nes, struct CPU* cpu, unsigned char* addr);

#endif //INSTRUCTIONS_INTERNAL_H
