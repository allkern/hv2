#include "disas.hpp"

std::unordered_map <uint32_t, std::string> regn_name_abi_map = {
    { 0, "r0" },
    { 1, "at" },
    { 2, "a0" },
    { 3, "x0" },
    { 4, "x1" },
    { 5, "x2" },
    { 6, "x3" },
    { 7, "x4" },
    { 8, "x5" },
    { 9, "x6" },
    { 10, "x7" },
    { 11, "x8" },
    { 12, "x9" },
    { 13, "x10" },
    { 14, "x11" },
    { 15, "x12" },
    { 16, "x13" },
    { 17, "x14" },
    { 18, "x15" },
    { 19, "x16" },
    { 20, "x17" },
    { 21, "x18" },
    { 22, "x19" },
    { 23, "x20" },
    { 24, "x21" },
    { 25, "x22" },
    { 26, "x23" },
    { 27, "x24" },
    { 28, "fp" },
    { 29, "sp" },
    { 30, "lr" },
    { 31, "pc" }
};

std::unordered_map <uint32_t, std::string> alu_op_mnemonic_map = {
    { 0 , "add"  }, { 1 , "sub" }, { 2 , "mul" }, { 3 , "mla" },
    { 4 , "div"  }, { 5 , "mod" }, { 6 , "and" }, { 7 , "or"  },
    { 8 , "xor"  }, { 9 , "lsl" }, { 10, "lsr" }, { 11, "asr" },
    { 12, "sx.b" }, { 13, "sx.s"}, { 14, "rol" }, { 15, "ror" }
};

std::unordered_map <uint32_t, std::string> cc_cond_map = {
    { 1, "eq" }, { 2, "ne" },
    { 3, "gt" }, { 4, "ge" },
    { 5, "lt" }, { 6, "le" }
};

std::unordered_map <uint32_t, std::string> copx_mnemonic_map = {
    { 0, "mtcr" },
    { 1, "mfcr" }
};

std::unordered_map <uint32_t, bool> copx_direction_map = {
    { 0, false },
    { 1, true }
};

std::unordered_map <uint32_t, std::string> sys_op_mnemonic_map = {
    { 0, "syscall" },
    { 1, "tpl0"    },
    { 2, "tpl1"    },
    { 3, "tpl2"    },
    { 4, "tpl3"    },
    { 5, "debug"   },
    { 6, "excep"   }
};

std::unordered_map <uint32_t, std::string> lsl_op_mnemonic_map = {
    { 0, "load"  },
    { 1, "store" },
    { 2, "lea"   }
};

std::unordered_map <uint32_t, bool> lsl_op_direction_map = {
    { 0, false },
    { 1, true  },
    { 2, false }
};

std::unordered_map <uint32_t, std::string> lsl_size_mod_map = {
    { 0, ".b" },
    { 1, ".s" },
    { 2, ".l" },
    { 3, ".x" }
};

inline uint32_t hv2d_d_instr    (uint32_t opc) { return (opc >> 27) & 0x1f; }
inline uint32_t hv2d_d_d        (uint32_t opc) { return (opc >> 22) & 0x1f; }
inline uint32_t hv2d_d_s0       (uint32_t opc) { return (opc >> 17) & 0x1f; }
inline uint32_t hv2d_d_s1       (uint32_t opc) { return (opc >> 12) & 0x1f; }
inline uint32_t hv2d_d_s2       (uint32_t opc) { return (opc >>  7) & 0x1f; }
inline uint32_t hv2d_d_alu_op   (uint32_t opc) { return (opc >>  2) & 0xf; }
inline uint32_t hv2d_d_alu_i    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hv2d_d_alu_sx   (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2d_d_alu_imm  (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hv2d_d_brn_imm8 (uint32_t opc) { return (opc >>  4) & 0xff; }
inline uint32_t hv2d_d_brn_c    (uint32_t opc) { return (opc >>  1) & 0x1; }
inline uint32_t hv2d_d_brn_i    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2d_d_brn_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hv2d_d_brn_l    (uint32_t opc) { return (opc      ) & 0x1; }
inline uint32_t hv2d_d_cpe_copr (uint32_t opc) { return (opc >> 12) & 0x3ff; }
inline uint32_t hv2d_d_cpe_copn (uint32_t opc) { return (opc >>  8) & 0x1f; }
inline uint32_t hv2d_d_cpe_op   (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hv2d_d_cpi_opc  (uint32_t opc) { return (opc >>  4) & 0xffffff; }
inline uint32_t hv2d_d_sys_imm24(uint32_t opc) { return (opc      ) & 0xffffff; }
inline uint32_t hv2d_d_sys_op   (uint32_t opc) { return (opc >> 24) & 0x7; }
inline uint32_t hv2d_d_lsl_imm10(uint32_t opc) { return (opc >>  7) & 0x3ff; }
inline uint32_t hv2d_d_lsl_size (uint32_t opc) { return (opc >>  5) & 0x3; }
inline uint32_t hv2d_d_lsl_op   (uint32_t opc) { return (opc >>  3) & 0x3; }
inline uint32_t hv2d_d_lsl_mode (uint32_t opc) { return (opc      ) & 0x7; }
inline uint32_t hv2d_d_li_imm16 (uint32_t opc) { return (opc >>  6) & 0xffff; }
inline uint32_t hv2d_d_li_sx    (uint32_t opc) { return (opc >>  5) & 0x1; }
inline uint32_t hv2d_d_li_shift (uint32_t opc) { return (opc      ) & 0x1f; }
inline uint32_t hv2d_d_sci_cond (uint32_t opc) { return (opc >> 28) & 0x7; }
inline uint32_t hv2d_d_sci_imm16(uint32_t opc) { return (opc >>  1) & 0xffff; }
inline uint32_t hv2d_d_sci_sx   (uint32_t opc) { return (opc      ) & 0x1; }

inline uint32_t hv2d_sign_extend16_if(uint32_t v, bool cond) {
    if (!cond) return v;

    v &= 0xffff;
    
    return (v & 0x8000) ? (v | 0xffff0000) : v;
}

inline int32_t hv2d_sign_extend17(uint32_t v) {
    v &= 0x1ffff;

    return (v & 0x10000) ? (v | 0xfffe0000) : v;
}

std::string hv2d_print_register(hv2_disassembler_t* dis, uint32_t r) {
    if (dis->rs == HV2D_RS_MIPS) {
        return "$" + std::to_string(r);
    }

    if (dis->rs == HV2D_RS_NO_ABI) {
        std::string str = "r" + std::to_string(r);

        if (dis->rm) {
            return std::string(1, dis->rm) + str;
        } else {
            return str;
        }
    }

    if (dis->rs == HV2D_RS_HS_ABI) {
        std::string str = regn_name_abi_map[r];

        if (dis->rm) {
            return std::string(1, dis->rm) + str;
        } else {
            return str;
        }
    }

    return "";
}

std::string set_string_to_size(std::string str, size_t size) {
    if (str.size() <= size) {
        str += std::string(size - str.size(), ' ');
    } else {
        str = str.substr(0, size - 3);
        str += "...";
    }

    return str;
}

std::string hv2d_print_integer(hv2_disassembler_t* dis, const char* fmt, uint32_t n) {
    std::string format;
    
    if (dis->im) {
        format = std::string(1, dis->im) + fmt;
    } else {
        format = fmt;
    }

    char buf[256];

    std::sprintf(buf, format.c_str(), n);

    return std::string(buf);
}

std::string hv2d_disassemble(hv2_disassembler_t* dis, uint32_t opcode) {
    dis->mnemonic = "";
    dis->operands = "";

    uint32_t instr = hv2d_d_instr(opcode);

    if (opcode == 0) {
        dis->mnemonic = "nop";

        goto skip;
    }

    switch (instr) {
        // ALU
        case 0b00000: {
            std::string d, s0, s1;
            std::string stem = alu_op_mnemonic_map[hv2d_d_alu_op(opcode)];
            
            d = hv2d_print_register(dis, hv2d_d_d(opcode));
            
            switch (hv2d_d_alu_i(opcode)) {
                // Register mode
                case 0: {
                    dis->mnemonic = stem;

                    s0 = hv2d_print_register(dis, hv2d_d_s0(opcode));
                    s1 = hv2d_print_register(dis, hv2d_d_s1(opcode));

                    dis->operands = d + ", " + s0 + ", " + s1;
                } break;

                // Immediate mode
                case 1: {
                    bool sx = hv2d_d_alu_sx(opcode);

                    std::string dm = sx ? ".s" : ".u";

                    s0 = hv2d_print_integer(dis, sx ? "0x%+04x" : "0x%04x", hv2d_d_alu_imm(opcode));

                    dis->mnemonic = stem + dm;
                    dis->operands = d + ", " + s0;
                } break;
            }
        } break;

        // Branch immediate
        case 0b00010: case 0b00100:
        case 0b00110: case 0b01000:
        case 0b01010: case 0b01100:
        case 0b10010: case 0b10100:
        case 0b10110: case 0b11000:
        case 0b11010: case 0b11100: {
            uint32_t cond = (instr >> 1) & 0x7;

            bool link = hv2d_d_brn_l(opcode);

            dis->mnemonic = (link ? "bl" : "b") + cc_cond_map[cond];

            std::string d = hv2d_print_register(dis, hv2d_d_d(opcode));
            std::string s0 = hv2d_print_register(dis, hv2d_d_s0(opcode));

            uint32_t imm = hv2d_d_brn_imm16(opcode) | ((instr & 0x10) << 12);

            imm = hv2d_sign_extend17(imm);

            dis->operands = d + ", " + s0 + ", " + hv2d_print_integer(dis, "0x%+x", imm);
        } break;

        // Branch register
        case 0b01101: {
            dis->mnemonic = "BRR to-do";
            dis->operands = "to-do";
        } break;

        // COP-CPU exchange
        case 0b01110: {
            uint32_t cpe_op = hv2d_d_cpe_op(opcode);

            dis->mnemonic = copx_mnemonic_map[cpe_op];

            bool direction = copx_direction_map[cpe_op];

            std::string copn = hv2d_print_integer(dis, "$%u", hv2d_d_cpe_copn(opcode));
            std::string copr = hv2d_print_integer(dis, "$%u", hv2d_d_cpe_copr(opcode));
            std::string d = hv2d_print_register(dis, hv2d_d_d(opcode));

            if (direction) {
                dis->operands = d + ", " + copn + ", " + copr;
            } else {
                dis->operands = copn + ", " + copr + ", " + d;
            }
        } break;

        // COP instruction
        case 0b11110: case 0b11111: {
            // To-do:
            // None implemented yet
            // COP0 is not an EC
        } break;

        // System
        case 0b01111: {
            dis->mnemonic = sys_op_mnemonic_map[hv2d_d_sys_op(opcode)];
            dis->operands = hv2d_print_integer(dis, "0x%x", hv2d_d_sys_imm24(opcode));
        } break;

        // Load/Store/LEA
        case 0b10000: {
            std::string dm = lsl_size_mod_map[hv2d_d_lsl_size(opcode)];

            dis->mnemonic = lsl_op_mnemonic_map[hv2d_d_lsl_op(opcode)] + dm;

            std::string d = hv2d_print_register(dis, hv2d_d_d(opcode));
            std::string s0 = hv2d_print_register(dis, hv2d_d_s0(opcode));

            uint32_t mode = hv2d_d_lsl_mode(opcode);
            bool direction = lsl_op_direction_map[hv2d_d_lsl_op(opcode)];

            switch (mode) {
                // Add scaled register
                case 0b000: {
                    std::string s1 = hv2d_print_register(dis, hv2d_d_s1(opcode));
                    std::string s2 = hv2d_print_integer(dis, "0x%x", hv2d_d_s2(opcode));

                    if (direction) {
                        dis->operands = "[" + s0 + "+" + s1 + "*" + s2 + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "+" + s1 + "*" + s2 + "]";
                    }
                } break;
                
                // Sub scaled register
                case 0b001: {
                    std::string s1 = hv2d_print_register(dis, hv2d_d_s1(opcode));
                    std::string s2 = hv2d_print_integer(dis, "0x%x", hv2d_d_s2(opcode));

                    if (direction) {
                        dis->operands = "[" + s0 + "-" + s1 + "*" + s2 + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "-" + s1 + "*" + s2 + "]";
                    }
                } break;

                // Add shifted register
                case 0b010: {
                    std::string s1 = hv2d_print_register(dis, hv2d_d_s1(opcode));
                    std::string s2 = hv2d_print_integer(dis, "0x%x", hv2d_d_s2(opcode));

                    if (direction) {
                        dis->operands = "[" + s0 + "+" + s1 + ":" + s2 + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "+" + s1 + ":" + s2 + "]";
                    }
                } break;
                
                // Sub shifted register
                case 0b011: {
                    std::string s1 = hv2d_print_register(dis, hv2d_d_s1(opcode));
                    std::string s2 = hv2d_print_integer(dis, "0x%x", hv2d_d_s2(opcode));

                    if (direction) {
                        dis->operands = "[" + s0 + "-" + s1 + ":" + s2 + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "-" + s1 + ":" + s2 + "]";
                    }
                } break;

                // Add fixed
                case 0b100: case 0b110: {
                    std::string imm = hv2d_print_integer(dis, "0x%x", hv2d_d_lsl_imm10(opcode) | ((mode & 2) << 9));

                    if (direction) {
                        dis->operands = "[" + s0 + "+" + imm + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "+" + imm + "]";
                    }
                } break;

                // Sub fixed
                case 0b101: case 0b111: {
                    std::string imm = hv2d_print_integer(dis, "0x%x", hv2d_d_lsl_imm10(opcode) | ((mode & 2) << 9));

                    if (direction) {
                        dis->operands = "[" + s0 + "-" + imm + "], " + d;
                    } else {
                        dis->operands = d + ", [" + s0 + "-" + imm + "]";
                    }
                } break;
            }
        } break;

        // Load immediate
        case 0b10001: {
            uint32_t shift = hv2d_d_li_shift(opcode);
            uint32_t immv = hv2d_sign_extend16_if(hv2d_d_li_imm16(opcode), hv2d_d_li_sx(opcode)) << shift;

            dis->mnemonic = "li";
            
            std::string d = hv2d_print_register(dis, hv2d_d_d(opcode));
            std::string imm = hv2d_print_integer(dis, "0x%08x", immv);

            dis->operands = d + ", " + imm;
        } break;

        // Set if cond immediate
        case 0b10011: case 0b10101:
        case 0b10111: case 0b11001:
        case 0b11011: case 0b11101: {
            dis->mnemonic = "SCI to-do";
            dis->operands = "to-do";
        } break;
    }

    skip:

    std::string opcode_str;

    if (dis->opcode_max_size) {
        int shift = 32;

        while (shift) {
            shift -= 8;

            uint8_t b = (opcode >> shift) & 0xff;
            
            opcode_str += hv2d_print_integer(dis, "%02x", b);
            opcode_str += " ";
        }

        opcode_str = set_string_to_size(opcode_str, dis->opcode_max_size);
    }

    dis->mnemonic = set_string_to_size(dis->mnemonic, dis->mnemonic_max_size);

    std::string line = opcode_str + dis->mnemonic + " " + dis->operands;

    if (dis->addr_max_size) {
        std::string addr = hv2d_print_integer(dis, "%08x", dis->vaddr) + ":";

        addr = set_string_to_size(addr, dis->addr_max_size);

        line = addr + line;
    }

    line = std::string(dis->line_indent, ' ') + line;

    if (dis->symbols.contains(dis->vaddr) && dis->print_symbols) {
        std::string addr = hv2d_print_integer(dis, "%08x", dis->vaddr);
        std::string symbol = "<" + dis->symbols[dis->vaddr] + ">";
        std::string symbol_line = "\n" + addr + " " + symbol + ":\n";

        dis->line_max_size += symbol_line.size();

        line = symbol_line + line;
    }

    line = set_string_to_size(line, dis->line_max_size);

    dis->vaddr += 4;

    return line;
}

void hv2d_register_symbol(hv2_disassembler_t* dis, std::string symbol, uint32_t addr, int type) {
    dis->symbols.insert({ addr, symbol });
}

hv2_disassembler_t* hv2d_create() {
    return new hv2_disassembler_t;
}

void hv2d_init(hv2_disassembler_t* dis) {
    std::memset(dis, 0, sizeof(hv2_disassembler_t));
}