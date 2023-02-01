#include "log.hpp"

#include <cstdint>
#include <cstring>

#define HYRISC_PIPELINE_SIZE 3

#define HY_COP0_CR0_XSTACKED_ISR    0x00000001
#define HY_COP0_CR0_XFLUSH_ON_IRQ   0x00000002
#define HY_COP0_CR0_XSTALL_ACCESS   0x00000004

struct mmu_map_entry_t {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t attr;
};

struct range_t {
    uint32_t start, end;
};

class mmio_device_t {
public:
    virtual range_t get_physical_range() = 0;
    virtual uint32_t read(uint32_t, int) = 0;
    virtual void write(uint32_t, uint32_t, int) = 0;
};

#include <vector>

struct hyrisc_t {
    uint32_t r[32];

    uint32_t alu_t0, alu_t1;
    
    uint32_t pipeline[3];

    int cycle;

    // COP0
    uint32_t cop0_cr0;
    uint32_t cop0_cr1;
    uint32_t cop0_xpc;
    uint32_t cop0_xcause;
    uint32_t cop0_xhaddr;

    // COP4 (MMU)
    std::vector <mmio_device_t*> mmu_devices;

    mmu_map_entry_t mmu_map[32];
};

#define HY_CAUSE_SYSCALL         0
#define HY_CAUSE_DEBUG           1
#define HY_CAUSE_SEXCEPT         2
#define HY_CAUSE_MMU_NOMAP       3
#define HY_CAUSE_MMU_PROT_READ   4
#define HY_CAUSE_MMU_PROT_WRITE  5
#define HY_CAUSE_MMU_PROT_EXEC   6
#define HY_CAUSE_ILLEGAL_INSTR   7
#define HY_CAUSE_INVALID_COPX    8

#define MMU_ATTR_READ  1
#define MMU_ATTR_WRITE 2
#define MMU_ATTR_EXEC  4

#define HY_BYTE  0
#define HY_SHORT 1
#define HY_LONG  2
#define HY_EXEC  3

void hyrisc_exception(hyrisc_t* cpu, uint32_t cause) {
    cpu->cop0_xcause = cause;
    cpu->cop0_xpc = cpu->r[31];
    cpu->r[31] = cpu->cop0_xhaddr;
}

mmu_map_entry_t* mmu_search_map(hyrisc_t* cpu, uint32_t vaddr) {
    for (int i = 0; i < 32; i++) {
        uint32_t map_vaddr = cpu->mmu_map[i].vaddr;
        uint32_t map_size = cpu->mmu_map[i].size;
    
        if ((vaddr >= map_vaddr) && (vaddr < (map_vaddr + map_size)))
            return &cpu->mmu_map[i];
    }

    return nullptr;
}

uint32_t mmu_get_physical_address(mmu_map_entry_t* me, uint32_t vaddr) {
    return me->paddr + (vaddr - me->vaddr);
}

uint32_t mmu_read(hyrisc_t* cpu, uint32_t addr, int size) {
    mmu_map_entry_t* me = mmu_search_map(cpu, addr);

    if (!me) {
        hyrisc_exception(cpu, HY_CAUSE_MMU_NOMAP);

        return 0x00000000;
    }

    if ((size == HY_EXEC) && !(me->attr & MMU_ATTR_EXEC)) {
        hyrisc_exception(cpu, HY_CAUSE_MMU_PROT_EXEC);
    }

    if (!(me->attr & MMU_ATTR_READ)) {
        hyrisc_exception(cpu, HY_CAUSE_MMU_PROT_READ);

        return 0x00000000;
    }

    for (mmio_device_t* dev : cpu->mmu_devices) {
        range_t range = dev->get_physical_range();

        uint32_t paddr = mmu_get_physical_address(me, addr);

        if (paddr >= range.start && paddr < range.end) {
            return dev->read(paddr - range.start, size);
        }
    }
}

hyrisc_t* hyrisc_create() {
    return new hyrisc_t;
}

void hyrisc_init(hyrisc_t* cpu) {
    std::memset(cpu, 0, sizeof(hyrisc_t));
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

inline uint32_t hyrisc_d_instr    (uint32_t opc) { return (opc >> 27) & 0x1f; }
inline uint32_t hyrisc_d_d        (uint32_t opc) { return (opc >> 22) & 0x1f; }
inline uint32_t hyrisc_d_s0       (uint32_t opc) { return (opc >> 17) & 0x1f; }
inline uint32_t hyrisc_d_s1       (uint32_t opc) { return (opc >> 12) & 0x1f; }
inline uint32_t hyrisc_d_s2       (uint32_t opc) { return (opc >>  7) & 0x1f; }
inline uint32_t hyrisc_d_alu_op   (uint32_t opc) { return (opc >>  2) & 0xf; }
inline uint32_t hyrisc_d_alu_i    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hyrisc_d_alu_sx   (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hyrisc_d_alu_imm  (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hyrisc_d_brn_imm8 (uint32_t opc) { return (opc >>  4) & 0xff; }
inline uint32_t hyrisc_d_brn_c    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hyrisc_d_brn_i    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hyrisc_d_brn_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hyrisc_d_brn_l    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hyrisc_d_cpe_copr (uint32_t opc) { return (opc >> 12) & 0x3ff; }
inline uint32_t hyrisc_d_cpe_copn (uint32_t opc) { return (opc >>  8) & 0x1f; }
inline uint32_t hyrisc_d_cpe_op   (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hyrisc_d_cpi_opc  (uint32_t opc) { return (opc >>  4) & 0xffffff; }
inline uint32_t hyrisc_d_sys_imm24(uint32_t opc) { return (opc      ) & 0xffffff; }
inline uint32_t hyrisc_d_sys_op   (uint32_t opc) { return (opc >> 24) & 0x7; }
inline uint32_t hyrisc_d_lsl_imm10(uint32_t opc) { return (opc >>  7) & 0x3ff; }
inline uint32_t hyrisc_d_lsl_size (uint32_t opc) { return (opc >>  5) & 0x3; }
inline uint32_t hyrisc_d_lsl_op   (uint32_t opc) { return (opc >>  3) & 0x3; }
inline uint32_t hyrisc_d_lsl_mode (uint32_t opc) { return (opc      ) & 0x7; }
inline uint32_t hyrisc_d_li_imm16 (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hyrisc_d_li_sx    (uint32_t opc) { return (opc >>  5) & 0x1; }
inline uint32_t hyrisc_d_li_shift (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hyrisc_d_sci_cond (uint32_t opc) { return (opc >> 28) & 0x7; }
inline uint32_t hyrisc_d_sci_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hyrisc_d_sci_sx   (uint32_t opc) { return (opc      ) & 0x1; }

typedef void (*hyrisc_alu_op_t)(uint32_t*, uint32_t, uint32_t);

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

typedef bool (*hyrisc_cond_t)(uint32_t, uint32_t);

bool cond_eq(uint32_t r0, uint32_t r1) { return r0 == r1; }
bool cond_ne(uint32_t r0, uint32_t r1) { return r0 != r1; }
bool cond_gt(uint32_t r0, uint32_t r1) { return r0 > r1; }
bool cond_ge(uint32_t r0, uint32_t r1) { return r0 >= r1; }
bool cond_lt(uint32_t r0, uint32_t r1) { return r0 < r1; }
bool cond_le(uint32_t r0, uint32_t r1) { return r0 <= r1; }

typedef void (*hyrisc_cpe_op_t)(hyrisc_t* cpu, uint32_t, uint32_t, uint32_t);

uint32_t* get_cop_register(hyrisc_t* cpu, uint32_t copn, uint32_t copr) {
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
    }

    return nullptr;
}

void cpe_mtcr(hyrisc_t* cpu, uint32_t copn, uint32_t cpur, uint32_t copr) {
    uint32_t* cr = get_cop_register(cpu, copn, copr);

    if (!cr) {
        hyrisc_exception(cpu, HY_CAUSE_INVALID_COPX);
    } else {
        *cr = cpu->r[cpur];
    }

}

void cpe_mfcr(hyrisc_t* cpu, uint32_t copn, uint32_t cpur, uint32_t copr) {
    uint32_t* cr = get_cop_register(cpu, copn, copr);
    
    if (!cr) {
        hyrisc_exception(cpu, HY_CAUSE_INVALID_COPX);
    } else {
        cpu->r[cpur] = *cr;
    }
}

hyrisc_alu_op_t hyrisc_alu_op_table[] = {
    alu_add, alu_sub, alu_mul, alu_mla,
    alu_div, alu_mod, alu_and, alu_or ,
    alu_xor, alu_lsl, alu_lsr, alu_asr,
    alu_sxb, alu_sxs, alu_rol, alu_ror
};

hyrisc_cond_t hyrisc_cond_table[] = {
    cond_eq, cond_ne,
    cond_gt, cond_ge,
    cond_lt, cond_le
};

hyrisc_cpe_op_t hyrisc_cpe_op_table[] = {
    cpe_mtcr,
    cpe_mfcr
};

inline uint32_t sign_extend16_if(uint32_t v, bool cond) {
    if (!cond) return v;

    v &= 0xffff;
    
    return (v & 0x8000) ? (v | 0xffff0000) : v;
}

inline uint32_t sign_extend17(uint32_t v) {
    v &= 0x1ffff;

    return (v & 0x10000) ? (v | 0xfffe0000) : v;
}

void hyrisc_execute(hyrisc_t* cpu) {
    uint32_t opcode = cpu->pipeline[2];
    uint32_t instr = hyrisc_d_instr(opcode);

    switch (instr) {
        // ALU
        case 0b00000: {
            uint32_t op = hyrisc_d_alu_op(opcode);
            uint32_t d = hyrisc_d_d(opcode);
            uint32_t s0, s1;
            
            switch (hyrisc_d_alu_i(opcode)) {
                // Register mode
                case 0: {
                    s0 = cpu->r[hyrisc_d_s0(opcode)];
                    s1 = cpu->r[hyrisc_d_s1(opcode)];
                } break;

                // Immediate mode
                case 1: {
                    s0 = cpu->r[d];

                    s1 = sign_extend16_if(hyrisc_d_alu_imm(opcode), hyrisc_d_alu_sx(opcode));
                } break;
            }

            hyrisc_alu_op_table[op](&cpu->r[d], s0, s1);
        } break;

        // Branch immediate
        case 0b00010: case 0b00100:
        case 0b00110: case 0b01000:
        case 0b01010: case 0b01100:
        case 0b10010: case 0b10100:
        case 0b10110: case 0b11000:
        case 0b11010: case 0b11100: {
            uint32_t cond = (instr >> 1) & 0x7;

            uint32_t d = hyrisc_d_d(opcode);
            uint32_t s0 = hyrisc_d_s0(opcode);

            if (hyrisc_cond_table[cond](cpu->r[d], cpu->r[s0])) {
                uint32_t imm = hyrisc_d_brn_imm16(opcode) | ((instr & 0x10) << 12);

                // Copy PC to LR
                if (hyrisc_d_brn_l(opcode))
                    cpu->r[30] = cpu->r[31];

                cpu->r[31] += sign_extend17(imm);
            }
        } break;

        // Branch register
        case 0b01101: {
            uint32_t cond = hyrisc_d_brn_c(opcode);
            uint32_t d = hyrisc_d_d(opcode);
            uint32_t s0 = hyrisc_d_s0(opcode);

            if (hyrisc_cond_table[cond](cpu->r[d], cpu->r[s0])) {
                uint32_t s1 = hyrisc_d_s1(opcode);

                if (hyrisc_d_brn_i(opcode)) {
                    cpu->r[31] += cpu->r[s1];
                } else {
                    cpu->r[31] = cpu->r[s1];
                }
            }
        } break;

        // COP-CPU exchange
        case 0b01110: {
            hyrisc_cpe_op_table[hyrisc_d_cpe_op(opcode)](
                cpu,
                hyrisc_d_cpe_copn(opcode),
                hyrisc_d_d(opcode),
                hyrisc_d_cpe_copr(opcode)
            );
        } break;

        // COP instruction
        case 0b11110: case 0b11111: {
            // To-do:
            // None implemented yet
            // COP0 is not an EC
        } break;

        // System
        case 0b01111: {
            uint32_t c = hyrisc_d_sys_imm24(opcode);

            switch (hyrisc_d_sys_op(opcode)) {
                // syscall
                case 0b000: {
                    hyrisc_exception(cpu, HY_CAUSE_SYSCALL | (c << 8));
                } break;

                // To-do: tpl0-3
                case 0b001: case 0b010:
                case 0b011: case 0b100: {
                    hyrisc_exception(cpu, HY_CAUSE_ILLEGAL_INSTR);
                } break;

                // debug
                case 0b101: {
                    hyrisc_exception(cpu, HY_CAUSE_DEBUG | (c << 8));
                } break;

                // excep
                case 0b110: {
                    hyrisc_exception(cpu, HY_CAUSE_SEXCEPT | (c << 8));
                } break;

                // Reserved
                default: {
                    hyrisc_exception(cpu, HY_CAUSE_ILLEGAL_INSTR);
                } break;
            } 
        } break;

        // Load/Store/LEA
        case 0b10000: {
            uint32_t addr;

            uint32_t d = hyrisc_d_d(opcode);
            uint32_t s0 = hyrisc_d_s0(opcode);

            uint32_t mode = hyrisc_d_lsl_mode(opcode);

            switch (mode) {
                // Add scaled register
                case 0b000: {
                    uint32_t s1 = hyrisc_d_s1(opcode);
                    uint32_t s2 = hyrisc_d_s2(opcode);

                    addr = cpu->r[s0] + (cpu->r[s1] * s2);
                } break;
                
                // Sub scaled register
                case 0b001: {
                    uint32_t s1 = hyrisc_d_s1(opcode);
                    uint32_t s2 = hyrisc_d_s2(opcode);

                    addr = cpu->r[s0] - (cpu->r[s1] * s2);
                } break;

                // Add shifted register
                case 0b010: {
                    uint32_t s1 = hyrisc_d_s1(opcode);
                    uint32_t s2 = hyrisc_d_s2(opcode);

                    addr = cpu->r[s0] + (cpu->r[s1] << s2);
                } break;
                
                // Sub shifted register
                case 0b011: {
                    uint32_t s1 = hyrisc_d_s1(opcode);
                    uint32_t s2 = hyrisc_d_s2(opcode);

                    addr = cpu->r[s0] - (cpu->r[s1] << s2);
                } break;

                // Add fixed
                case 0b100: case 0b110: {
                    uint32_t imm11 = hyrisc_d_lsl_imm10(opcode) | ((mode & 2) << 9);

                    addr = cpu->r[s0] + imm11;
                } break;

                // Sub fixed
                case 0b101: case 0b111: {
                    uint32_t imm11 = hyrisc_d_lsl_imm10(opcode) | ((mode & 2) << 9);

                    addr = cpu->r[s0] - imm11;
                } break;
            }

            uint32_t size = hyrisc_d_lsl_size(opcode);

            switch (hyrisc_d_lsl_op(opcode)) {
                // load
                case 0: {
                    cpu->r[d] = mmu_read(cpu, addr, size);
                } break;

                // Store
                case 1: {
                    /* To-do */
                } break;

                // LEA
                case 2: {
                    cpu->r[d] = addr;
                } break;
            }
        } break;

        // Load immediate
        case 0b10001: {
            uint32_t d = hyrisc_d_d(opcode);
            uint32_t imm = hyrisc_d_li_imm16(opcode);
            uint32_t shift = hyrisc_d_li_shift(opcode);

            cpu->r[d] = sign_extend16_if(imm, hyrisc_d_li_sx(opcode)) << shift;
        } break;

        // Set if cond immediate
        case 0b10011: case 0b10101:
        case 0b10111: case 0b11001:
        case 0b11011: case 0b11101: {
            uint32_t d = hyrisc_d_d(opcode);
            uint32_t s0 = hyrisc_d_s0(opcode);
            uint32_t imm = sign_extend16_if(hyrisc_d_sci_imm16(opcode), hyrisc_d_sci_sx(opcode));

            cpu->r[d] = hyrisc_cond_table[(instr >> 1) & 0x7](s0, imm) ? 1 : 0;
        } break;

        default: {
            hyrisc_exception(cpu, HY_CAUSE_ILLEGAL_INSTR);
        } break;
    }

    cpu->r[0] = 0;
}

void hyrisc_cycle(hyrisc_t* cpu) {
    cpu->pipeline[2] = cpu->pipeline[1];
    cpu->pipeline[1] = cpu->pipeline[0];
    cpu->pipeline[0] = mmu_read(cpu, cpu->r[31], HY_EXEC);

    hyrisc_execute(cpu);
}

int main() {
    hyrisc_t* cpu = hyrisc_create();

    hyrisc_init(cpu);

    // li.u     x0, 0x80000000
    _log(debug, "r1=%08x, r2=%08x", cpu->r[3], cpu->r[2]);

    cpu->pipeline[2] = 0x88f7ab50;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x", cpu->r[3], cpu->r[2]);

    cpu->pipeline[2] = 0x00efbbde;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x", cpu->r[3], cpu->r[2]);

    cpu->pipeline[2] = 0x00c0043a;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x, cop0_cr0=%08x", cpu->r[3], cpu->r[2], cpu->cop0_cr0);
    
    cpu->pipeline[2] = 0x00c0023a;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x, cop0_cr0=%08x", cpu->r[3], cpu->r[2], cpu->cop0_cr0);

    cpu->pipeline[2] = 0x00c0043a;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x, cop0_cr0=%08x", cpu->r[3], cpu->r[2], cpu->cop0_cr0);
    
    cpu->pipeline[2] = 0x00c0023a;
    hyrisc_execute(cpu);
    _log(debug, "r1=%08x, r2=%08x, cop0_cr0=%08x", cpu->r[3], cpu->r[2], cpu->cop0_cr0);
}
