/*
    pci.hpp - Peripheral Component Interconnect Bus Emulator
    Copyright (C) 2023 Allkern <github.com/allkern>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "io_device.hpp"
#include "pci_device.hpp"

#include <vector>
#include <cstdint>

#define PCI_CFG_ADDR 0xcf8
#define PCI_CFG_DATA 0xcfc

class io_device_pci_t : public io_device_t {
    std::vector <pci_device_t*> devices;

    pci_device_t* search_device(int bus, int device) {
        for (pci_device_t* dev : devices)
            if ((dev->bus == bus) && (dev->device == device))
                return dev;
        
        return nullptr;
    }

    io_device_port_list_t ports = {
        PCI_CFG_ADDR, PCI_CFG_DATA
    };

public:
    uint32_t addr;
    uint8_t bus, device, function, reg;
    bool enable;

    void register_device(pci_device_t* dev, int bus, int device) {
        dev->bus    = bus;
        dev->device = device;

        devices.push_back(dev);
    }

    io_device_port_list_t* get_port_list() override {
        return &ports;
    }

    uint32_t read(uint32_t port, int size) override {
        switch (port) {
            case PCI_CFG_ADDR: {
                return addr;
            } break;

            case PCI_CFG_DATA: {
                reg      = (addr >> 0 ) & 0xfc;
                function = (addr >> 8 ) & 0x7;
                device   = (addr >> 11) & 0x1f;
                bus      = (addr >> 16) & 0xff;
                enable   =  addr & 0x80000000;

                pci_device_t* dev = search_device(bus, device);

                if (!dev)
                    return 0xffffffff;

                switch (reg >> 2) {
                    case 0x0: return (dev->desc.devid    << 16) |
                                     (dev->desc.vendor   << 0 ); break;
                    case 0x1: return (dev->desc.status   << 16); break;
                    case 0x2: return (dev->desc.devclass << 24) |
                                     (dev->desc.subclass << 16) |
                                     (dev->desc.pif      << 8 ) |
                                     (dev->desc.rev      << 0 ); break;
                    case 0x3: return (dev->desc.bist     << 24) |
                                     (dev->desc.hdr      << 16) |
                                     (dev->desc.lat      << 8 ) |
                                     (dev->desc.clsize   << 0 ); break;

                    case 0x4: case 0x5: case 0x6: case 0x7: case 0x8: case 0x9: {
                        return dev->desc.bar[(reg >> 2) - 0x4];
                    } break;
                    default: {
                        return 0xffffffff;
                        // Unsupported PCI register read
                    } break;
                }

                return 0xffffffff;
            } break;
        }

        return 0xffffffff;
    }

    void write(uint32_t port, uint32_t value, int size) override {
        switch (port) {
            case PCI_CFG_ADDR: {
                addr = value;
            } break;

            case PCI_CFG_DATA: {
                reg      = (addr >> 0 ) & 0xfc;
                function = (addr >> 8 ) & 0x7;
                device   = (addr >> 11) & 0x1f;
                bus      = (addr >> 16) & 0xff;
                enable   =  addr & 0x80000000;
                
                pci_device_t* dev = search_device(bus, device);

                if (!dev)
                    return;

                switch (reg >> 2) {
                    case 0x1: {
                        if (!value) dev->disabled = true;

                        //dev->execute_command(value);
                    } break;
                    default: {
                        // Unsupported PCI register write
                    }
                }
            } break;
        }
    }
};