#include "hv2.hpp"
#include "exception.hpp"

#include <cstdio>
#include <cstdlib>

hv2_t* hv2_create() {
    return new hv2_t;
}

void hv2_init(hv2_t* cpu) {
    //std::memset(cpu, 0, sizeof(hv2_t));
}

/*                              31    24 23    16 15     8 7      0
    ALU register:               00000xxx xxyyyyyz zzzzz--- --OOOOMS
    ALU immediate:              00000xxx xxIIIIII IIIIIIII IIOOOOMS
    Branch register:            01101xxx xxyyyyyz zzzzwwww wIIIcccM
    Branch immediate:           Sccc0xxx xxyyyyyI IIIIIIII IIIIIIIL
    Coprocessor-CPU exchange:   01110xxx xxyyyyyy yyyycccc c--OOOOO
    Coprocessor instruction:    1111iiii iiiiiiii iiiiiiii iiiicccc
    System:                     01111ooo cccccccc cccccccc cccccccc
    Load/Store/LEA Fixed:       iiiiixxx xxyyyyyI IIIIIIII ISSOOmmm
    Load/Store/LEA Register:    iiiiixxx xxyyyyyz zzzzwwww wSSOOmmm
    Load immediate:             10001xxx xxIIIIII IIIIIIII IISsssss
    Set Cond Immediate:         1ccc1xxx xxyyyyyI IIIIIIII IIIIIIIS
*/

inline uint32_t hv2_d_instr    (uint32_t opc) { return (opc >> 27) & 0x1f; }
inline uint32_t hv2_d_d        (uint32_t opc) { return (opc >> 22) & 0x1f; }
inline uint32_t hv2_d_s0       (uint32_t opc) { return (opc >> 17) & 0x1f; }
inline uint32_t hv2_d_s1       (uint32_t opc) { return (opc >> 12) & 0x1f; }
inline uint32_t hv2_d_s2       (uint32_t opc) { return (opc >>  7) & 0x1f; }
inline uint32_t hv2_d_alu_op   (uint32_t opc) { return (opc >>  2) & 0xf; }
inline uint32_t hv2_d_alu_i    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hv2_d_alu_sx   (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2_d_alu_imm  (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hv2_d_brn_imm8 (uint32_t opc) { return (opc >>  4) & 0xff; }
inline uint32_t hv2_d_brn_c    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hv2_d_brn_i    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2_d_brn_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hv2_d_brn_l    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2_d_cpe_copr (uint32_t opc) { return (opc >> 12) & 0x3ff; }
inline uint32_t hv2_d_cpe_copn (uint32_t opc) { return (opc >>  8) & 0x1f; }
inline uint32_t hv2_d_cpe_op   (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hv2_d_cpi_opc  (uint32_t opc) { return (opc >>  4) & 0xffffff; }
inline uint32_t hv2_d_sys_imm24(uint32_t opc) { return (opc      ) & 0xffffff; }
inline uint32_t hv2_d_sys_op   (uint32_t opc) { return (opc >> 24) & 0x7; }
inline uint32_t hv2_d_lsl_imm10(uint32_t opc) { return (opc >>  7) & 0x3ff; }
inline uint32_t hv2_d_lsl_size (uint32_t opc) { return (opc >>  5) & 0x3; }
inline uint32_t hv2_d_lsl_op   (uint32_t opc) { return (opc >>  3) & 0x3; }
inline uint32_t hv2_d_lsl_mode (uint32_t opc) { return (opc      ) & 0x7; }
inline uint32_t hv2_d_li_imm16 (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hv2_d_li_sx    (uint32_t opc) { return (opc >>  5) & 0x1; }
inline uint32_t hv2_d_li_shift (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hv2_d_sci_cond (uint32_t opc) { return (opc >> 28) & 0x7; }
inline uint32_t hv2_d_sci_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hv2_d_sci_sx   (uint32_t opc) { return (opc      ) & 0x1; }

typedef void (*hv2_alu_op_t)(uint32_t*, uint32_t, uint32_t);

inline uint32_t asr_impl(uint32_t v, uint32_t s) {
    if (v & 0x80000000) {
        v >>= s;

        v |= ~(0xffffffff >> s);
    } else {
        v >>= s;
    }
    
    return v;
}

inline uint32_t rol_impl(uint32_t v, uint32_t r) {
    r %= 32;

    return (v << r) | (v >> (32 - r));
}

inline uint32_t ror_impl(uint32_t v, uint32_t r) {
    r %= 32;

    return (v >> r) | (v << (32 - r));
}

void alu_add(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 + s1; }
void alu_sub(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 - s1; }
void alu_mul(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 * s1; }
void alu_mla(uint32_t* d, uint32_t s0, uint32_t s1) { *d += s0 * s1; }
void alu_div(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 / s1; }
void alu_mod(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 % s1; }
void alu_and(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 & s1; }
void alu_or (uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 | s1; }
void alu_xor(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 ^ s1; }
void alu_lsl(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 << s1; }
void alu_lsr(uint32_t* d, uint32_t s0, uint32_t s1) { *d = s0 >> s1; }
void alu_asr(uint32_t* d, uint32_t s0, uint32_t s1) { *d = asr_impl(s0, s1); }
void alu_sxb(uint32_t* d, uint32_t s0, uint32_t s1) { s0 &= 0xff; *d = (s0 & 0x80) ? (s0 | 0xffffff00) : s0; }
void alu_sxs(uint32_t* d, uint32_t s0, uint32_t s1) { s0 &= 0xffff; *d = (s0 & 0x8000) ? (s0 | 0xffff0000) : s0; }
void alu_rol(uint32_t* d, uint32_t s0, uint32_t s1) { *d = rol_impl(s0, s1); }
void alu_ror(uint32_t* d, uint32_t s0, uint32_t s1) { *d = ror_impl(s0, s1); }

typedef bool (*hv2_cond_t)(uint32_t, uint32_t);

bool cond_eq(uint32_t r0, uint32_t r1) { return r0 == r1; }
bool cond_ne(uint32_t r0, uint32_t r1) { return r0 != r1; }
bool cond_gt(uint32_t r0, uint32_t r1) { return r0 > r1; }
bool cond_ge(uint32_t r0, uint32_t r1) { return r0 >= r1; }
bool cond_lt(uint32_t r0, uint32_t r1) { return r0 < r1; }
bool cond_le(uint32_t r0, uint32_t r1) { return r0 <= r1; }

typedef void (*hv2_cpe_op_t)(hv2_t* cpu, uint32_t, uint32_t, uint32_t);

uint32_t* hv2_get_cop_register(hv2_t* cpu, uint32_t copn, uint32_t copr) {
    switch (copn) {
        // COP0 (SCU)
        case 0: {
            switch (copr) {
                case 0: { return &cpu->cop0_cr0; } break;
                case 1: { return &cpu->cop0_cr1; } break;
                case 2: { return &cpu->cop0_xcause; } break;
                case 3: { return &cpu->cop0_xhaddr; } break;
                case 4: { return &cpu->cop0_xpc; } break;
            }
        } break;

        // COP4 (MMU)
        case 4: {
            if (copr == 0) {
                return &cpu->cop4_ctrl;
            }

            constexpr unsigned mmu_map_size = sizeof(hv2_mmu_entry_t) * 32;

            if ((copr >= 1) && (copr < (1 + mmu_map_size))) {
                uint32_t* ptr = (uint32_t*)&cpu->mmu_map;

                return &ptr[copr - 1];
            }
        } break;
    }

    return nullptr;
}

void cpe_mtcr(hv2_t* cpu, uint32_t copn, uint32_t cpur, uint32_t copr) {
    uint32_t* cr = hv2_get_cop_register(cpu, copn, copr);

    if (!cr) {
        hv2_exception(cpu, HV2_CAUSE_INVALID_COPX);
    } else {
        *cr = cpu->r[cpur];
    }

}

void cpe_mfcr(hv2_t* cpu, uint32_t copn, uint32_t cpur, uint32_t copr) {
    uint32_t* cr = hv2_get_cop_register(cpu, copn, copr);
    
    if (!cr) {
        hv2_exception(cpu, HV2_CAUSE_INVALID_COPX);
    } else {
        cpu->r[cpur] = *cr;
    }
}

hv2_alu_op_t hv2_alu_op_table[] = {
    alu_add, alu_sub, alu_mul, alu_mla,
    alu_div, alu_mod, alu_and, alu_or ,
    alu_xor, alu_lsl, alu_lsr, alu_asr,
    alu_sxb, alu_sxs, alu_rol, alu_ror
};

hv2_cond_t hv2_cond_table[] = {
    cond_eq, cond_ne,
    cond_gt, cond_ge,
    cond_lt, cond_le
};

hv2_cpe_op_t hv2_cpe_op_table[] = {
    cpe_mtcr,
    cpe_mfcr
};

inline uint32_t sign_extend16_if(uint32_t v, bool cond) {
    if (!cond) return v;

    v &= 0xffff;
    
    return (v & 0x8000) ? (v | 0xffff0000) : v;
}

inline int32_t sign_extend17(uint32_t v) {
    v &= 0x1ffff;

    return (v & 0x10000) ? (v | 0xfffe0000) : v;
}

void hv2_flush(hv2_t* cpu, uint32_t d) {
    if ((d == 31) && (cpu->cop0_cr0 & HV2_COP0_CR0_XFLUSH_ON_FT)) {
        cpu->pipeline[0] = 0;
        cpu->pipeline[1] = 0;
        cpu->pipeline[2] = 0;
    }
}

void hv2_execute(hv2_t* cpu) {
    uint32_t opcode = cpu->pipeline[2];
    uint32_t instr = hv2_d_instr(opcode);

    switch (instr) {
        // ALU
        case 0b00000: {
            uint32_t op = hv2_d_alu_op(opcode);
            uint32_t d = hv2_d_d(opcode);
            uint32_t s0, s1;
            
            switch (hv2_d_alu_i(opcode)) {
                // Register mode
                case 0: {
                    s0 = cpu->r[hv2_d_s0(opcode)];
                    s1 = cpu->r[hv2_d_s1(opcode)];
                } break;

                // Immediate mode
                case 1: {
                    s0 = cpu->r[d];

                    s1 = sign_extend16_if(hv2_d_alu_imm(opcode), hv2_d_alu_sx(opcode));
                } break;
            }

            hv2_alu_op_table[op](&cpu->r[d], s0, s1);

            hv2_flush(cpu, d);
        } break;

        // Branch immediate
        case 0b00010: case 0b00100:
        case 0b00110: case 0b01000:
        case 0b01010: case 0b01100:
        case 0b10010: case 0b10100:
        case 0b10110: case 0b11000:
        case 0b11010: case 0b11100: {
            uint32_t cond = (instr >> 1) & 0x7;

            uint32_t d = hv2_d_d(opcode);
            uint32_t s0 = hv2_d_s0(opcode);

            if (hv2_cond_table[cond - 1](cpu->r[d], cpu->r[s0])) {
                uint32_t imm = hv2_d_brn_imm16(opcode) | ((instr & 0x10) << 12);

                // Copy PC to LR
                if (hv2_d_brn_l(opcode))
                    cpu->r[30] = cpu->r[31];
                
                //printf("taken %08x (%08x)\n", cpu->r[31], sign_extend17(imm));

                cpu->r[31] += sign_extend17(imm);

                //printf("taken %08x after\n", cpu->r[31]);

                hv2_flush(cpu, 31);
            }
        } break;

        // Branch register
        case 0b01101: {
            uint32_t cond = hv2_d_brn_c(opcode);
            uint32_t d = hv2_d_d(opcode);
            uint32_t s0 = hv2_d_s0(opcode);

            if (hv2_cond_table[cond](cpu->r[d], cpu->r[s0])) {
                uint32_t s1 = hv2_d_s1(opcode);

                if (hv2_d_brn_i(opcode)) {
                    cpu->r[31] += cpu->r[s1];
                } else {
                    cpu->r[31] = cpu->r[s1];
                }

                hv2_flush(cpu, 31);
            }
        } break;

        // COP-CPU exchange
        case 0b01110: {
            hv2_cpe_op_table[hv2_d_cpe_op(opcode)](
                cpu,
                hv2_d_cpe_copn(opcode),
                hv2_d_d(opcode),
                hv2_d_cpe_copr(opcode)
            );

            hv2_flush(cpu, hv2_d_d(opcode));
        } break;

        // COP instruction
        case 0b11110: case 0b11111: {
            // To-do:
            // None implemented yet
            // COP0 is not an EC
        } break;

        // System
        case 0b01111: {
            uint32_t c = hv2_d_sys_imm24(opcode);

            switch (hv2_d_sys_op(opcode)) {
                // syscall
                case 0b000: {
                    if (c == 0xabcd00) {
                        std::putchar(cpu->r[1] & 0xff);
                    } else {
                        hv2_exception(cpu, HV2_CAUSE_SYSCALL | (c << 8));
                    }
                } break;

                // To-do: tpl0-3
                case 0b001: case 0b010:
                case 0b011: case 0b100: {
                    hv2_exception(cpu, HV2_CAUSE_ILLEGAL_INSTR);
                } break;

                // debug
                case 0b101: {
                    if (c == 0xadc0de) {
                        std::printf("\na0=%08x\n", cpu->r[2]);

                        std::exit(0);
                    }

                    hv2_exception(cpu, HV2_CAUSE_DEBUG | (c << 8));
                } break;

                // excep
                case 0b110: {
                    hv2_exception(cpu, HV2_CAUSE_SEXCEPT | (c << 8));
                } break;

                // Reserved
                default: {
                    hv2_exception(cpu, HV2_CAUSE_ILLEGAL_INSTR);
                } break;
            } 
        } break;

        // Load/Store/LEA
        case 0b10000: {
            uint32_t addr;

            uint32_t d = hv2_d_d(opcode);
            uint32_t s0 = hv2_d_s0(opcode);

            uint32_t mode = hv2_d_lsl_mode(opcode);

            switch (mode) {
                // Add scaled register
                case 0b000: {
                    uint32_t s1 = hv2_d_s1(opcode);
                    uint32_t s2 = hv2_d_s2(opcode);

                    addr = cpu->r[s0] + (cpu->r[s1] * s2);
                } break;
                
                // Sub scaled register
                case 0b001: {
                    uint32_t s1 = hv2_d_s1(opcode);
                    uint32_t s2 = hv2_d_s2(opcode);

                    addr = cpu->r[s0] - (cpu->r[s1] * s2);
                } break;

                // Add shifted register
                case 0b010: {
                    uint32_t s1 = hv2_d_s1(opcode);
                    uint32_t s2 = hv2_d_s2(opcode);

                    addr = cpu->r[s0] + (cpu->r[s1] << s2);
                } break;
                
                // Sub shifted register
                case 0b011: {
                    uint32_t s1 = hv2_d_s1(opcode);
                    uint32_t s2 = hv2_d_s2(opcode);

                    addr = cpu->r[s0] - (cpu->r[s1] << s2);
                } break;

                // Add fixed
                case 0b100: case 0b110: {
                    uint32_t imm11 = hv2_d_lsl_imm10(opcode) | ((mode & 2) << 9);

                    addr = cpu->r[s0] + imm11;
                } break;

                // Sub fixed
                case 0b101: case 0b111: {
                    uint32_t imm11 = hv2_d_lsl_imm10(opcode) | ((mode & 2) << 9);

                    addr = cpu->r[s0] - imm11;
                } break;
            }

            uint32_t size = hv2_d_lsl_size(opcode);

            switch (hv2_d_lsl_op(opcode)) {
                // Load
                case 0: {
                    cpu->r[d] = hv2_mmu_read(cpu, addr, size);

                    hv2_flush(cpu, d);
                } break;

                // Store
                case 1: {
                    hv2_mmu_write(cpu, addr, cpu->r[d], size);
                } break;

                // LEA
                case 2: {
                    cpu->r[d] = addr;

                    hv2_flush(cpu, d);
                } break;
            }
        } break;

        // Load immediate
        case 0b10001: {
            uint32_t d = hv2_d_d(opcode);
            uint32_t imm = hv2_d_li_imm16(opcode);
            uint32_t shift = hv2_d_li_shift(opcode);

            cpu->r[d] = sign_extend16_if(imm, hv2_d_li_sx(opcode)) << shift;

            hv2_flush(cpu, d);
        } break;

        // Set if cond immediate
        case 0b10011: case 0b10101:
        case 0b10111: case 0b11001:
        case 0b11011: case 0b11101: {
            uint32_t d = hv2_d_d(opcode);
            uint32_t s0 = hv2_d_s0(opcode);
            uint32_t imm = sign_extend16_if(hv2_d_sci_imm16(opcode), hv2_d_sci_sx(opcode));

            cpu->r[d] = hv2_cond_table[(instr >> 1) & 0x7](s0, imm) ? 1 : 0;

            hv2_flush(cpu, d);
        } break;

        default: {
            hv2_exception(cpu, HV2_CAUSE_ILLEGAL_INSTR);
        } break;
    }

    cpu->r[0] = 0;
}

#include "disas.hpp"

void hv2_cycle(hv2_t* cpu) {
    cpu->pipeline[2] = cpu->pipeline[1];
    cpu->pipeline[1] = cpu->pipeline[0];
    cpu->pipeline[0] = hv2_mmu_read(cpu, cpu->r[31], HV2_EXEC);

    cpu->r[31] += 4;

    // std::printf("%08x: %s sp=%08x, fp=%08x, x0=%08x, a0=%08x, at=%08x\n",
    //     cpu->r[31],
    //     hv2d_disassemble(cpu->pipeline[2]).c_str(),
    //     cpu->r[29],
    //     cpu->r[28],
    //     cpu->r[3],
    //     cpu->r[2],
    //     cpu->r[1]
    // );

    hv2_execute(cpu);
}