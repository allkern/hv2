#include "hv2/hv2.hpp"
#include "hv2/mmu.hpp"
#include "hv2/log.hpp"

// Devices
#include "dev/ram.hpp"
#include "dev/vga_textmode.hpp"

// 64 MiB @ 80000000
#define CPU_SPEED 1000000 // 1 MHz
#define RAM_SIZE 0x4000000
#define RAM_BASE 

#include "elfio/elfio.hpp"
#include "elfio/elfio_segment.hpp"
#include "elfio/elfio_section.hpp"
#include "elfio/elfio_symbols.hpp"

#include "hv2/disas.hpp"

#include "screen.hpp"

void hv2f_load_elf_to_guest_memory(std::string name, hv2_t* cpu, dev_ram_t* ram, uint32_t phys_ram_base) {
    ELFIO::elfio reader;

    reader.load(name);

    uint32_t phys = 0;

    std::vector <uint8_t>* buf = ram->get_buf();

    for (int i = 0; i < reader.segments.size(); i++) {
        hv2_mmu_entry_t me;

        const ELFIO::segment* seg = reader.segments[i];

        me.vaddr = seg->get_virtual_address();
        me.paddr = phys_ram_base + phys;
        me.size  = seg->get_memory_size();
        me.attr  = seg->get_flags();

        uint32_t size_in_file = seg->get_file_size();

        const char* data = seg->get_data();

        for (unsigned t = 0; t < me.size; t++) {
            int idx = t % size_in_file;

            (*buf)[t + phys] = data[idx];
        }

        phys += me.size;

        hv2_mmu_create_mapping(cpu, i, me);
    }

    // Set PC to ELF entry
    cpu->r[31] = reader.get_entry();
}

dev_ram_t* hv2f_attach_memory(hv2_t* cpu, uint32_t base, uint32_t size) {
    dev_ram_t* ram = new dev_ram_t;

    ram->init(base, size);
    
    hv2_mmu_attach_device(cpu, ram);

    return ram;
}

uint32_t hv2f_hrsize_to_bytes(std::string size) {
    uint32_t bytes;

    char unit = size.back();

    bytes = std::stoi(size.substr(0, size.size() - 1));

    switch (unit) {
        case 'G': { bytes <<= 30; } break;
        case 'g': { bytes *= 1000000000; } break;
        case 'M': { bytes <<= 20; } break;
        case 'm': { bytes *= 1000000; } break;
        case 'K': { bytes <<= 10; } break;
        case 'k': { bytes *= 1000; } break;
    }

    return bytes;
}

void hv2f_disassemble_load_symbols(ELFIO::elfio& elf, hv2_disassembler_t* dis) {
    // Load symbols
    for (int i = 0; i < elf.sections.size(); i++) {
        ELFIO::section* sect = elf.sections[i];

        if (sect->get_type() == ELFIO::SHT_SYMTAB) {
            ELFIO::symbol_section_accessor ssa(elf, sect);

            for (ELFIO::Elf_Xword j = 0; j < ssa.get_symbols_num(); j++) {
                std::string name;
                ELFIO::Elf64_Addr value;
                ELFIO::Elf_Xword size;
                unsigned char bind;
                unsigned char type;
                uint16_t section_index;
                unsigned char other;

                ssa.get_symbol(
                    j,
                    name,
                    value,
                    size,
                    bind,
                    type,
                    section_index,
                    other
                );

                if (!name.size())
                    continue;
                
                if (bind != ELFIO::STB_GLOBAL)
                    continue;

                hv2d_register_symbol(dis, name, value, 0);
            }
        }
    }
}

#include <algorithm> 
#include <cctype>
#include <locale>

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

void hv2f_disassemble_elf(std::string name) {
    ELFIO::elfio elf;

    elf.load(name);

    hv2_disassembler_t* dis = hv2d_create();

    hv2f_disassemble_load_symbols(elf, dis);

    // Disassemble sections with SHF_EXECINSTR flag
    for (int i = 0; i < elf.sections.size(); i++) {
        const ELFIO::section* sect = elf.sections[i];

        bool execinstr = sect->get_flags() & ELFIO::SHF_EXECINSTR;

        if (!execinstr)
            continue;
        
        dis->vaddr = sect->get_address();

        std::string name = sect->get_name();

        if (!name.size())
            continue;

        std::printf("\nDisassembly of section %s:\n", name.c_str());

        const char* data = sect->get_data();
        uint32_t* ptr = (uint32_t*)data;

        for (int j = 0; j < sect->get_size(); j += 4) {
            std::string disasm = hv2d_disassemble(dis, *(uint32_t*)&data[j]);

            rtrim(disasm);

            std::cout << disasm << std::endl;
        }
    }
}

#include "cli.hpp"

#undef main

#ifndef OS_INFO
#define OS_INFO "unknown"
#endif
#ifndef BUILD_INFO
#define BUILD_INFO
#endif
#ifndef REP_VERSION
#define REP_VERSION "latest"
#endif
#ifndef REP_COMMIT_HASH
#define REP_COMMIT_HASH
#endif

#define STR1(m) #m
#define STR(m) STR1(m)

static const char* version_text =
#ifdef _WIN32
    "hv2.exe (" STR(OS_INFO) STR(BUILD_INFO) ") " STR(REP_VERSION) "-" STR(REP_COMMIT_HASH) "\n"
#elif __linux__
    "hv2 (" STR(OS_INFO) STR(BUILD_INFO) ") " STR(REP_VERSION) "-" STR(REP_COMMIT_HASH) "\n"
#else
    "hv2 (" STR(OS_INFO) STR(BUILD_INFO) ") " STR(REP_VERSION) "-" STR(REP_COMMIT_HASH) "\n"
#endif
    "Copyright (C) 2023 Allkern/Lycoder (Lisandro Alarcon)\n"
    "This is free software; see the source for copying conditions.  There is NO\n"
    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";

static const char* help_text =
    "Usage: hv2 [options] file\n"
    "Options:\n"
    "  -i, --input <file>        Specify an input file\n"
    "  -v, --version             Display compiler version information\n"
    "  -d, --disassemble         Disassemble input\n"
    "  -H, --help                Display this information text\n"
    "  -M, --memory-size <size><kKmMgG>\n"
    "                            Set guest memory size\n"
    "      --memory-base         Set memory physical address\n"
    "      --stdin               Get input stream from stdin\n"
    "\n"
    "Disassembler options:\n"
    "  -Sm, --mnemonic-size      Set the maximum length for an instruction's\n"
    "                            mnemonic\n"
    "  -So, --operands-size      Set the maximum length for an instruction's\n"
    "                            operands\n"
    "  -Sl, --line-size          Set the maximum length for a disassembly line\n"
    "\n"
    "Options need to be specified individually (i.e. no \"-VvqaL...\") and\n"
    "arguments to options need to be passed leaving a space between the option\n"
    "and argument. e.g. -T x86_64. Comma-separated arguments must be written\n"
    "without spaces between each argument. e.g. -I inc,inc2,inc3\n"
    "\n"
    "For bug reporting please file an issue on:\n"
    "https://github.com/allkern/hv2/issues";

int main(int argc, const char* argv[]) {
    _hv2_log::init("hv2");

    cli::parser_t cli;

    cli.init(argc, argv);
    cli.parse();

    if (cli.get_switch(cli::SW_VERSION)) {
        std::cout << version_text << std::endl;

        return 0;
    }

    if (cli.get_switch(cli::SW_HELP)) {
        std::cout << help_text << std::endl;

        return 0;
    }

    if (cli.get_switch(cli::SW_DISASSEMBLE)) {
        // To-do: disassemble raw files
        hv2f_disassemble_elf(cli.get_setting(cli::ST_INPUT));

        return 0;
    }

    uint32_t memory_base, memory_size;

    if (cli.is_set(cli::ST_MEMORY_BASE)) {
        memory_base = std::stoi(cli.get_setting(cli::ST_MEMORY_BASE));
    } else {
        memory_base = 0x80000000;
    }

    if (cli.is_set(cli::ST_MEMORY_SIZE)) {
        memory_size = hv2f_hrsize_to_bytes(cli.get_setting(cli::ST_MEMORY_SIZE));
    } else {
        memory_size = 0x400000;
    }

    hv2_t* cpu = hv2_create();

    hv2_init(cpu);

    // Make CPU flush pipeline after flow transfers
    cpu->cop0_cr0 |= HV2_COP0_CR0_XFLUSH_ON_FT;

    // Enable MMU
    cpu->cop4_ctrl = 1;

    dev_ram_t* ram = hv2f_attach_memory(cpu, memory_base, memory_size);

    dev_vga_textmode_t vga;

    vga.init("IBM_VGA_8x16.bin", 8, 16);
    
    hv2_mmu_attach_device(cpu, &vga);

    screen_t* screen = screen_create();

    screen_init(screen, "VGA screen", vga.get_screen_width(), vga.get_screen_height());
    
    hv2f_load_elf_to_guest_memory(cli.get_setting(cli::ST_INPUT), cpu, ram, memory_base);

    // Map VGA range to virtual memory
    hv2_mmu_entry_t me;

    me.vaddr = 0xb8000;
    me.paddr = 0xb8000;
    me.size  = 0x8000;
    me.attr  = ELFIO::PF_R | ELFIO::PF_W;

    hv2_mmu_create_mapping(cpu, 20, me);

    while (screen->open) {
        int counter = CPU_SPEED / 60;

        while (counter--) {
            hv2_cycle(cpu);
        }

        vga.render();

        screen_update(screen, vga.get_screen_buf());
    }

    screen_destroy(screen);

    return 0;
}