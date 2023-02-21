#pragma once

#include <fstream>
#include <vector>
#include <cstdio>
#include <string>

#include "hv2/mmu_device.hpp"

#define WIDTH 80
#define HEIGHT 25
#define CELL_SIZE sizeof(uint16_t)

static const uint32_t vga_palette_rgba[] = {
    0x000000ff, 0x0000aaff, 0x00aa00ff, 0x00aaaaff,
    0xaa0000ff, 0xaa00aaff, 0xaa5500ff, 0xaaaaaaff,
    0x555555ff, 0x5555ffff, 0x55ff55ff, 0x55ffffff,
    0xff5555ff, 0xff55ffff, 0xffff55ff, 0xffffffff
};

class dev_vga_textmode_t : public hv2_mmio_device_t {
    std::vector <uint8_t> buf;
    std::vector <uint32_t> screen_buf;
    std::vector <uint8_t> rom;

    int char_width = 8;
    int char_height = 16;

    uint32_t base = 0xb8000;
    uint32_t size = 0x8000;

public:
    void load_rom(std::string name, int char_width, int char_height) {
        std::ifstream file(name, std::ios::binary | std::ios::ate);

        int size = file.tellg();

        file.clear();
        file.seekg(0);

        rom.resize(size);

        file.read((char*)rom.data(), size);
    }

    uint32_t* get_screen_buf() {
        return screen_buf.data();
    }

    int get_screen_width() {
        return WIDTH * char_width;
    }

    int get_screen_height() {
        return HEIGHT * char_height;
    }

    uint8_t* get_buf() {
        return buf.data();
    }

    hv2_range_t get_physical_range() override {
        return { base, base + size };
    }

    uint32_t read(uint32_t addr, int size) override {
        uint32_t v = *(uint32_t*)&buf[addr - base];
        
        if (size == HV2_EXEC)
            return v;

        int bytes = 1 << size;

        v &= 0xffffffff >> (32 - (bytes * 8));

        return v;
    }

    void write(uint32_t addr, uint32_t value, int size) override {
        //std::printf("RAM write addr=%08x (%08x), value=%08x, size=%u\n", addr, addr - base, value, size);
        switch (size) {
            case HV2_BYTE: { buf[addr - base] = value; } break;
            case HV2_SHORT: { *(uint16_t*)&buf[addr - base] = value; } break;

            // HV2_LONG, HV2_EXEC
            default: { *(uint32_t*)&buf[addr - base] = value; } break;
        }
    }

    void render() {
        for (int y = 0; y < HEIGHT * char_height; y++) {
            for (int x = 0; x < WIDTH * char_width; x++) {
                int cx = x / char_width;
                int cy = y / char_height;

                uint32_t off = (cx * sizeof(uint16_t)) + (cy * sizeof(uint16_t) * WIDTH);

                uint16_t data = *(uint16_t*)&buf[off];

                uint32_t rom_off = ((data & 0xff) * char_height) + (y % char_height);

                int b = x % char_width;

                int fgi = (data >> 8) & 0xf;
                int bgi = (data >> 12) & 0x7;

                uint32_t screen_buf_off = x + (y * WIDTH * char_width);
                bool fg = (rom[rom_off] << b) & 0x80;

                screen_buf[screen_buf_off] = fg ? vga_palette_rgba[fgi] : vga_palette_rgba[bgi];
            }
        }
    }

    void init_screen_buf() {
        screen_buf.resize((WIDTH * char_width) * (HEIGHT * char_height));
    }

    void init(std::string name, int char_width, int char_height) {
        buf.resize(size);

        this->char_width = char_width;
        this->char_height = char_height;

        load_rom(name, char_width, char_height);

        init_screen_buf();
    }
};