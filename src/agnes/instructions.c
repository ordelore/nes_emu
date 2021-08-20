#ifndef AGNES_SINGLE_HEADER
#include "instructions.h"

#include "agnes_types.h"
#include "cpu.h"
#endif

static int op_adc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_and(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_asl(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bcc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bcs(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_beq(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bit(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bmi(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bne(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bpl(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_brk(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bvc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_bvs(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_clc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_cld(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_cli(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_clv(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_cmp(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_cpx(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_cpy(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_dec(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_dex(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_dey(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_eor(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_inc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_inx(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_iny(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_jmp(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_jsr(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_lda(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_ldx(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_ldy(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_lsr(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_nop(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_ora(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_pha(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_php(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_pla(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_plp(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_rol(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_ror(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_rti(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_rts(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sbc(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sec(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sed(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sei(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sta(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_stx(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_sty(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_tax(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_tay(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_tsx(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_txa(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_txs(cpu_t *cpu, uint16_t addr, addr_mode_t mode);
static int op_tya(cpu_t *cpu, uint16_t addr, addr_mode_t mode);

static int take_branch(cpu_t *cpu, uint16_t addr);

#define INS(OPC, NAME, CYCLES, PCC, OP, MODE) { NAME, OPC, CYCLES, PCC, MODE, OP }
#define INE(OPC) { "ILL", OPC, 1, false, ADDR_MODE_IMPLIED, NULL }

static instruction_t instructions[256] = {
    INS(0x00, "BRK", 7, false, op_brk, ADDR_MODE_IMPLIED_BRK),
    INS(0x01, "ORA", 6, false, op_ora, ADDR_MODE_INDIRECT_X),
    INE(0x02),
    INE(0x03),
    INE(0x04),
    INS(0x05, "ORA", 3, false, op_ora, ADDR_MODE_ZERO_PAGE),
    INS(0x06, "ASL", 5, false, op_asl, ADDR_MODE_ZERO_PAGE),
    INE(0x07),
    INS(0x08, "PHP", 3, false, op_php, ADDR_MODE_IMPLIED),
    INS(0x09, "ORA", 2, false, op_ora, ADDR_MODE_IMMEDIATE),
    INS(0x0a, "ASL", 2, false, op_asl, ADDR_MODE_ACCUMULATOR),
    INE(0x0b),
    INE(0x0c),
    INS(0x0d, "ORA", 4, false, op_ora, ADDR_MODE_ABSOLUTE),
    INS(0x0e, "ASL", 6, false, op_asl, ADDR_MODE_ABSOLUTE),
    INE(0x0f),
    INS(0x10, "BPL", 2, true,  op_bpl, ADDR_MODE_RELATIVE),
    INS(0x11, "ORA", 5, true,  op_ora, ADDR_MODE_INDIRECT_Y),
    INE(0x12),
    INE(0x13),
    INE(0x14),
    INS(0x15, "ORA", 4, false, op_ora, ADDR_MODE_ZERO_PAGE_X),
    INS(0x16, "ASL", 6, false, op_asl, ADDR_MODE_ZERO_PAGE_X),
    INE(0x17),
    INS(0x18, "CLC", 2, false, op_clc, ADDR_MODE_IMPLIED),
    INS(0x19, "ORA", 4, true,  op_ora, ADDR_MODE_ABSOLUTE_Y),
    INE(0x1a),
    INE(0x1b),
    INE(0x1c),
    INS(0x1d, "ORA", 4, true,  op_ora, ADDR_MODE_ABSOLUTE_X),
    INS(0x1e, "ASL", 7, false, op_asl, ADDR_MODE_ABSOLUTE_X),
    INE(0x1f),
    INS(0x20, "JSR", 6, false, op_jsr, ADDR_MODE_ABSOLUTE),
    INS(0x21, "AND", 6, false, op_and, ADDR_MODE_INDIRECT_X),
    INE(0x22),
    INE(0x23),
    INS(0x24, "BIT", 3, false, op_bit, ADDR_MODE_ZERO_PAGE),
    INS(0x25, "AND", 3, false, op_and, ADDR_MODE_ZERO_PAGE),
    INS(0x26, "ROL", 5, false, op_rol, ADDR_MODE_ZERO_PAGE),
    INE(0x27),
    INS(0x28, "PLP", 4, false, op_plp, ADDR_MODE_IMPLIED),
    INS(0x29, "AND", 2, false, op_and, ADDR_MODE_IMMEDIATE),
    INS(0x2a, "ROL", 2, false, op_rol, ADDR_MODE_ACCUMULATOR),
    INE(0x2b),
    INS(0x2c, "BIT", 4, false, op_bit, ADDR_MODE_ABSOLUTE),
    INS(0x2d, "AND", 4, false, op_and, ADDR_MODE_ABSOLUTE),
    INS(0x2e, "ROL", 6, false, op_rol, ADDR_MODE_ABSOLUTE),
    INE(0x2f),
    INS(0x30, "BMI", 2, true,  op_bmi, ADDR_MODE_RELATIVE),
    INS(0x31, "AND", 5, true,  op_and, ADDR_MODE_INDIRECT_Y),
    INE(0x32),
    INE(0x33),
    INE(0x34),
    INS(0x35, "AND", 4, false, op_and, ADDR_MODE_ZERO_PAGE_X),
    INS(0x36, "ROL", 6, false, op_rol, ADDR_MODE_ZERO_PAGE_X),
    INE(0x37),
    INS(0x38, "SEC", 2, false, op_sec, ADDR_MODE_IMPLIED),
    INS(0x39, "AND", 4, true,  op_and, ADDR_MODE_ABSOLUTE_Y),
    INE(0x3a),
    INE(0x3b),
    INE(0x3c),
    INS(0x3d, "AND", 4, true,  op_and, ADDR_MODE_ABSOLUTE_X),
    INS(0x3e, "ROL", 7, false, op_rol, ADDR_MODE_ABSOLUTE_X),
    INE(0x3f),
    INS(0x40, "RTI", 6, false, op_rti, ADDR_MODE_IMPLIED),
    INS(0x41, "EOR", 6, false, op_eor, ADDR_MODE_INDIRECT_X),
    INE(0x42),
    INE(0x43),
    INE(0x44),
    INS(0x45, "EOR", 3, false, op_eor, ADDR_MODE_ZERO_PAGE),
    INS(0x46, "LSR", 5, false, op_lsr, ADDR_MODE_ZERO_PAGE),
    INE(0x47),
    INS(0x48, "PHA", 3, false, op_pha, ADDR_MODE_IMPLIED),
    INS(0x49, "EOR", 2, false, op_eor, ADDR_MODE_IMMEDIATE),
    INS(0x4a, "LSR", 2, false, op_lsr, ADDR_MODE_ACCUMULATOR),
    INE(0x4b),
    INS(0x4c, "JMP", 3, false, op_jmp, ADDR_MODE_ABSOLUTE),
    INS(0x4d, "EOR", 4, false, op_eor, ADDR_MODE_ABSOLUTE),
    INS(0x4e, "LSR", 6, false, op_lsr, ADDR_MODE_ABSOLUTE),
    INE(0x4f),
    INS(0x50, "BVC", 2, true,  op_bvc, ADDR_MODE_RELATIVE),
    INS(0x51, "EOR", 5, true,  op_eor, ADDR_MODE_INDIRECT_Y),
    INE(0x52),
    INE(0x53),
    INE(0x54),
    INS(0x55, "EOR", 4, false, op_eor, ADDR_MODE_ZERO_PAGE_X),
    INS(0x56, "LSR", 6, false, op_lsr, ADDR_MODE_ZERO_PAGE_X),
    INE(0x57),
    INS(0x58, "CLI", 2, false, op_cli, ADDR_MODE_IMPLIED),
    INS(0x59, "EOR", 4, true,  op_eor, ADDR_MODE_ABSOLUTE_Y),
    INE(0x5a),
    INE(0x5b),
    INE(0x5c),
    INS(0x5d, "EOR", 4, true,  op_eor, ADDR_MODE_ABSOLUTE_X),
    INS(0x5e, "LSR", 7, false, op_lsr, ADDR_MODE_ABSOLUTE_X),
    INE(0x5f),
    INS(0x60, "RTS", 6, false, op_rts, ADDR_MODE_IMPLIED),
    INS(0x61, "ADC", 6, false, op_adc, ADDR_MODE_INDIRECT_X),
    INE(0x62),
    INE(0x63),
    INE(0x64),
    INS(0x65, "ADC", 3, false, op_adc, ADDR_MODE_ZERO_PAGE),
    INS(0x66, "ROR", 5, false, op_ror, ADDR_MODE_ZERO_PAGE),
    INE(0x67),
    INS(0x68, "PLA", 4, false, op_pla, ADDR_MODE_IMPLIED),
    INS(0x69, "ADC", 2, false, op_adc, ADDR_MODE_IMMEDIATE),
    INS(0x6a, "ROR", 2, false, op_ror, ADDR_MODE_ACCUMULATOR),
    INE(0x6b),
    INS(0x6c, "JMP", 5, false, op_jmp, ADDR_MODE_INDIRECT),
    INS(0x6d, "ADC", 4, false,  op_adc, ADDR_MODE_ABSOLUTE),
    INS(0x6e, "ROR", 6, false, op_ror, ADDR_MODE_ABSOLUTE),
    INE(0x6f),
    INS(0x70, "BVS", 2, true,  op_bvs, ADDR_MODE_RELATIVE),
    INS(0x71, "ADC", 5, true,  op_adc, ADDR_MODE_INDIRECT_Y),
    INE(0x72),
    INE(0x73),
    INE(0x74),
    INS(0x75, "ADC", 4, false, op_adc, ADDR_MODE_ZERO_PAGE_X),
    INS(0x76, "ROR", 6, false, op_ror, ADDR_MODE_ZERO_PAGE_X),
    INE(0x77),
    INS(0x78, "SEI", 2, false, op_sei, ADDR_MODE_IMPLIED),
    INS(0x79, "ADC", 4, true,  op_adc, ADDR_MODE_ABSOLUTE_Y),
    INE(0x7a),
    INE(0x7b),
    INE(0x7c),
    INS(0x7d, "ADC", 4, true,  op_adc, ADDR_MODE_ABSOLUTE_X),
    INS(0x7e, "ROR", 7, false, op_ror, ADDR_MODE_ABSOLUTE_X),
    INE(0x7f),
    INE(0x80),
    INS(0x81, "STA", 6, false, op_sta, ADDR_MODE_INDIRECT_X),
    INE(0x82),
    INE(0x83),
    INS(0x84, "STY", 3, false, op_sty, ADDR_MODE_ZERO_PAGE),
    INS(0x85, "STA", 3, false, op_sta, ADDR_MODE_ZERO_PAGE),
    INS(0x86, "STX", 3, false, op_stx, ADDR_MODE_ZERO_PAGE),
    INE(0x87),
    INS(0x88, "DEY", 2, false, op_dey, ADDR_MODE_IMPLIED),
    INE(0x89),
    INS(0x8a, "TXA", 2, false, op_txa, ADDR_MODE_IMPLIED),
    INE(0x8b),
    INS(0x8c, "STY", 4, false, op_sty, ADDR_MODE_ABSOLUTE),
    INS(0x8d, "STA", 4, false, op_sta, ADDR_MODE_ABSOLUTE),
    INS(0x8e, "STX", 4, false, op_stx, ADDR_MODE_ABSOLUTE),
    INE(0x8f),
    INS(0x90, "BCC", 2, true,  op_bcc, ADDR_MODE_RELATIVE),
    INS(0x91, "STA", 6, false, op_sta, ADDR_MODE_INDIRECT_Y),
    INE(0x92),
    INE(0x93),
    INS(0x94, "STY", 4, false, op_sty, ADDR_MODE_ZERO_PAGE_X),
    INS(0x95, "STA", 4, false, op_sta, ADDR_MODE_ZERO_PAGE_X),
    INS(0x96, "STX", 4, false, op_stx, ADDR_MODE_ZERO_PAGE_Y),
    INE(0x97),
    INS(0x98, "TYA", 2, false, op_tya, ADDR_MODE_IMPLIED),
    INS(0x99, "STA", 5, false, op_sta, ADDR_MODE_ABSOLUTE_Y),
    INS(0x9a, "TXS", 2, false, op_txs, ADDR_MODE_IMPLIED),
    INE(0x9b),
    INE(0x9c),
    INS(0x9d, "STA", 5, false, op_sta, ADDR_MODE_ABSOLUTE_X),
    INE(0x9e),
    INE(0x9f),
    INS(0xa0, "LDY", 2, false, op_ldy, ADDR_MODE_IMMEDIATE),
    INS(0xa1, "LDA", 6, false, op_lda, ADDR_MODE_INDIRECT_X),
    INS(0xa2, "LDX", 2, false, op_ldx, ADDR_MODE_IMMEDIATE),
    INE(0xa3),
    INS(0xa4, "LDY", 3, false, op_ldy, ADDR_MODE_ZERO_PAGE),
    INS(0xa5, "LDA", 3, false, op_lda, ADDR_MODE_ZERO_PAGE),
    INS(0xa6, "LDX", 3, false, op_ldx, ADDR_MODE_ZERO_PAGE),
    INE(0xa7),
    INS(0xa8, "TAY", 2, false, op_tay, ADDR_MODE_IMPLIED),
    INS(0xa9, "LDA", 2, false, op_lda, ADDR_MODE_IMMEDIATE),
    INS(0xaa, "TAX", 2, false, op_tax, ADDR_MODE_IMPLIED),
    INE(0xab),
    INS(0xac, "LDY", 4, false, op_ldy, ADDR_MODE_ABSOLUTE),
    INS(0xad, "LDA", 4, false, op_lda, ADDR_MODE_ABSOLUTE),
    INS(0xae, "LDX", 4, false, op_ldx, ADDR_MODE_ABSOLUTE),
    INE(0xaf),
    INS(0xb0, "BCS", 2, true,  op_bcs, ADDR_MODE_RELATIVE),
    INS(0xb1, "LDA", 5, true,  op_lda, ADDR_MODE_INDIRECT_Y),
    INE(0xb2),
    INE(0xb3),
    INS(0xb4, "LDY", 4, false, op_ldy, ADDR_MODE_ZERO_PAGE_X),
    INS(0xb5, "LDA", 4, false, op_lda, ADDR_MODE_ZERO_PAGE_X),
    INS(0xb6, "LDX", 4, false, op_ldx, ADDR_MODE_ZERO_PAGE_Y),
    INE(0xb7),
    INS(0xb8, "CLV", 2, false, op_clv, ADDR_MODE_IMPLIED),
    INS(0xb9, "LDA", 4, true,  op_lda, ADDR_MODE_ABSOLUTE_Y),
    INS(0xba, "TSX", 2, false, op_tsx, ADDR_MODE_IMPLIED),
    INE(0xbb),
    INS(0xbc, "LDY", 4, true,  op_ldy, ADDR_MODE_ABSOLUTE_X),
    INS(0xbd, "LDA", 4, true,  op_lda, ADDR_MODE_ABSOLUTE_X),
    INS(0xbe, "LDX", 4, true,  op_ldx, ADDR_MODE_ABSOLUTE_Y),
    INE(0xbf),
    INS(0xc0, "CPY", 2, false, op_cpy, ADDR_MODE_IMMEDIATE),
    INS(0xc1, "CMP", 6, false, op_cmp, ADDR_MODE_INDIRECT_X),
    INE(0xc2),
    INE(0xc3),
    INS(0xc4, "CPY", 3, false, op_cpy, ADDR_MODE_ZERO_PAGE),
    INS(0xc5, "CMP", 3, false, op_cmp, ADDR_MODE_ZERO_PAGE),
    INS(0xc6, "DEC", 5, false, op_dec, ADDR_MODE_ZERO_PAGE),
    INE(0xc7),
    INS(0xc8, "INY", 2, false, op_iny, ADDR_MODE_IMPLIED),
    INS(0xc9, "CMP", 2, false, op_cmp, ADDR_MODE_IMMEDIATE),
    INS(0xca, "DEX", 2, false, op_dex, ADDR_MODE_IMPLIED),
    INE(0xcb),
    INS(0xcc, "CPY", 4, false, op_cpy, ADDR_MODE_ABSOLUTE),
    INS(0xcd, "CMP", 4, false, op_cmp, ADDR_MODE_ABSOLUTE),
    INS(0xce, "DEC", 6, false, op_dec, ADDR_MODE_ABSOLUTE),
    INE(0xcf),
    INS(0xd0, "BNE", 2, true,  op_bne, ADDR_MODE_RELATIVE),
    INS(0xd1, "CMP", 5, true,  op_cmp, ADDR_MODE_INDIRECT_Y),
    INE(0xd2),
    INE(0xd3),
    INE(0xd4),
    INS(0xd5, "CMP", 4, false, op_cmp, ADDR_MODE_ZERO_PAGE_X),
    INS(0xd6, "DEC", 6, false, op_dec, ADDR_MODE_ZERO_PAGE_X),
    INE(0xd7),
    INS(0xd8, "CLD", 2, false, op_cld, ADDR_MODE_IMPLIED),
    INS(0xd9, "CMP", 4, true,  op_cmp, ADDR_MODE_ABSOLUTE_Y),
    INE(0xda),
    INE(0xdb),
    INE(0xdc),
    INS(0xdd, "CMP", 4, true,  op_cmp, ADDR_MODE_ABSOLUTE_X),
    INS(0xde, "DEC", 7, false, op_dec, ADDR_MODE_ABSOLUTE_X),
    INE(0xdf),
    INS(0xe0, "CPX", 2, false, op_cpx, ADDR_MODE_IMMEDIATE),
    INS(0xe1, "SBC", 6, false, op_sbc, ADDR_MODE_INDIRECT_X),
    INE(0xe2),
    INE(0xe3),
    INS(0xe4, "CPX", 3, false, op_cpx, ADDR_MODE_ZERO_PAGE),
    INS(0xe5, "SBC", 3, false, op_sbc, ADDR_MODE_ZERO_PAGE),
    INS(0xe6, "INC", 5, false, op_inc, ADDR_MODE_ZERO_PAGE),
    INE(0xe7),
    INS(0xe8, "INX", 2, false, op_inx, ADDR_MODE_IMPLIED),
    INS(0xe9, "SBC", 2, false, op_sbc, ADDR_MODE_IMMEDIATE),
    INS(0xea, "NOP", 2, false, op_nop, ADDR_MODE_IMPLIED),
    INE(0xeb),
    INS(0xec, "CPX", 4, false, op_cpx, ADDR_MODE_ABSOLUTE),
    INS(0xed, "SBC", 4, false, op_sbc, ADDR_MODE_ABSOLUTE),
    INS(0xee, "INC", 6, false, op_inc, ADDR_MODE_ABSOLUTE),
    INE(0xef),
    INS(0xf0, "BEQ", 2, true,  op_beq, ADDR_MODE_RELATIVE),
    INS(0xf1, "SBC", 5, true,  op_sbc, ADDR_MODE_INDIRECT_Y),
    INE(0xf2),
    INE(0xf3),
    INE(0xf4),
    INS(0xf5, "SBC", 4, false, op_sbc, ADDR_MODE_ZERO_PAGE_X),
    INS(0xf6, "INC", 6, false, op_inc, ADDR_MODE_ZERO_PAGE_X),
    INE(0xf7),
    INS(0xf8, "SED", 2, false, op_sed, ADDR_MODE_IMPLIED),
    INS(0xf9, "SBC", 4, true,  op_sbc, ADDR_MODE_ABSOLUTE_Y),
    INE(0xfa),
    INE(0xfb),
    INE(0xfc),
    INS(0xfd, "SBC", 4, true,  op_sbc, ADDR_MODE_ABSOLUTE_X),
    INS(0xfe, "INC", 7, false, op_inc, ADDR_MODE_ABSOLUTE_X),
    INE(0xff),
};

#undef INE
#undef INS

instruction_t* instruction_get(uint8_t opc) {
    return &instructions[opc];
}

uint8_t instruction_get_size(addr_mode_t mode) {
    switch (mode) {
        case ADDR_MODE_NONE:        return 0;
        case ADDR_MODE_ABSOLUTE:    return 3;
        case ADDR_MODE_ABSOLUTE_X:  return 3;
        case ADDR_MODE_ABSOLUTE_Y:  return 3;
        case ADDR_MODE_ACCUMULATOR: return 1;
        case ADDR_MODE_IMMEDIATE:   return 2;
        case ADDR_MODE_IMPLIED:     return 1;
        case ADDR_MODE_IMPLIED_BRK: return 2;
        case ADDR_MODE_INDIRECT:    return 3;
        case ADDR_MODE_INDIRECT_X:  return 2;
        case ADDR_MODE_INDIRECT_Y:  return 2;
        case ADDR_MODE_RELATIVE:    return 2;
        case ADDR_MODE_ZERO_PAGE:   return 2;
        case ADDR_MODE_ZERO_PAGE_X: return 2;
        case ADDR_MODE_ZERO_PAGE_Y: return 2;
        default: return 0;
    }
}

static int op_adc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t old_acc = cpu->acc;
    uint8_t val = cpu_read8(cpu, addr);
    int res = cpu->acc + val + (uint8_t)cpu->flag_carry;
    cpu->acc = (uint8_t)res;
    cpu->flag_carry = res > 0xff;
    cpu->flag_overflow = !((old_acc ^ val) & 0x80) && ((old_acc ^ cpu->acc) & 0x80);
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_and(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->acc = cpu->acc & val;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_asl(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    if (mode == ADDR_MODE_ACCUMULATOR) {
        cpu->flag_carry = AGNES_GET_BIT(cpu->acc, 7);
        cpu->acc = cpu->acc << 1;
        cpu_update_zn_flags(cpu, cpu->acc);
    } else {
        uint8_t val = cpu_read8(cpu, addr);
        cpu->flag_carry = AGNES_GET_BIT(val, 7);
        val = val << 1;
        cpu_write8(cpu, addr, val);
        cpu_update_zn_flags(cpu, val);
    }
    return 0;
}

static int op_bcc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return !cpu->flag_carry ? take_branch(cpu, addr) : 0;
}

static int op_bcs(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return cpu->flag_carry ? take_branch(cpu, addr) : 0;
}

static int op_beq(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return cpu->flag_zero ? take_branch(cpu, addr) : 0;
}

static int op_bit(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    uint8_t res = cpu->acc & val;
    cpu->flag_zero = res == 0;
    cpu->flag_overflow = AGNES_GET_BIT(val, 6);
    cpu->flag_negative = AGNES_GET_BIT(val, 7);
    return 0;
}

static int op_bmi(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return cpu->flag_negative ? take_branch(cpu, addr) : 0;
}

static int op_bne(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return !cpu->flag_zero ? take_branch(cpu, addr) : 0;
}

static int op_bpl(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return !cpu->flag_negative ? take_branch(cpu, addr) : 0;
}

static int op_brk(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_stack_push16(cpu, cpu->pc);
    uint8_t flags = cpu_get_flags(cpu);
    cpu_stack_push8(cpu, flags | 0x30);
    cpu->pc = cpu_read16(cpu, 0xfffe);
    cpu->flag_dis_interrupt = true;
    return 0;
}

static int op_bvc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return !cpu->flag_overflow ? take_branch(cpu, addr) : 0;
}

static int op_bvs(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return cpu->flag_overflow ? take_branch(cpu, addr) : 0;
}

static int op_clc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_carry = false;
    return 0;
}

static int op_cld(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_decimal = false;
    return 0;
}

static int op_cli(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_dis_interrupt = false;
    return 0;
}

static int op_clv(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_overflow = false;
    return 0;
}

static int op_cmp(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu_update_zn_flags(cpu, cpu->acc - val);
    cpu->flag_carry = cpu->acc >= val;
    return 0;
}

static int op_cpx(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu_update_zn_flags(cpu, cpu->x - val);
    cpu->flag_carry = cpu->x >= val;
    return 0;
}

static int op_cpy(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu_update_zn_flags(cpu, cpu->y - val);
    cpu->flag_carry = cpu->y >= val;
    return 0;
}

static int op_dec(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu_write8(cpu, addr, val - 1);
    cpu_update_zn_flags(cpu, val - 1);
    return 0;
}

static int op_dex(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->x--;
    cpu_update_zn_flags(cpu, cpu->x);
    return 0;
}

static int op_dey(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->y--;
    cpu_update_zn_flags(cpu, cpu->y);
    return 0;
}

static int op_eor(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->acc = cpu->acc ^ val;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_inc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu_write8(cpu, addr, val + 1);
    cpu_update_zn_flags(cpu, val + 1);
    return 0;
}

static int op_inx(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->x++;
    cpu_update_zn_flags(cpu, cpu->x);
    return 0;
}

static int op_iny(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->y++;
    cpu_update_zn_flags(cpu, cpu->y);
    return 0;
}

static int op_jmp(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->pc = addr;
    return 0;
}

static int op_jsr(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_stack_push16(cpu, cpu->pc - 1);
    cpu->pc = addr;
    return 0;
}

static int op_lda(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->acc = val;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_ldx(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->x = val;
    cpu_update_zn_flags(cpu, cpu->x);
    return 0;
}

static int op_ldy(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->y = val;
    cpu_update_zn_flags(cpu, cpu->y);
    return 0;
}

static int op_lsr(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    if (mode == ADDR_MODE_ACCUMULATOR) {
        cpu->flag_carry = AGNES_GET_BIT(cpu->acc, 0);
        cpu->acc = cpu->acc >> 1;
        cpu_update_zn_flags(cpu, cpu->acc);
    } else {
        uint8_t val = cpu_read8(cpu, addr);
        cpu->flag_carry = AGNES_GET_BIT(val, 0);
        val = val >> 1;
        cpu_write8(cpu, addr, val);
        cpu_update_zn_flags(cpu, val);
    }
    return 0;
}

static int op_nop(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    return 0;
}

static int op_ora(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    cpu->acc = cpu->acc | val;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_pha(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_stack_push8(cpu, cpu->acc);
    return 0;
}

static int op_php(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t flags = cpu_get_flags(cpu);
    cpu_stack_push8(cpu, flags | 0x30);
    return 0;
}

static int op_pla(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->acc = cpu_stack_pop8(cpu);
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_plp(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t flags = cpu_stack_pop8(cpu);
    cpu_restore_flags(cpu, flags);
    return 0;
}

static int op_rol(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t old_carry = cpu->flag_carry;
    if (mode == ADDR_MODE_ACCUMULATOR) {
        cpu->flag_carry = AGNES_GET_BIT(cpu->acc, 7);
        cpu->acc = (cpu->acc << 1) | old_carry;
        cpu_update_zn_flags(cpu, cpu->acc);
    } else {
        uint8_t val = cpu_read8(cpu, addr);
        cpu->flag_carry = AGNES_GET_BIT(val, 7);
        val = (val << 1) | old_carry;
        cpu_write8(cpu, addr, val);
        cpu_update_zn_flags(cpu, val);
    }
    return 0;
}

static int op_ror(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t old_carry = cpu->flag_carry;
    if (mode == ADDR_MODE_ACCUMULATOR) {
        cpu->flag_carry = AGNES_GET_BIT(cpu->acc, 0);
        cpu->acc = (cpu->acc >> 1) | (old_carry << 7);
        cpu_update_zn_flags(cpu, cpu->acc);
    } else {
        uint8_t val = cpu_read8(cpu, addr);
        cpu->flag_carry = AGNES_GET_BIT(val, 0);
        val = (val >> 1) | (old_carry << 7);
        cpu_write8(cpu, addr, val);
        cpu_update_zn_flags(cpu, val);
    }
    return 0;
}

static int op_rti(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t flags = cpu_stack_pop8(cpu);
    cpu_restore_flags(cpu, flags);
    cpu->pc = cpu_stack_pop16(cpu);
    return 0;
}

static int op_rts(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->pc = cpu_stack_pop16(cpu) + 1;
    return 0;
}

static int op_sbc(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    uint8_t val = cpu_read8(cpu, addr);
    uint8_t old_acc = cpu->acc;
    int res = cpu->acc - val - (cpu->flag_carry ? 0 : 1);
    cpu->acc = (uint8_t)res;
    cpu_update_zn_flags(cpu, cpu->acc);
    cpu->flag_carry = res >= 0;
    cpu->flag_overflow = ((old_acc ^ val) & 0x80) && ((old_acc ^ cpu->acc) & 0x80);
    return 0;
}

static int op_sec(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_carry = true;
    return 0;
}

static int op_sed(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_decimal = true;
    return 0;
}

static int op_sei(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->flag_dis_interrupt = true;
    return 0;
}

static int op_sta(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_write8(cpu, addr, cpu->acc);
    return 0;
}

static int op_stx(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_write8(cpu, addr, cpu->x);
    return 0;
}

static int op_sty(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu_write8(cpu, addr, cpu->y);
    return 0;
}

static int op_tax(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->x = cpu->acc;
    cpu_update_zn_flags(cpu, cpu->x);
    return 0;
}

static int op_tay(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->y = cpu->acc;
    cpu_update_zn_flags(cpu, cpu->y);
    return 0;
}

static int op_tsx(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->x = cpu->sp;
    cpu_update_zn_flags(cpu, cpu->x);
    return 0;
}

static int op_txa(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->acc = cpu->x;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int op_txs(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->sp = cpu->x;
    return 0;
}

static int op_tya(cpu_t *cpu, uint16_t addr, addr_mode_t mode) {
    cpu->acc = cpu->y;
    cpu_update_zn_flags(cpu, cpu->acc);
    return 0;
}

static int take_branch(cpu_t *cpu, uint16_t addr) {
    bool page_crossed = (cpu->pc & 0xff00) != (addr & 0xff00);
    cpu->pc = addr;
    return page_crossed ? 2 : 1;
}
