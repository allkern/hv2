#pragma once

#include <fstream>
#include <vector>
#include <cstdio>
#include <string>

#include "hv2/mmu_device.hpp"

class dev_bios_rom_t : public hv2_mmio_device_t {
    std::vector <uint8_t> buf;

    uint32_t base;
    uint32_t size;

public:
    std::vector <uint8_t>* get_buf() {
        return &buf;
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
        return;
    }

    void init(std::string file, uint32_t base) {
        this->base = base;

        std::ifstream rom(file, std::ios::binary | std::ios::ate);

        this->size = rom.tellg();

        buf.resize(this->size);

        rom.seekg(0);
        rom.read((char*)buf.data(), this->size);
    }
};