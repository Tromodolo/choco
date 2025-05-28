//
// Created by tromo on 5/25/25.
//

#include "nes-internal.h"
#include "instructions.h"
#include "instructions-internal.h"

#include "cartridge.h"
#include "nes.h"

#define INSTRUCTION(opcode, func, numbytes, cyclecount, addressing) \
case opcode: {\
    func(nes, cpu, get_address(nes, cpu, addressing)); \
    cpu->pc += numbytes - 1;\
    cpu->waiting_cycles += cyclecount;\
    break; \
} \

void set_zero_and_negative(struct Nes* nes, struct CPU* cpu, unsigned char value) {
    cpu->p.flags.zero = value == 0;
    cpu->p.flags.negative = (value & 0x80) == 0x80;
}

void set_lower_carry(struct Nes* nes, struct CPU* cpu, unsigned char value) {
    cpu->p.flags.carry = (value & 0x01) == 0x01;
}

void set_higher_carry(struct Nes* nes, struct CPU* cpu, unsigned char value) {
    cpu->p.flags.carry = (value & 0x80) == 0x80;
}

bool is_page_cross(unsigned short base, unsigned short addr) {
    return base & 0xFF00 != addr & 0xFF00;
}

inline unsigned char* get_address(struct Nes* nes, struct CPU* cpu, enum AddressingMode mode) {
    switch (mode) {
        case Addressing_NoneAddressing:
            return nullptr;
        case Addressing_Immediate:
            return nes_get_addr_ptr(nes, cpu->pc);
        case Addressing_Accumulator:
            return &cpu->acc;
        case Addressing_Relative:
        {
            const unsigned char jump = nes_read_char(nes, cpu->pc++);
            return nes_get_addr_ptr(nes, cpu->pc + jump);
        }
        case Addressing_ZeroPage:
            return nes_get_addr_ptr(nes, cpu->pc);
        case Addressing_ZeroPageX:
            return nes_get_addr_ptr(nes, cpu->pc + cpu->x);
        case Addressing_ZeroPageY:
            return nes_get_addr_ptr(nes, cpu->pc + cpu->y);
        case Addressing_Absolute:
            return nes_get_addr_ptr(nes, nes_read_short(nes, cpu->pc));
        case Addressing_AbsoluteX:
        {
            const unsigned short base = nes_read_short(nes, cpu->pc);
            const unsigned short addr = base + cpu->x;

            if (is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_AbsoluteY:
        {
            const unsigned short base = nes_read_short(nes, cpu->pc);
            const unsigned short addr = base + cpu->x;

            if (is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_Indirect:
        {
            const unsigned short addr = nes_read_short(nes, cpu->pc);
            // 6502 bug mode with page boundary:
            //  if address $3000 contains $40, $30FF contains $80, and $3100 contains $50,
            // the result of JMP ($30FF) will be a transfer of control to $4080 rather than $5080 as you intended
            // i.e. the 6502 took the low byte of the address from $30FF and the high byte from $3000
            if ((addr & 0xFF) == 0xFF) {
                const unsigned char lo = nes_read_char(nes, addr);
                const unsigned char hi = nes_read_char(nes, addr & 0xFF00);
                return nes_get_addr_ptr(nes, hi << 8 | lo);
            }

            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_IndirectX:
        {
            const unsigned char baseAddr = nes_read_char(nes, cpu->pc);
            const unsigned char addr = baseAddr + cpu->x;
            const unsigned char lo = nes_read_char(nes, addr);
            const unsigned char hi = nes_read_char(nes, addr + 1);
            return nes_get_addr_ptr(nes, hi << 8 | lo);
        }
        case Addressing_IndirectY:
        {
            const unsigned char baseAddr = nes_read_char(nes, cpu->pc);
            const unsigned char lo = nes_read_char(nes, baseAddr);
            const unsigned char hi = nes_read_char(nes, baseAddr + 1);
            const unsigned derefBase = (hi << 8 | lo);
            const unsigned short deref = derefBase + cpu->y;

            if (is_page_cross(deref, derefBase))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, deref);
        }
    }
}

inline void brk(struct Nes* nes, struct CPU* cpu, unsigned char* addr) {
    cpu->is_stopped = true;
}

inline void ora(struct Nes* nes, struct CPU* cpu, unsigned char* addr){
    cpu->acc |= *addr;
}

inline void kil(struct Nes* nes, struct CPU* cpu, unsigned char* addr){
    cpu->is_stopped = true;
}

inline void slo(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void asl(struct Nes* nes, struct CPU* cpu, unsigned char* addr){
    *addr <<= 1;

    set_higher_carry(nes, cpu, *addr);
    set_zero_and_negative(nes, cpu, *addr);
}

inline void php(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void aac(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bpl(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void clc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void jsr(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void and(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void rla(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bit(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void rol(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void plp(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bmi(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sec(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void rti(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void eor(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sre(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void lsr(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void pha(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void asr(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void jmp(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bvc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void cli(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void rts(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void adc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void rra(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void ror(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void pla(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void arr(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bvs(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sei(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sta(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sax(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sty(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void stx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void dey(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void txa(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void xaa(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bcc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void axa(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void tya(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void txs(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void xas(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sya(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sxa(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void ldy(struct Nes* nes, struct CPU* cpu, unsigned char* addr){
    cpu->y = *addr;
    set_zero_and_negative(nes,cpu, cpu->y);
}

inline void lda(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void ldx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void lax(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void tay(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void tax(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void atx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bcs(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void clv(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void tsx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void lar(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void cpy(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void cmp(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void dcp(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void dec(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void iny(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void dex(struct Nes* nes, struct CPU* cpu, unsigned char* addr){
    cpu->x--;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void axs(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void bne(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void cld(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void cpx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sbc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void isb(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void inc(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void inx(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void nop(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void beq(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void sed(struct Nes* nes, struct CPU* cpu, unsigned char* addr){

}

inline void nes_cpu_handle_instruction(struct Nes* nes, struct CPU* cpu, unsigned char opcode) {
    switch (opcode) {
        INSTRUCTION(0x00, brk, 1, 7, Addressing_NoneAddressing)
        INSTRUCTION(0x01, ora, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x02, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x03, slo, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0x04, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x05, ora, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x06, asl, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x07, slo, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0x08, php, 1, 3, Addressing_NoneAddressing)
        INSTRUCTION(0x09, ora, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x0a, asl, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x0b, aac, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x0c, nop, 3, 4,  Addressing_Absolute)
        INSTRUCTION(0x0d, ora, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x0e, asl, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x0f, slo, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0x10, bpl, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x11, ora, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x12, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x13, slo, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0x14, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x15, ora, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x16, asl, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x17, slo, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0x18, clc, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x19, ora, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x1A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x1b, slo, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0x1c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x1d, ora, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x1e, asl, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x1f, slo, 3, 7,  Addressing_AbsoluteX)
        INSTRUCTION(0x20, jsr, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x21, and, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x22, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x23, rla, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0x24, bit, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x25, and, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x26, rol, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x27, rla, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0x28, plp, 1, 4, Addressing_NoneAddressing)
        INSTRUCTION(0x29, and, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x2a, rol, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x2b, aac, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x2c, bit, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x2d, and, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x2e, rol, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x2f, rla, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0x30, bmi, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x31, and, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x32, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x33, rla, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0x34, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x35, and, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x36, rol, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x37, rla, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0x38, sec, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x39, and, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x3A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x3b, rla, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0x3c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x3d, and, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x3e, rol, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x3f, rla, 3, 7,  Addressing_AbsoluteX)
        INSTRUCTION(0x40, rti, 1, 6, Addressing_NoneAddressing)
        INSTRUCTION(0x41, eor, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x42, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x43, sre, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0x44, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x45, eor, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x46, lsr, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x47, sre, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0x48, pha, 1, 3, Addressing_NoneAddressing)
        INSTRUCTION(0x49, eor, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x4a, lsr, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x4b, asr, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x4c, jmp, 3, 3, Addressing_Absolute) //AddressingMode that acts as Immidiate
        INSTRUCTION(0x4d, eor, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x4e, lsr, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x4f, sre, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0x50, bvc, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x51, eor, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x52, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x53, sre, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0x54, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x55, eor, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x56, lsr, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x57, sre, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0x58, cli, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x59, eor, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x5A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x5b, sre, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0x5c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x5d, eor, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x5e, lsr, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x5f, sre, 3, 7,  Addressing_AbsoluteX)
        INSTRUCTION(0x60, rts, 1, 6, Addressing_NoneAddressing)
        INSTRUCTION(0x61, adc, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x62, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x63, rra, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0x64, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x65, adc, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x66, ror, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x67, rra, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0x68, pla, 1, 4, Addressing_NoneAddressing)
        INSTRUCTION(0x69, adc, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x6a, ror, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x6b, arr, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x6c, jmp, 3, 5, Addressing_Indirect)
        INSTRUCTION(0x6d, adc, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x6e, ror, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x6f, rra, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0x70, bvs, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x71, adc, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x72, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x73, rra, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0x74, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x75, adc, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x76, ror, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x77, rra, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0x78, sei, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x79, adc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x7A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x7b, rra, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0x7c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x7d, adc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x7e, ror, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x7f, rra, 3, 7,  Addressing_AbsoluteX)
        INSTRUCTION(0x80, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x81, sta, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x82, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x83, sax, 2, 6,  Addressing_IndirectX)
        INSTRUCTION(0x84, sty, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x85, sta, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x86, stx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x87, sax, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x88, dey, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x89, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x8a, txa, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x8b, xaa, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x8c, sty, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8d, sta, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8e, stx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8f, sax, 3, 4,  Addressing_Absolute)
        INSTRUCTION(0x90, bcc, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x91, sta, 2, 6, Addressing_IndirectY)
        INSTRUCTION(0x92, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x93, axa, 2, 6,  Addressing_IndirectY)
        INSTRUCTION(0x94, sty, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x95, sta, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x96, stx, 2, 4, Addressing_ZeroPageY)
        INSTRUCTION(0x97, sax, 2, 4,  Addressing_ZeroPageY)
        INSTRUCTION(0x98, tya, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x99, sta, 3, 5, Addressing_AbsoluteY)
        INSTRUCTION(0x9a, txs, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x9b, xas, 3, 5,  Addressing_AbsoluteY)
        INSTRUCTION(0x9c, sya, 3, 5,  Addressing_AbsoluteX)
        INSTRUCTION(0x9d, sta, 3, 5, Addressing_AbsoluteX)
        INSTRUCTION(0x9e, sxa, 3, 5,  Addressing_AbsoluteY)
        INSTRUCTION(0x9f, axa, 3, 5,  Addressing_AbsoluteY)
        INSTRUCTION(0xa0, ldy, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xa1, lda, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xa2, ldx, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xa3, lax, 2, 6,  Addressing_IndirectX)
        INSTRUCTION(0xa4, ldy, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa5, lda, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa6, ldx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa7, lax, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0xa8, tay, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xa9, lda, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xaa, tax, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xab, atx, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xac, ldy, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xad, lda, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xae, ldx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xaf, lax, 3, 4,  Addressing_Absolute)
        INSTRUCTION(0xb0, bcs, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xb1, lda, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xb2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xb3, lax, 2, 5,  Addressing_IndirectY)
        INSTRUCTION(0xb4, ldy, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xb5, lda, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xb6, ldx, 2, 4, Addressing_ZeroPageY)
        INSTRUCTION(0xb7, lax, 2, 4,  Addressing_ZeroPageY)
        INSTRUCTION(0xb8, clv, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xb9, lda, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xba, tsx, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xbb, lar, 3, 4,  Addressing_AbsoluteY)
        INSTRUCTION(0xbc, ldy, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xbd, lda, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xbe, ldx, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xbf, lax, 3, 4,  Addressing_AbsoluteY)
        INSTRUCTION(0xc0, cpy, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xc1, cmp, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xc2, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xc3, dcp, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0xc4, cpy, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xc5, cmp, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xc6, dec, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0xc7, dcp, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0xc8, iny, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xc9, cmp, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xca, dex, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xcb, axs, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xcc, cpy, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xcd, cmp, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xce, dec, 3, 6, Addressing_Absolute)
        INSTRUCTION(0xcf, dcp, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0xd0, bne, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xd1, cmp, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xd2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xd3, dcp, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0xd4, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0xd5, cmp, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xd6, dec, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0xd7, dcp, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0xD8, cld, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xd9, cmp, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xda, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0xdb, dcp, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0xdc, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0xdd, cmp, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xde, dec, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0xdf, dcp, 3, 7,  Addressing_AbsoluteX)
        INSTRUCTION(0xe0, cpx, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xe1, sbc, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xe2, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xe3, isb, 2, 8,  Addressing_IndirectX)
        INSTRUCTION(0xe4, cpx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xe5, sbc, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xe6, inc, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0xe7, isb, 2, 5,  Addressing_ZeroPage)
        INSTRUCTION(0xe8, inx, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xe9, sbc, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xea, nop, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xeb, sbc, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xec, cpx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xed, sbc, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xee, inc, 3, 6, Addressing_Absolute)
        INSTRUCTION(0xef, isb, 3, 6,  Addressing_Absolute)
        INSTRUCTION(0xf0, beq, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xf1, sbc, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xf2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xf3, isb, 2, 8,  Addressing_IndirectY)
        INSTRUCTION(0xf4, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0xf5, sbc, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xf6, inc, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0xf7, isb, 2, 6,  Addressing_ZeroPageX)
        INSTRUCTION(0xf8, sed, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xf9, sbc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xfa, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0xfb, isb, 3, 7,  Addressing_AbsoluteY)
        INSTRUCTION(0xfc, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0xfd, sbc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xfe, inc, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0xff, isb, 3, 7,  Addressing_AbsoluteX)
        default:
            // Just putting this here so we don't get c warning, and intentionally cause crash
            int* p = nullptr;
            *p = 0;
            break;
    }
}