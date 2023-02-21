#pragma once

#include <vector>
#include <cstdio>

#include "hv2/mmu_device.hpp"

class dev_ram_t : public hv2_mmio_device_t {
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
        
        // if (addr >= 0x80080000)
        //     std::printf("RAM read addr=%08x (%08x), value=%08x, size=%u\n", addr, addr - base, v, size);

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

    void init(uint32_t base, uint32_t size) {
        this->base = base;
        this->size = size;

        buf.resize(size);
    }
};