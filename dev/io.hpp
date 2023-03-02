#pragma once

#include <vector>
#include <cstdio>
#include <array>

#include "hv2/mmu_device.hpp"
#include "io_device.hpp"

class dev_io_t : public hv2_mmio_device_t {
    std::vector <io_device_t*> devices;

    uint32_t base;

public:
    hv2_range_t get_physical_range() override {
        return { base, base + 0x10000 };
    }

    uint32_t read(uint32_t addr, int size) override {
        uint32_t port = addr - base;

        for (io_device_t* dev : devices) {
            io_device_port_list_t* pl = dev->get_port_list();

            for (uint16_t dev_port : *pl) {
                if (port == dev_port) return dev->read(port, size);
            }
        }

        return 0xffffffff;
    }

    void write(uint32_t addr, uint32_t value, int size) override {
        uint32_t port = addr - base;

        for (io_device_t* dev : devices) {
            io_device_port_list_t* pl = dev->get_port_list();

            for (uint16_t dev_port : *pl) {
                if (port == dev_port) {
                    dev->write(port, value, size);

                    return;
                }
            }
        }
    }

    void init(uint32_t base) {
        this->base = base;
    }

    void register_device(io_device_t* dev) {
        devices.push_back(dev);
    }
};