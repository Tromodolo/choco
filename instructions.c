//
// Created by tromo on 5/25/25.
//

#include "instructions.h"
#include "instructions-internal.h"
#include "cpu.h"
#include "nes.h"

#define INSTRUCTION(opcode, func, numbytes, cyclecount, addressing) \
case opcode: {\
    cpu->current_instruction = opcode; \
    cpu->pc_pre = cpu->pc; \
    func(nes, cpu, get_address(nes, cpu, addressing)); \
    if (cpu->pc == cpu->pc_pre)\
        cpu->pc += numbytes - 1;\
    cpu->waiting_cycles += cyclecount;\
    break; \
} \

enum Interrupt {
    Interrupt_BRK,
    Interrupt_NMI,
    Interrupt_IRQ,
};

const int NMI_VECTOR = 0xFFFA;
const int BRK_VECTOR = 0xFFFE;
const int IRQ_VECTOR = 0xFFFE;

const int STACK_START = 0x0100;

void set_zero_and_negative(struct Nes* nes, struct CPU* cpu, const uint8_t value) {
    cpu->p.zero = value == 0;
    cpu->p.negative = (value & 0x80) == 0x80;
}

void set_lower_carry(struct Nes* nes, struct CPU* cpu, const uint8_t value) {
    cpu->p.carry = (value & 0x01) == 0x01;
}

void set_higher_carry(struct Nes* nes, struct CPU* cpu, const uint8_t value) {
    cpu->p.carry = (value & 0x80) == 0x80;
}

bool is_page_cross(const uint16_t base, const uint16_t addr) {
    return base & 0xFF00 != addr & 0xFF00;
}

void branch(struct Nes* nes, struct CPU* cpu, const bool condition, const uint8_t* addr) {
    if (!condition)
        return;

    cpu->waiting_cycles += 1;
    const int8_t offset = (int8_t)(*addr);
    const uint16_t jump_addr = cpu->pc + offset + 1;

    if (is_page_cross(cpu->pc + 1, jump_addr)) {
        cpu->waiting_cycles += 1;
    }
    cpu->pc = jump_addr;
}

inline uint8_t* get_address(struct Nes* nes, struct CPU* cpu, const enum AddressingMode mode) {
    switch (mode) {
        case Addressing_NoneAddressing:
            return nullptr;
        case Addressing_Immediate:
            return nes_get_addr_ptr(nes, cpu->pc);
        case Addressing_Accumulator:
            return &cpu->acc;
        case Addressing_Relative:
            return nes_get_addr_ptr(nes, cpu->pc);
        case Addressing_ZeroPage:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc);
            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_ZeroPageX:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->x;
            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_ZeroPageY:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->y;
            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_Absolute:
            return nes_get_addr_ptr(nes, nes_read_short(nes, cpu->pc));
        case Addressing_AbsoluteX:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->x;

            if (is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_AbsoluteY:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->y;

            if (is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, addr);
        }
        case Addressing_Indirect:
        {
            // NOTE: This is only needed for JMP, and because it needs to get a 16-bit address to jump to,
            // it's not posible to handle here, so i'm just returning null
            return nullptr;
        }
        case Addressing_IndirectX:
        {
            const uint8_t base_addr = nes_read_char(nes, cpu->pc);
            const uint8_t addr_lo = base_addr + cpu->x;
            const uint8_t addr_hi = addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, addr_lo);
            const uint8_t hi = nes_read_char(nes, addr_hi);
            return nes_get_addr_ptr(nes, hi << 8 | lo);
        }
        case Addressing_IndirectY:
        {
            const uint8_t base_addr_lo = nes_read_char(nes, cpu->pc);
            const uint8_t base_addr_hi = base_addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, base_addr_lo);
            const uint8_t hi = nes_read_char(nes, base_addr_hi);
            const unsigned deref_base = (hi << 8 | lo);
            const uint16_t deref = deref_base + cpu->y;

            if (is_page_cross(deref, deref_base))
                cpu->waiting_cycles++;

            return nes_get_addr_ptr(nes, deref);
        }
        default:
            return nullptr;
    }
}

uint8_t stack_pop(struct Nes* nes, struct CPU* cpu) {
    cpu->sp++;
    return nes_read_char(nes, STACK_START + cpu->sp);
}

void stack_push(struct Nes* nes, struct CPU* cpu, uint8_t val) {
    nes_write_char(nes, STACK_START + cpu->sp, val);
    cpu->sp--;
}

void handle_cpu_interrupt(struct Nes* nes, struct CPU* cpu, enum Interrupt type) {
    stack_push(nes, cpu, (cpu->pc & 0xFF00) >> 8);
    stack_push(nes, cpu, cpu->pc & 0xFF);

    Flags status = cpu->p;
    status.break1 = 1;
    status.break2 = 1;
    stack_push(nes, cpu, status.value);
    cpu->p.interrupt_disable = 1;

    switch (type) {
        case Interrupt_BRK:
            cpu->pc = nes_read_short(nes, BRK_VECTOR);
        return;
        case Interrupt_NMI:
            cpu->pc = nes_read_short(nes, NMI_VECTOR);
        return;
        case Interrupt_IRQ:
            cpu->pc = nes_read_short(nes, IRQ_VECTOR);
        return;
        default:
            return;
    }
}

inline void brk(struct Nes* nes, struct CPU* cpu, uint8_t* addr) {
    handle_cpu_interrupt(nes, cpu, Interrupt_BRK);
}

inline void ora(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc |= *addr;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void kil(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->is_stopped = true;
}

inline void asl(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    set_higher_carry(nes, cpu, *addr);
    *addr <<= 1;
    set_zero_and_negative(nes, cpu, *addr);
}

inline void php(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    Flags status = cpu->p;
    status.break1 = 1;
    status.break2 = 1;
    stack_push(nes, cpu, status.value);
}

inline void aac(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    and(nes, cpu, addr);
    set_higher_carry(nes, cpu, cpu->acc);
}

inline void bpl(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.negative == 0, addr);
}

inline void clc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.carry = 0;
}

inline void jsr(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    uint16_t new_pc = cpu->pc + 1;
    stack_push(nes, cpu, (new_pc & 0xFF00) >> 8);
    stack_push(nes, cpu, new_pc & 0xFF);
    cpu->pc = nes_read_short(nes, cpu->pc);
}

inline void and(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc &= *addr;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bit(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t result = cpu->acc & *addr;
    cpu->p.zero = result == 0;
    cpu->p.negative = (*addr & 0x80) >> 7;
    cpu->p.overflow = (*addr & 0x40) >> 6;
}

inline void rol(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t old_carry = cpu->p.carry;
    set_higher_carry(nes, cpu, *addr);
    *addr <<= 1;
    *addr += old_carry;
    set_zero_and_negative(nes, cpu, *addr);
}

inline void plp(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    // Masked away break1
    cpu->p.value = stack_pop(nes, cpu);
    cpu->p.break1 = 0;
    cpu->p.break2 = 1;
}

inline void bmi(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.negative == 1, addr);
}

inline void sec(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.carry = 1;
}

inline void rti(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    // Masked away break1
    cpu->p.value = stack_pop(nes, cpu);
    cpu->p.break1 = 0;
    cpu->p.break2 = 1;

    const uint8_t pc_lo = stack_pop(nes, cpu);
    const uint8_t pc_hi = stack_pop(nes, cpu);
    cpu->pc = pc_hi << 8 | pc_lo;
}

inline void eor(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc ^= *addr;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void lsr(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    set_lower_carry(nes, cpu, *addr);
    *addr >>= 1;
    set_zero_and_negative(nes, cpu, *addr);
}

inline void pha(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    stack_push(nes, cpu, cpu->acc);
}

inline void jmp(struct Nes* nes, struct CPU* cpu, uint8_t* _){
    // absolute addressing
    if (cpu->current_instruction == 0x4C) {
        cpu->pc = nes_read_short(nes, cpu->pc);
        return;
    }

    // otherwise indirect
    const uint16_t addr = nes_read_short(nes, cpu->pc);

    // 6502 bug mode with page boundary:
    //  if address $3000 contains $40, $30FF contains $80, and $3100 contains $50,
    // the result of JMP ($30FF) will be a transfer of control to $4080 rather than $5080 as you intended
    // i.e. the 6502 took the low byte of the address from $30FF and the high byte from $3000
    uint16_t jmp_addr = 0;
    if ((addr & 0xFF) == 0xFF) {
        const uint8_t lo = nes_read_char(nes, addr);
        const uint8_t hi = nes_read_char(nes, addr & 0xFF00);
        jmp_addr = hi << 8 | lo;
    } else {
        jmp_addr = nes_read_short(nes, addr);
    }

    cpu->pc = jmp_addr;
}

inline void bvc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.overflow == 0, addr);
}

inline void cli(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.interrupt_disable = 0;
}

inline void rts(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t pc_lo = stack_pop(nes, cpu);
    const uint8_t pc_hi = stack_pop(nes, cpu);
    const uint16_t new_pc = pc_hi << 8 | pc_lo;
    cpu->pc = new_pc + 1;
}

inline void adc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const int sum = cpu->acc + *addr + cpu->p.carry;

    // Whether or not the addition resulted in a signed overflow, 128 -> -127
    // Example
    // Acc = 0x7F
    // *addr = 2
    // (0x7F ^ 0x81) & (0x02 ^ 0x81) & 0x80;
    // (0xfe) & (0x83) & 0x80 == 0x80
    // Meaning if the sum matches either the sign of the acc or the value, it's not an overflow
    cpu->p.overflow = ((cpu->acc ^ sum) & (*addr ^ sum) & 0x80) >> 7;
    cpu->p.carry = sum > 0xFF;

    cpu->acc = sum;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void ror(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t old_carry = cpu->p.carry;
    set_lower_carry(nes, cpu, *addr);
    *addr >>= 1;
    *addr |= (old_carry << 7);
    set_zero_and_negative(nes, cpu, *addr);
}

inline void pla(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc = stack_pop(nes, cpu);
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bvs(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.overflow == 1, addr);
}

inline void sei(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.interrupt_disable = 1;
}

inline void sta(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    *addr = cpu->acc;
}

inline void sty(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    *addr = cpu->y;
}

inline void stx(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    *addr = cpu->x;
}

inline void dey(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->y--;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void txa(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc = cpu->x;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bcc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.carry == 0, addr);
}

inline void tya(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc = cpu->y;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void txs(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->sp = cpu->x;
}

inline void ldy(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->y = *addr;
    set_zero_and_negative(nes,cpu, cpu->y);
}

inline void lda(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->acc = *addr;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void ldx(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->x = *addr;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void tay(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->y = cpu->acc;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void tax(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->x = cpu->acc;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void bcs(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.carry == 1, addr);
}

inline void clv(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.overflow = 0;
}

inline void tsx(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->x = cpu->sp;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void cpy(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t result = cpu->y - *addr;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->y >= *addr;
}

inline void cmp(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t result = cpu->acc - *addr;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->acc >= *addr;
}

inline void dec(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    (*addr)--;
    set_zero_and_negative(nes, cpu, *addr);
}

inline void iny(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->y++;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void dex(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->x--;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void bne(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.zero == 0, addr);
}

inline void cld(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.decimal = 0;
}

inline void cpx(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    const uint8_t result = cpu->x - *addr;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->x >= *addr;
}

inline void sbc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    // a bit strange behavior here but this is how its implemented in hardware:
    // A + ~M + (C=1)
    // It can also be A - M - (!C) where !C is inverted carry but the hardware implementation is what i managed to get working
    const uint8_t added_value = 255 - *addr;
    const int sum = cpu->acc + added_value + cpu->p.carry;

    // Whether or not the subtraction resulted in a signed overflow, 128 -> -127
    // Example
    // Acc = 0x7F
    // *addr = 2
    // (0x7F ^ 0x81) & (0x02 ^ 0x81) & 0x80;
    // (0xfe) & (0x83) & 0x80 == 0x80
    // Meaning if the sum matches either the sign of the acc or the value, it's not an overflow
    cpu->p.overflow = ((cpu->acc ^ sum) & (added_value ^ sum) & 0x80) >> 7;
    cpu->p.carry = sum > 0xFF;

    cpu->acc = sum;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void inc(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    (*addr)++;
    set_zero_and_negative(nes, cpu, *addr);
}

inline void inx(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->x++;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void nop(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    // nothing here lol
}

inline void beq(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    branch(nes, cpu, cpu->p.zero == 1, addr);
}

inline void sed(struct Nes* nes, struct CPU* cpu, uint8_t* addr){
    cpu->p.decimal = 1;
}

inline void nes_cpu_handle_instruction(struct Nes* nes, struct CPU* cpu, uint8_t opcode) {
    switch (opcode) {
        INSTRUCTION(0x00, brk, 1, 7, Addressing_NoneAddressing)
        INSTRUCTION(0x01, ora, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x02, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x03, nop, 2, 8,  Addressing_IndirectX) // SLO
        INSTRUCTION(0x04, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x05, ora, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x06, asl, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x07, nop, 2, 5,  Addressing_ZeroPage) // SLO
        INSTRUCTION(0x08, php, 1, 3, Addressing_NoneAddressing)
        INSTRUCTION(0x09, ora, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x0a, asl, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x0b, nop, 2, 2,  Addressing_Immediate) // ANC
        INSTRUCTION(0x0c, nop, 3, 4,  Addressing_Absolute)
        INSTRUCTION(0x0d, ora, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x0e, asl, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x0f, nop, 3, 6,  Addressing_Absolute) // SLO
        INSTRUCTION(0x10, bpl, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x11, ora, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x12, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x13, nop, 2, 8,  Addressing_IndirectY) // SLO
        INSTRUCTION(0x14, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x15, ora, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x16, asl, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x17, nop, 2, 6,  Addressing_ZeroPageX) // SLO
        INSTRUCTION(0x18, clc, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x19, ora, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x1A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x1b, nop, 3, 7,  Addressing_AbsoluteY) // SLO
        INSTRUCTION(0x1c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x1d, ora, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x1e, asl, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x1f, nop, 3, 7,  Addressing_AbsoluteX) // SLO
        INSTRUCTION(0x20, jsr, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x21, and, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x22, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x23, nop, 2, 8,  Addressing_IndirectX) // RLA
        INSTRUCTION(0x24, bit, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x25, and, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x26, rol, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x27, nop, 2, 5,  Addressing_ZeroPage) // RLA
        INSTRUCTION(0x28, plp, 1, 4, Addressing_NoneAddressing)
        INSTRUCTION(0x29, and, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x2a, rol, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x2b, nop, 2, 2,  Addressing_Immediate) // ANC (ANC2)
        INSTRUCTION(0x2c, bit, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x2d, and, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x2e, rol, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x2f, nop, 3, 6,  Addressing_Absolute) // RLA
        INSTRUCTION(0x30, bmi, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x31, and, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x32, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x33, nop, 2, 8,  Addressing_IndirectY) // RLA
        INSTRUCTION(0x34, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x35, and, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x36, rol, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x37, nop, 2, 6,  Addressing_ZeroPageX) // RLA
        INSTRUCTION(0x38, sec, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x39, and, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x3A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x3b, nop, 3, 7,  Addressing_AbsoluteY) // RLA
        INSTRUCTION(0x3c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x3d, and, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x3e, rol, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x3f, nop, 3, 7,  Addressing_AbsoluteX) // RLA
        INSTRUCTION(0x40, rti, 1, 6, Addressing_NoneAddressing)
        INSTRUCTION(0x41, eor, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x42, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x43, nop, 2, 8,  Addressing_IndirectX) // SRE
        INSTRUCTION(0x44, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x45, eor, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x46, lsr, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x47, nop, 2, 5,  Addressing_ZeroPage) // SRE
        INSTRUCTION(0x48, pha, 1, 3, Addressing_NoneAddressing)
        INSTRUCTION(0x49, eor, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x4a, lsr, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x4b, nop, 2, 2,  Addressing_Immediate) // ALR (ASR)
        INSTRUCTION(0x4c, jmp, 3, 3, Addressing_Absolute) //AddressingMode that acts as Immidiate
        INSTRUCTION(0x4d, eor, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x4e, lsr, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x4f, nop, 3, 6,  Addressing_Absolute) // SRE
        INSTRUCTION(0x50, bvc, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x51, eor, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x52, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x53, nop, 2, 8,  Addressing_IndirectY) // SRE
        INSTRUCTION(0x54, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x55, eor, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x56, lsr, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x57, nop, 2, 6,  Addressing_ZeroPageX) // SRE
        INSTRUCTION(0x58, cli, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x59, eor, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x5A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x5b, nop, 3, 7,  Addressing_AbsoluteY) // SRE
        INSTRUCTION(0x5c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x5d, eor, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x5e, lsr, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x5f, nop, 3, 7,  Addressing_AbsoluteX) // SRE
        INSTRUCTION(0x60, rts, 1, 6, Addressing_NoneAddressing)
        INSTRUCTION(0x61, adc, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x62, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x63, nop, 2, 8,  Addressing_IndirectX) // RRA
        INSTRUCTION(0x64, nop, 2, 3,  Addressing_ZeroPage)
        INSTRUCTION(0x65, adc, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x66, ror, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0x67, nop, 2, 5,  Addressing_ZeroPage) // RRA
        INSTRUCTION(0x68, pla, 1, 4, Addressing_NoneAddressing)
        INSTRUCTION(0x69, adc, 2, 2, Addressing_Immediate)
        INSTRUCTION(0x6a, ror, 1, 2, Addressing_Accumulator)
        INSTRUCTION(0x6b, nop, 2, 2,  Addressing_Immediate) // ARR
        INSTRUCTION(0x6c, jmp, 3, 5, Addressing_Indirect)
        INSTRUCTION(0x6d, adc, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x6e, ror, 3, 6, Addressing_Absolute)
        INSTRUCTION(0x6f, nop, 3, 6,  Addressing_Absolute) // RRA
        INSTRUCTION(0x70, bvs, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x71, adc, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0x72, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x73, nop, 2, 8,  Addressing_IndirectY) // RRA
        INSTRUCTION(0x74, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0x75, adc, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x76, ror, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0x77, nop, 2, 6,  Addressing_ZeroPageX) // RRA
        INSTRUCTION(0x78, sei, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x79, adc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0x7A, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0x7b, nop, 3, 7,  Addressing_AbsoluteY) // RRA
        INSTRUCTION(0x7c, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0x7d, adc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0x7e, ror, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0x7f, nop, 3, 7,  Addressing_AbsoluteX) // RRA
        INSTRUCTION(0x80, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x81, sta, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0x82, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x83, nop, 2, 6,  Addressing_IndirectX) // SAX
        INSTRUCTION(0x84, sty, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x85, sta, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x86, stx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0x87, nop, 2, 3,  Addressing_ZeroPage) // SAX
        INSTRUCTION(0x88, dey, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x89, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0x8a, txa, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x8b, nop, 2, 2,  Addressing_Immediate) // XAA
        INSTRUCTION(0x8c, sty, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8d, sta, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8e, stx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0x8f, nop, 3, 4,  Addressing_Absolute) // SAX
        INSTRUCTION(0x90, bcc, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0x91, sta, 2, 6, Addressing_IndirectY)
        INSTRUCTION(0x92, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0x93, nop, 2, 6,  Addressing_IndirectY) // AXA
        INSTRUCTION(0x94, sty, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x95, sta, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0x96, stx, 2, 4, Addressing_ZeroPageY)
        INSTRUCTION(0x97, nop, 2, 4,  Addressing_ZeroPageY) // SAX
        INSTRUCTION(0x98, tya, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x99, sta, 3, 5, Addressing_AbsoluteY)
        INSTRUCTION(0x9a, txs, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0x9b, nop, 3, 5,  Addressing_AbsoluteY) // XAS
        INSTRUCTION(0x9c, nop, 3, 5,  Addressing_AbsoluteX) // SYA
        INSTRUCTION(0x9d, sta, 3, 5, Addressing_AbsoluteX)
        INSTRUCTION(0x9e, nop, 3, 5,  Addressing_AbsoluteY) // SXA
        INSTRUCTION(0x9f, nop, 3, 5,  Addressing_AbsoluteY) // AXA
        INSTRUCTION(0xa0, ldy, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xa1, lda, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xa2, ldx, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xa3, nop, 2, 6,  Addressing_IndirectX) // LAX
        INSTRUCTION(0xa4, ldy, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa5, lda, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa6, ldx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xa7, nop, 2, 3,  Addressing_ZeroPage) // LAX
        INSTRUCTION(0xa8, tay, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xa9, lda, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xaa, tax, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xab, nop, 2, 2,  Addressing_Immediate) // ATX
        INSTRUCTION(0xac, ldy, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xad, lda, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xae, ldx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xaf, nop, 3, 4,  Addressing_Absolute) // LAX
        INSTRUCTION(0xb0, bcs, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xb1, lda, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xb2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xb3, nop, 2, 5,  Addressing_IndirectY) // LAX
        INSTRUCTION(0xb4, ldy, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xb5, lda, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xb6, ldx, 2, 4, Addressing_ZeroPageY)
        INSTRUCTION(0xb7, nop, 2, 4,  Addressing_ZeroPageY) // LAX
        INSTRUCTION(0xb8, clv, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xb9, lda, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xba, tsx, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xbb, nop, 3, 4,  Addressing_AbsoluteY) // LAR
        INSTRUCTION(0xbc, ldy, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xbd, lda, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xbe, ldx, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xbf, nop, 3, 4,  Addressing_AbsoluteY) // LAX
        INSTRUCTION(0xc0, cpy, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xc1, cmp, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xc2, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xc3, nop, 2, 8,  Addressing_IndirectX) // DCP
        INSTRUCTION(0xc4, cpy, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xc5, cmp, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xc6, dec, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0xc7, nop, 2, 5,  Addressing_ZeroPage) // DCP
        INSTRUCTION(0xc8, iny, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xc9, cmp, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xca, dex, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xcb, nop, 2, 2,  Addressing_Immediate) // AXS
        INSTRUCTION(0xcc, cpy, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xcd, cmp, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xce, dec, 3, 6, Addressing_Absolute)
        INSTRUCTION(0xcf, nop, 3, 6,  Addressing_Absolute) // DCP
        INSTRUCTION(0xd0, bne, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xd1, cmp, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xd2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xd3, nop, 2, 8,  Addressing_IndirectY) // DCP
        INSTRUCTION(0xd4, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0xd5, cmp, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xd6, dec, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0xd7, nop, 2, 6,  Addressing_ZeroPageX) // DCP
        INSTRUCTION(0xD8, cld, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xd9, cmp, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xda, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0xdb, nop, 3, 7,  Addressing_AbsoluteY) // DCP
        INSTRUCTION(0xdc, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0xdd, cmp, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xde, dec, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0xdf, nop, 3, 7,  Addressing_AbsoluteX) // DCP
        INSTRUCTION(0xe0, cpx, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xe1, sbc, 2, 6, Addressing_IndirectX)
        INSTRUCTION(0xe2, nop, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xe3, nop, 2, 8,  Addressing_IndirectX) // ISB
        INSTRUCTION(0xe4, cpx, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xe5, sbc, 2, 3, Addressing_ZeroPage)
        INSTRUCTION(0xe6, inc, 2, 5, Addressing_ZeroPage)
        INSTRUCTION(0xe7, nop, 2, 5,  Addressing_ZeroPage) // ISB
        INSTRUCTION(0xe8, inx, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xe9, sbc, 2, 2, Addressing_Immediate)
        INSTRUCTION(0xea, nop, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xeb, sbc, 2, 2,  Addressing_Immediate)
        INSTRUCTION(0xec, cpx, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xed, sbc, 3, 4, Addressing_Absolute)
        INSTRUCTION(0xee, inc, 3, 6, Addressing_Absolute)
        INSTRUCTION(0xef, nop, 3, 6,  Addressing_Absolute) // ISB
        INSTRUCTION(0xf0, beq, 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, Addressing_Relative)
        INSTRUCTION(0xf1, sbc, 2, 5 /*+1 if page crossed*/, Addressing_IndirectY)
        INSTRUCTION(0xf2, kil, 1, 0,  Addressing_NoneAddressing)
        INSTRUCTION(0xf3, nop, 2, 8,  Addressing_IndirectY) // ISB
        INSTRUCTION(0xf4, nop, 2, 4,  Addressing_ZeroPageX)
        INSTRUCTION(0xf5, sbc, 2, 4, Addressing_ZeroPageX)
        INSTRUCTION(0xf6, inc, 2, 6, Addressing_ZeroPageX)
        INSTRUCTION(0xf7, nop, 2, 6,  Addressing_ZeroPageX) // ISB
        INSTRUCTION(0xf8, sed, 1, 2, Addressing_NoneAddressing)
        INSTRUCTION(0xf9, sbc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteY)
        INSTRUCTION(0xfa, nop, 1, 2,  Addressing_NoneAddressing)
        INSTRUCTION(0xfb, nop, 3, 7,  Addressing_AbsoluteY) // ISB
        INSTRUCTION(0xfc, nop, 3, 4,  Addressing_AbsoluteX)
        INSTRUCTION(0xfd, sbc, 3, 4 /*+1 if page crossed*/, Addressing_AbsoluteX)
        INSTRUCTION(0xfe, inc, 3, 7, Addressing_AbsoluteX)
        INSTRUCTION(0xff, nop, 3, 7,  Addressing_AbsoluteX) // ISB
        default:
            // Just putting this here so we don't get c warning, and intentionally cause crash
            int* p = nullptr;
            *p = 0;
            break;
    }
}