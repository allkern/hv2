#pragma once

#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>

enum hv2d_register_style_t : int {
    HV2D_RS_MIPS,       // $0, $1
    HV2D_RS_NO_ABI,     // r0, r1
    HV2D_RS_HS_ABI      // r0, at
};

enum hv2d_symbol_style_t : int {
    HV2D_SS_COMPACT,    // symbol: pc: opcode mnemonic operands
    HV2D_SS_OBJDUMP     // addr <symbol>:
                        //      pc: opcode mnemonic operands
};

struct hv2d_symbol_t {
    std::string name;

    uint32_t addr;

    int type;
};

struct hv2_disassembler_t {
    bool print_symbols = true;
    unsigned int mnemonic_max_size = 8;
    unsigned int opcode_max_size = 16;
    unsigned int addr_max_size = 16;
    unsigned int line_max_size = 80;
    unsigned int line_indent = 4;

    hv2d_register_style_t rs = HV2D_RS_HS_ABI;
    char rm = '\0';
    char im = '\0';

    uint32_t vaddr = 0;
    size_t mnemonic_pos = 0;
    size_t operands_pos = 0;
    std::string mnemonic;
    std::string operands;

    std::unordered_map <uint32_t, std::string> symbols;
};

std::string hv2d_print_register(hv2_disassembler_t*, uint32_t);
std::string hv2d_print_integer(hv2_disassembler_t*, const char*, uint32_t);
std::string hv2d_disassemble(hv2_disassembler_t*, uint32_t);
void hv2d_register_symbol(hv2_disassembler_t*, std::string, uint32_t, int);
hv2_disassembler_t* hv2d_create();
void hv2d_init(hv2_disassembler_t*);