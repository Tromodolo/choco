#include "instructions.h"
#include "instructions-internal.h"
#include "cpu.h"
#include "nes.h"

#define INSTRUCTION(opcode, func, numbytes, cyclecount, canpagecross, addressing) \
case opcode: {\
    cpu->current_instruction = opcode; \
    cpu->current_addressing_mode = addressing; \
    cpu->can_page_cross = canpagecross; \
    cpu->pc_pre = cpu->pc; \
    cpu->did_branch = false; \
    \
    func(nes, cpu); \
    \
    if (cpu->pc == cpu->pc_pre && !cpu->did_branch)\
        cpu->pc += numbytes - 1;\
    \
    cpu->waiting_cycles += cyclecount;\
    break; \
} \

enum Interrupt {
    Interrupt_BRK,
    Interrupt_NMI,
    Interrupt_IRQ,
};

constexpr int NMI_VECTOR = 0xFFFA;
constexpr int BRK_VECTOR = 0xFFFE;
constexpr int IRQ_VECTOR = 0xFFFE;

constexpr int STACK_START = 0x0100;

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
    return (base & 0xFF00) != (addr & 0xFF00);
}

void branch(struct Nes* nes, struct CPU* cpu, const bool condition, const uint8_t value) {
    if (!condition)
        return;

    cpu->did_branch = true;

    cpu->waiting_cycles += 1;
    const int8_t offset = (int8_t)(value);
    const uint16_t jump_addr = cpu->pc + offset + 1;

    if (is_page_cross(cpu->pc + 1, jump_addr)) {
        cpu->waiting_cycles += 1;
    }
    cpu->pc = jump_addr;
}

inline void fetch_addressed_value(struct Nes* nes, struct CPU* cpu) {
    cpu->read_tmp = get_address(nes, cpu, cpu->current_addressing_mode, cpu->can_page_cross);
}

inline void update_addressed_value(struct Nes* nes, struct CPU* cpu) {
    write_address(nes, cpu, cpu->read_tmp, cpu->current_addressing_mode);
}

// I don't care how stupid this and the write_address solution is with read_tmp, so don't @ me
inline uint8_t get_address(struct Nes* nes, struct CPU* cpu, const enum AddressingMode mode, bool can_page_cross) {
    switch (mode) {
        case Addressing_NoneAddressing:
            return 0;
        case Addressing_Immediate:
            return nes_read_char(nes, cpu->pc);
        case Addressing_Accumulator:
            return cpu->acc;
        case Addressing_Relative:
            return nes_read_char(nes, cpu->pc);
        case Addressing_ZeroPage:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc);
            return nes_read_char(nes, addr);
        }
        case Addressing_ZeroPageX:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->x;
            return nes_read_char(nes, addr);
        }
        case Addressing_ZeroPageY:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->y;
            return nes_read_char(nes, addr);
        }
        case Addressing_Absolute:
            return nes_read_char(nes, nes_read_short(nes, cpu->pc));
        case Addressing_AbsoluteX:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->x;

            if (can_page_cross && is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_read_char(nes, addr);
        }
        case Addressing_AbsoluteY:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->y;

            if (can_page_cross && is_page_cross(base, addr))
                cpu->waiting_cycles++;

            return nes_read_char(nes, addr);
        }
        case Addressing_Indirect:
        {
            // NOTE: This is only needed for JMP, and because it needs to get a 16-bit address to jump to,
            // it's not posible to handle here, so i'm just returning null
            return 0;
        }
        case Addressing_IndirectX:
        {
            const uint8_t base_addr = nes_read_char(nes, cpu->pc);
            const uint8_t addr_lo = base_addr + cpu->x;
            const uint8_t addr_hi = addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, addr_lo);
            const uint8_t hi = nes_read_char(nes, addr_hi);
            return nes_read_char(nes, hi << 8 | lo);
        }
        case Addressing_IndirectY:
        {
            const uint8_t base_addr_lo = nes_read_char(nes, cpu->pc);
            const uint8_t base_addr_hi = base_addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, base_addr_lo);
            const uint8_t hi = nes_read_char(nes, base_addr_hi);
            const unsigned deref_base = (hi << 8 | lo);
            const uint16_t deref = deref_base + cpu->y;

            if (can_page_cross && is_page_cross(deref, deref_base))
                cpu->waiting_cycles++;

            return nes_read_char(nes, deref);
        }
        default:
            return 0;
    }
}

inline void write_address(struct Nes* nes, struct CPU* cpu, const uint8_t val, const enum AddressingMode mode) {
    switch (mode) {
        case Addressing_NoneAddressing:
            return;
        case Addressing_Immediate:
            nes_write_char(nes, cpu->pc, val);
            return;
        case Addressing_Accumulator:
            cpu->acc = val;
            return;
        case Addressing_Relative:
            nes_write_char(nes, cpu->pc, val);
            return;
        case Addressing_ZeroPage:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc);
            nes_write_char(nes, addr, val);
            return;
        }
        case Addressing_ZeroPageX:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->x;
            nes_write_char(nes, addr, val);
            return;
        }
        case Addressing_ZeroPageY:
        {
            const uint8_t addr = nes_read_char(nes, cpu->pc) + cpu->y;
            nes_write_char(nes, addr, val);
            return;
        }
        case Addressing_Absolute:
            nes_write_char(nes, nes_read_short(nes, cpu->pc), val);
            return;
        case Addressing_AbsoluteX:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->x;
            nes_write_char(nes, addr, val);
            return;
        }
        case Addressing_AbsoluteY:
        {
            const uint16_t base = nes_read_short(nes, cpu->pc);
            const uint16_t addr = base + cpu->y;
            nes_write_char(nes, addr, val);
            return;
        }
        case Addressing_Indirect:
        {
            // NOTE: This is only needed for JMP, and because it needs to get a 16-bit address to jump to,
            // it's not posible to handle here, so i'm just returning null
            return;
        }
        case Addressing_IndirectX:
        {
            const uint8_t base_addr = nes_read_char(nes, cpu->pc);
            const uint8_t addr_lo = base_addr + cpu->x;
            const uint8_t addr_hi = addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, addr_lo);
            const uint8_t hi = nes_read_char(nes, addr_hi);
            nes_write_char(nes, hi << 8 | lo, val);
            return;
        }
        case Addressing_IndirectY:
        {
            const uint8_t base_addr_lo = nes_read_char(nes, cpu->pc);
            const uint8_t base_addr_hi = base_addr_lo + 1;
            const uint8_t lo = nes_read_char(nes, base_addr_lo);
            const uint8_t hi = nes_read_char(nes, base_addr_hi);
            const unsigned deref_base = (hi << 8 | lo);
            const uint16_t deref = deref_base + cpu->y;
            nes_write_char(nes, deref, val);
            return;
        }
        default:
            return;
    }
}

uint8_t stack_pop(struct Nes* nes, struct CPU* cpu) {
    cpu->sp++;
    return nes_read_char(nes, STACK_START + cpu->sp);
}

void stack_push(struct Nes* nes, struct CPU* cpu, const uint8_t val) {
    nes_write_char(nes, STACK_START + cpu->sp, val);
    cpu->sp--;
}

void handle_cpu_interrupt(struct Nes* nes, struct CPU* cpu, const enum Interrupt type) {
    const uint16_t new_pc = cpu->pc + 1;
    stack_push(nes, cpu, (new_pc & 0xFF00) >> 8);
    stack_push(nes, cpu, new_pc & 0xFF);

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

inline void brk(struct Nes* nes, struct CPU* cpu) {
    handle_cpu_interrupt(nes, cpu, Interrupt_BRK);
}

inline void ora(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc |= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void kil(struct Nes* nes, struct CPU* cpu){
    cpu->pc = cpu->pc_pre - 1;
    cpu->is_stopped = true;
}

inline void asl(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    set_higher_carry(nes, cpu, cpu->read_tmp);
    cpu->read_tmp <<= 1;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void php(struct Nes* nes, struct CPU* cpu){
    Flags status = cpu->p;
    status.break1 = 1;
    status.break2 = 1;
    stack_push(nes, cpu, status.value);
}

inline void bpl(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.negative == 0, cpu->read_tmp);
}

inline void clc(struct Nes* nes, struct CPU* cpu){
    cpu->p.carry = 0;
}

inline void jsr(struct Nes* nes, struct CPU* cpu){
    const uint16_t new_pc = cpu->pc + 1;
    stack_push(nes, cpu, (new_pc & 0xFF00) >> 8);
    stack_push(nes, cpu, new_pc & 0xFF);
    cpu->pc = nes_read_short(nes, cpu->pc);
}

inline void and(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bit(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t result = cpu->acc & cpu->read_tmp;
    cpu->p.zero = result == 0;
    cpu->p.negative = (cpu->read_tmp & 0x80) >> 7;
    cpu->p.overflow = (cpu->read_tmp & 0x40) >> 6;
}

inline void rol(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t old_carry = cpu->p.carry;
    set_higher_carry(nes, cpu, cpu->read_tmp);
    cpu->read_tmp <<= 1;
    cpu->read_tmp += old_carry;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void plp(struct Nes* nes, struct CPU* cpu){
    // Masked away break1
    cpu->p.value = stack_pop(nes, cpu);
    cpu->p.break1 = 0;
    cpu->p.break2 = 1;
}

inline void bmi(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.negative == 1, cpu->read_tmp);
}

inline void sec(struct Nes* nes, struct CPU* cpu){
    cpu->p.carry = 1;
}

inline void rti(struct Nes* nes, struct CPU* cpu){
    // Masked away break1
    cpu->p.value = stack_pop(nes, cpu);
    cpu->p.break1 = 0;
    cpu->p.break2 = 1;

    const uint8_t pc_lo = stack_pop(nes, cpu);
    const uint8_t pc_hi = stack_pop(nes, cpu);
    cpu->pc = pc_hi << 8 | pc_lo;
}

inline void eor(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc ^= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void lsr(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    set_lower_carry(nes, cpu, cpu->read_tmp);
    cpu->read_tmp >>= 1;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void pha(struct Nes* nes, struct CPU* cpu){
    stack_push(nes, cpu, cpu->acc);
}

inline void jmp(struct Nes* nes, struct CPU* cpu){
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

inline void bvc(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.overflow == 0, cpu->read_tmp);
}

inline void cli(struct Nes* nes, struct CPU* cpu){
    cpu->p.interrupt_disable = 0;
}

inline void rts(struct Nes* nes, struct CPU* cpu){
    const uint8_t pc_lo = stack_pop(nes, cpu);
    const uint8_t pc_hi = stack_pop(nes, cpu);
    const uint16_t new_pc = pc_hi << 8 | pc_lo;
    cpu->pc = new_pc + 1;
}

inline void adc(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const int sum = cpu->acc + cpu->read_tmp + cpu->p.carry;

    // Whether or not the addition resulted in a signed overflow, 128 -> -127
    // Example
    // Acc = 0x7F
    // cpu->read_tmp = 2
    // (0x7F ^ 0x81) & (0x02 ^ 0x81) & 0x80;
    // (0xfe) & (0x83) & 0x80 == 0x80
    // Meaning if the sum matches either the sign of the acc or the value, it's not an overflow
    cpu->p.overflow = ((cpu->acc ^ sum) & (cpu->read_tmp ^ sum) & 0x80) >> 7;
    cpu->p.carry = sum > 0xFF;

    cpu->acc = sum;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void ror(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t old_carry = cpu->p.carry;
    set_lower_carry(nes, cpu, cpu->read_tmp);
    cpu->read_tmp >>= 1;
    cpu->read_tmp |= (old_carry << 7);
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void pla(struct Nes* nes, struct CPU* cpu){
    cpu->acc = stack_pop(nes, cpu);
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bvs(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.overflow == 1, cpu->read_tmp);
}

inline void sei(struct Nes* nes, struct CPU* cpu){
    cpu->p.interrupt_disable = 1;
}

inline void sta(struct Nes* nes, struct CPU* cpu){
    cpu->read_tmp = cpu->acc;

    update_addressed_value(nes, cpu);
}

inline void sty(struct Nes* nes, struct CPU* cpu){
    cpu->read_tmp = cpu->y;

    update_addressed_value(nes, cpu);
}

inline void stx(struct Nes* nes, struct CPU* cpu){
    cpu->read_tmp = cpu->x;

    update_addressed_value(nes, cpu);
}

inline void dey(struct Nes* nes, struct CPU* cpu){
    cpu->y--;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void txa(struct Nes* nes, struct CPU* cpu){
    cpu->acc = cpu->x;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void bcc(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.carry == 0, cpu->read_tmp);
}

inline void tya(struct Nes* nes, struct CPU* cpu){
    cpu->acc = cpu->y;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void txs(struct Nes* nes, struct CPU* cpu){
    cpu->sp = cpu->x;
}

inline void ldy(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->y = cpu->read_tmp;
    set_zero_and_negative(nes,cpu, cpu->y);
}

inline void lda(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc = cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void ldx(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->x = cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void tay(struct Nes* nes, struct CPU* cpu){
    cpu->y = cpu->acc;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void tax(struct Nes* nes, struct CPU* cpu){
    cpu->x = cpu->acc;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void bcs(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.carry == 1, cpu->read_tmp);
}

inline void clv(struct Nes* nes, struct CPU* cpu){
    cpu->p.overflow = 0;
}

inline void tsx(struct Nes* nes, struct CPU* cpu){
    cpu->x = cpu->sp;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void cpy(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t result = cpu->y - cpu->read_tmp;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->y >= cpu->read_tmp;
}

inline void cmp(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t result = cpu->acc - cpu->read_tmp;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->acc >= cpu->read_tmp;
}

inline void dec(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp--;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void iny(struct Nes* nes, struct CPU* cpu){
    cpu->y++;
    set_zero_and_negative(nes, cpu, cpu->y);
}

inline void dex(struct Nes* nes, struct CPU* cpu){
    cpu->x--;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void bne(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.zero == 0, cpu->read_tmp);
}

inline void cld(struct Nes* nes, struct CPU* cpu){
    cpu->p.decimal = 0;
}

inline void cpx(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    const uint8_t result = cpu->x - cpu->read_tmp;
    set_zero_and_negative(nes, cpu, result);
    cpu->p.carry = cpu->x >= cpu->read_tmp;
}

inline void sbc(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    // a bit strange behavior here but this is how its implemented in hardware:
    // A + ~M + (C=1)
    // It can also be A - M - (!C) where !C is inverted carry but the hardware implementation is what i managed to get working
    const uint8_t added_value = 255 - cpu->read_tmp;
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

inline void inc(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp++;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);

    update_addressed_value(nes, cpu);
}

inline void inx(struct Nes* nes, struct CPU* cpu){
    cpu->x++;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void nop(struct Nes* nes, struct CPU* cpu){
    // nothing here lol
}

inline void beq(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    branch(nes, cpu, cpu->p.zero == 1, cpu->read_tmp);
}

inline void sed(struct Nes* nes, struct CPU* cpu){
    cpu->p.decimal = 1;
}

// illegal
inline void slo(struct Nes* nes, struct CPU* cpu) {
    asl(nes, cpu);

    cpu->acc |= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void anc(struct Nes* nes, struct CPU* cpu) {
    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
    set_higher_carry(nes, cpu, cpu->acc);
}

inline void rla(struct Nes* nes, struct CPU* cpu) {
    rol(nes, cpu);

    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void sre(struct Nes* nes, struct CPU* cpu) {
    lsr(nes, cpu);

    fetch_addressed_value(nes, cpu);

    cpu->acc ^= cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void asr(struct Nes* nes, struct CPU* cpu) {
    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;
    cpu->current_addressing_mode = Addressing_Accumulator;
    ror(nes, cpu);
}

inline void rra(struct Nes* nes, struct CPU* cpu) {
    ror(nes, cpu);

    fetch_addressed_value(nes, cpu);

    const int sum = cpu->acc + cpu->read_tmp + cpu->p.carry;

    // Whether or not the addition resulted in a signed overflow, 128 -> -127
    // Example
    // Acc = 0x7F
    // cpu->read_tmp = 2
    // (0x7F ^ 0x81) & (0x02 ^ 0x81) & 0x80;
    // (0xfe) & (0x83) & 0x80 == 0x80
    // Meaning if the sum matches either the sign of the acc or the value, it's not an overflow
    cpu->p.overflow = ((cpu->acc ^ sum) & (cpu->read_tmp ^ sum) & 0x80) >> 7;
    cpu->p.carry = sum > 0xFF;

    cpu->acc = sum;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void arr(struct Nes* nes, struct CPU* cpu) {
    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;

    cpu->current_addressing_mode = Addressing_Accumulator;
    ror(nes, cpu);

    set_zero_and_negative(nes, cpu, cpu->acc);

    const bool six = (cpu->acc & 0b01000000) > 0;
    const bool five = (cpu->acc & 0b00100000) > 0;

    if ((six && five) || (six && !five)) {
        cpu->p.carry = 1;
        cpu->p.overflow = 0;
    } else if ((!six && !five) || (!six && five)) {
        cpu->p.overflow = 1;
        cpu->p.carry = 0;
    }
}

inline void xaa(struct Nes* nes, struct CPU* cpu){
    // nothing lol
}

inline void sax(struct Nes* nes, struct CPU* cpu){
    cpu->read_tmp = cpu->x & cpu->acc;

    update_addressed_value(nes, cpu);
}

inline void sya(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp = cpu->y & (cpu->read_tmp & 0xF0) + 1;

    update_addressed_value(nes, cpu);
}

inline void sxa(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp = cpu->x & (cpu->read_tmp & 0xF0) + 1;

    update_addressed_value(nes, cpu);
}

inline void axa(struct Nes* nes, struct CPU* cpu){
    cpu->read_tmp = cpu->x & cpu->acc & 0b111;

    update_addressed_value(nes, cpu);
}

inline void atx(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc &= cpu->read_tmp;
    cpu->x = cpu->acc;
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void dcp(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp--;

    update_addressed_value(nes, cpu);

    cpu->p.carry = cpu->read_tmp <= cpu->acc;

    const uint8_t result = cpu->acc - cpu->read_tmp;
    set_zero_and_negative(nes, cpu, result);
}

inline void isb(struct Nes* nes, struct CPU* cpu){
    inc(nes, cpu);

    fetch_addressed_value(nes, cpu);
    cpu->read_tmp  = 255 - cpu->read_tmp;

    const int sum = cpu->acc + cpu->read_tmp + cpu->p.carry;

    // Whether or not the addition resulted in a signed overflow, 128 -> -127
    // Example
    // Acc = 0x7F
    // cpu->read_tmp = 2
    // (0x7F ^ 0x81) & (0x02 ^ 0x81) & 0x80;
    // (0xfe) & (0x83) & 0x80 == 0x80
    // Meaning if the sum matches either the sign of the acc or the value, it's not an overflow
    cpu->p.overflow = ((cpu->acc ^ sum) & (cpu->read_tmp ^ sum) & 0x80) >> 7;
    cpu->p.carry = sum > 0xFF;

    cpu->acc = sum;
    set_zero_and_negative(nes, cpu, cpu->acc);
}

inline void lax(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->acc = cpu->read_tmp;
    cpu->x = cpu->read_tmp;
    set_zero_and_negative(nes, cpu, cpu->read_tmp);
}

inline void lar(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->read_tmp &= cpu->sp;
    cpu->acc = cpu->read_tmp;
    cpu->x = cpu->read_tmp;
    cpu->sp = cpu->read_tmp;

    set_zero_and_negative(nes, cpu, cpu->read_tmp);
}

inline void xas(struct Nes* nes, struct CPU* cpu){
    // unstable, skipping
}

inline void axs(struct Nes* nes, struct CPU* cpu){
    fetch_addressed_value(nes, cpu);

    cpu->x &= cpu->acc;
    cpu->x - cpu->read_tmp;

    set_higher_carry(nes, cpu, cpu->read_tmp);
    set_zero_and_negative(nes, cpu, cpu->x);
}

inline void nes_cpu_handle_instruction(struct Nes* nes, struct CPU* cpu, const uint8_t opcode) {
    if (nes_is_nmi(nes)) {
        handle_cpu_interrupt(nes, cpu, Interrupt_NMI);
        return;
    }

    switch (opcode) {
        INSTRUCTION(0x00, brk, 1, 7, false, Addressing_NoneAddressing)
        INSTRUCTION(0x01, ora, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0x02, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x03, slo, 2, 8, false, Addressing_IndirectX) // SLO
        INSTRUCTION(0x04, nop, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x05, ora, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x06, asl, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0x07, slo, 2, 5, false, Addressing_ZeroPage) // SLO
        INSTRUCTION(0x08, php, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x09, ora, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x0a, asl, 1, 2, false, Addressing_Accumulator)
        INSTRUCTION(0x0b, anc, 2, 2, false, Addressing_Immediate) // ANC
        INSTRUCTION(0x0c, nop, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x0d, ora, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x0e, asl, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0x0f, slo, 3, 6, false, Addressing_Absolute) // SLO
        INSTRUCTION(0x10, bpl, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0x11, ora, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0x12, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x13, slo, 2, 8, false, Addressing_IndirectY) // SLO
        INSTRUCTION(0x14, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x15, ora, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x16, asl, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0x17, slo, 2, 6, false, Addressing_ZeroPageX) // SLO
        INSTRUCTION(0x18, clc, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x19, ora, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0x1A, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x1b, slo, 3, 7, false, Addressing_AbsoluteY) // SLO
        INSTRUCTION(0x1c, nop, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x1d, ora, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x1e, asl, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0x1f, slo, 3, 7, false, Addressing_AbsoluteX) // SLO
        INSTRUCTION(0x20, jsr, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0x21, and, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0x22, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x23, rla, 2, 8, false, Addressing_IndirectX) // RLA
        INSTRUCTION(0x24, bit, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x25, and, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x26, rol, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0x27, rla, 2, 5, false, Addressing_ZeroPage) // RLA
        INSTRUCTION(0x28, plp, 1, 4, false, Addressing_NoneAddressing)
        INSTRUCTION(0x29, and, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x2a, rol, 1, 2, false, Addressing_Accumulator)
        INSTRUCTION(0x2b, anc, 2, 2, false, Addressing_Immediate) // ANC (ANC2)
        INSTRUCTION(0x2c, bit, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x2d, and, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x2e, rol, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0x2f, rla, 3, 6, false, Addressing_Absolute) // RLA
        INSTRUCTION(0x30, bmi, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0x31, and, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0x32, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x33, rla, 2, 8, false, Addressing_IndirectY) // RLA
        INSTRUCTION(0x34, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x35, and, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x36, rol, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0x37, rla, 2, 6, false, Addressing_ZeroPageX) // RLA
        INSTRUCTION(0x38, sec, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x39, and, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0x3A, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x3b, rla, 3, 7, false, Addressing_AbsoluteY) // RLA
        INSTRUCTION(0x3c, nop, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x3d, and, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x3e, rol, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0x3f, rla, 3, 7, false, Addressing_AbsoluteX) // RLA
        INSTRUCTION(0x40, rti, 1, 6, false, Addressing_NoneAddressing)
        INSTRUCTION(0x41, eor, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0x42, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x43, sre, 2, 8, false, Addressing_IndirectX) // SRE
        INSTRUCTION(0x44, nop, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x45, eor, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x46, lsr, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0x47, sre, 2, 5, false, Addressing_ZeroPage) // SRE
        INSTRUCTION(0x48, pha, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x49, eor, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x4a, lsr, 1, 2, false, Addressing_Accumulator)
        INSTRUCTION(0x4b, asr, 2, 2, false, Addressing_Immediate) // ALR (ASR)
        INSTRUCTION(0x4c, jmp, 3, 3, false, Addressing_Absolute) //AddressingMode that acts as Immidiate
        INSTRUCTION(0x4d, eor, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x4e, lsr, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0x4f, sre, 3, 6, false, Addressing_Absolute) // SRE
        INSTRUCTION(0x50, bvc, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0x51, eor, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0x52, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x53, sre, 2, 8, false, Addressing_IndirectY) // SRE
        INSTRUCTION(0x54, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x55, eor, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x56, lsr, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0x57, sre, 2, 6, false, Addressing_ZeroPageX) // SRE
        INSTRUCTION(0x58, cli, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x59, eor, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0x5A, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x5b, sre, 3, 7, false, Addressing_AbsoluteY) // SRE
        INSTRUCTION(0x5c, nop, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x5d, eor, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x5e, lsr, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0x5f, sre, 3, 7, false, Addressing_AbsoluteX) // SRE
        INSTRUCTION(0x60, rts, 1, 6, false, Addressing_NoneAddressing)
        INSTRUCTION(0x61, adc, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0x62, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x63, rra, 2, 8, false, Addressing_IndirectX) // RRA
        INSTRUCTION(0x64, nop, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x65, adc, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x66, ror, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0x67, rra, 2, 5, false, Addressing_ZeroPage) // RRA
        INSTRUCTION(0x68, pla, 1, 4, false, Addressing_NoneAddressing)
        INSTRUCTION(0x69, adc, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x6a, ror, 1, 2, false, Addressing_Accumulator)
        INSTRUCTION(0x6b, arr, 2, 2, false, Addressing_Immediate) // ARR
        INSTRUCTION(0x6c, jmp, 3, 5, false, Addressing_Indirect)
        INSTRUCTION(0x6d, adc, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x6e, ror, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0x6f, rra, 3, 6, false, Addressing_Absolute) // RRA
        INSTRUCTION(0x70, bvs, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0x71, adc, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0x72, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x73, rra, 2, 8, false, Addressing_IndirectY) // RRA
        INSTRUCTION(0x74, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x75, adc, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x76, ror, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0x77, rra, 2, 6, false, Addressing_ZeroPageX) // RRA
        INSTRUCTION(0x78, sei, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x79, adc, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0x7A, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x7b, rra, 3, 7, false, Addressing_AbsoluteY) // RRA
        INSTRUCTION(0x7c, nop, 3, 4, true,/*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x7d, adc, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0x7e, ror, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0x7f, rra, 3, 7, false, Addressing_AbsoluteX) // RRA
        INSTRUCTION(0x80, nop, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x81, sta, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0x82, nop, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x83, sax, 2, 6, false, Addressing_IndirectX) // SAX
        INSTRUCTION(0x84, sty, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x85, sta, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x86, stx, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0x87, sax, 2, 3, false, Addressing_ZeroPage) // SAX
        INSTRUCTION(0x88, dey, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x89, nop, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0x8a, txa, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x8b, xaa, 2, 2, false, Addressing_Immediate) // XAA
        INSTRUCTION(0x8c, sty, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x8d, sta, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x8e, stx, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0x8f, sax, 3, 4, false, Addressing_Absolute) // SAX
        INSTRUCTION(0x90, bcc, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0x91, sta, 2, 6, false, Addressing_IndirectY)
        INSTRUCTION(0x92, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0x93, axa, 2, 6, false, Addressing_IndirectY) // AXA
        INSTRUCTION(0x94, sty, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x95, sta, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0x96, stx, 2, 4, false, Addressing_ZeroPageY)
        INSTRUCTION(0x97, sax, 2, 4, false, Addressing_ZeroPageY) // SAX
        INSTRUCTION(0x98, tya, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x99, sta, 3, 5, false, Addressing_AbsoluteY)
        INSTRUCTION(0x9a, txs, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0x9b, xas, 3, 5, false, Addressing_AbsoluteY) // XAS
        INSTRUCTION(0x9c, sya, 3, 5, false, Addressing_AbsoluteX) // SYA
        INSTRUCTION(0x9d, sta, 3, 5, false, Addressing_AbsoluteX)
        INSTRUCTION(0x9e, sxa, 3, 5, false, Addressing_AbsoluteY) // SXA
        INSTRUCTION(0x9f, axa, 3, 5, false, Addressing_AbsoluteY) // AXA
        INSTRUCTION(0xa0, ldy, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xa1, lda, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0xa2, ldx, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xa3, lax, 2, 6, false, Addressing_IndirectX) // LAX
        INSTRUCTION(0xa4, ldy, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xa5, lda, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xa6, ldx, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xa7, lax, 2, 3, false, Addressing_ZeroPage) // LAX
        INSTRUCTION(0xa8, tay, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xa9, lda, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xaa, tax, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xab, atx, 2, 2, false, Addressing_Immediate) // ATX
        INSTRUCTION(0xac, ldy, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xad, lda, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xae, ldx, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xaf, lax, 3, 4, false, Addressing_Absolute) // LAX
        INSTRUCTION(0xb0, bcs, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0xb1, lda, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0xb2, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0xb3, lax, 2, 5, false, Addressing_IndirectY) // LAX
        INSTRUCTION(0xb4, ldy, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xb5, lda, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xb6, ldx, 2, 4, false, Addressing_ZeroPageY)
        INSTRUCTION(0xb7, lax, 2, 4, false, Addressing_ZeroPageY) // LAX
        INSTRUCTION(0xb8, clv, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xb9, lda, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0xba, tsx, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xbb, lar, 3, 4, false, Addressing_AbsoluteY) // LAR
        INSTRUCTION(0xbc, ldy, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xbd, lda, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xbe, ldx, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0xbf, lax, 3, 4, false, Addressing_AbsoluteY) // LAX
        INSTRUCTION(0xc0, cpy, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xc1, cmp, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0xc2, nop, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xc3, dcp, 2, 8, false, Addressing_IndirectX) // DCP
        INSTRUCTION(0xc4, cpy, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xc5, cmp, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xc6, dec, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0xc7, dcp, 2, 5, false, Addressing_ZeroPage) // DCP
        INSTRUCTION(0xc8, iny, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xc9, cmp, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xca, dex, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xcb, axs, 2, 2, false, Addressing_Immediate) // AXS
        INSTRUCTION(0xcc, cpy, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xcd, cmp, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xce, dec, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0xcf, dcp, 3, 6, false, Addressing_Absolute) // DCP
        INSTRUCTION(0xd0, bne, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0xd1, cmp, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0xd2, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0xd3, dcp, 2, 8, false, Addressing_IndirectY) // DCP
        INSTRUCTION(0xd4, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xd5, cmp, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xd6, dec, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0xd7, dcp, 2, 6, false, Addressing_ZeroPageX) // DCP
        INSTRUCTION(0xD8, cld, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xd9, cmp, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0xda, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xdb, dcp, 3, 7, false, Addressing_AbsoluteY) // DCP
        INSTRUCTION(0xdc, nop, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xdd, cmp, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xde, dec, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0xdf, dcp, 3, 7, false, Addressing_AbsoluteX) // DCP
        INSTRUCTION(0xe0, cpx, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xe1, sbc, 2, 6, false, Addressing_IndirectX)
        INSTRUCTION(0xe2, nop, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xe3, isb, 2, 8, false, Addressing_IndirectX) // ISB
        INSTRUCTION(0xe4, cpx, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xe5, sbc, 2, 3, false, Addressing_ZeroPage)
        INSTRUCTION(0xe6, inc, 2, 5, false, Addressing_ZeroPage)
        INSTRUCTION(0xe7, isb, 2, 5, false, Addressing_ZeroPage) // ISB
        INSTRUCTION(0xe8, inx, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xe9, sbc, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xea, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xeb, sbc, 2, 2, false, Addressing_Immediate)
        INSTRUCTION(0xec, cpx, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xed, sbc, 3, 4, false, Addressing_Absolute)
        INSTRUCTION(0xee, inc, 3, 6, false, Addressing_Absolute)
        INSTRUCTION(0xef, isb, 3, 6, false, Addressing_Absolute) // ISB
        INSTRUCTION(0xf0, beq, 2, 2, true, /*(+1 if branch succeeds +2 if to a new page)*/ Addressing_Relative)
        INSTRUCTION(0xf1, sbc, 2, 5, true, /*+1 if page crossed*/ Addressing_IndirectY)
        INSTRUCTION(0xf2, kil, 1, 3, false, Addressing_NoneAddressing)
        INSTRUCTION(0xf3, isb, 2, 8, false, Addressing_IndirectY) // ISB
        INSTRUCTION(0xf4, nop, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xf5, sbc, 2, 4, false, Addressing_ZeroPageX)
        INSTRUCTION(0xf6, inc, 2, 6, false, Addressing_ZeroPageX)
        INSTRUCTION(0xf7, isb, 2, 6, false, Addressing_ZeroPageX) // ISB
        INSTRUCTION(0xf8, sed, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xf9, sbc, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteY)
        INSTRUCTION(0xfa, nop, 1, 2, false, Addressing_NoneAddressing)
        INSTRUCTION(0xfb, isb, 3, 7, false, Addressing_AbsoluteY) // ISB
        INSTRUCTION(0xfc, nop, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xfd, sbc, 3, 4, true, /*+1 if page crossed*/ Addressing_AbsoluteX)
        INSTRUCTION(0xfe, inc, 3, 7, false, Addressing_AbsoluteX)
        INSTRUCTION(0xff, isb, 3, 7, false, Addressing_AbsoluteX) // ISB
        default:
            // Just putting this here so we don't get c warning, and intentionally cause crash
            int* p = nullptr;
            *p = 0;
            break;
    }
}