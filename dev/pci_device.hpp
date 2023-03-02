#pragma once

#include <cstdint>

#define PCI_BAR_MEM 0
#define PCI_BAR_IO  1
#define PCI_IO_BAR(port) ((port & 0xfffffffc) | PCI_BAR_IO)
#define PCI_MEM_BAR(addr) ((addr & 0xfffffff0) | PCI_BAR_MEM)

struct pci_desc_t {
    uint32_t devid;
    uint32_t vendor;
    uint32_t status;
    uint32_t command;
    uint32_t devclass;
    uint32_t subclass;
    uint32_t pif;
    uint32_t rev;
    uint32_t bist;
    uint32_t hdr;
    uint32_t lat;
    uint32_t clsize;
    uint32_t bar[6];
};

struct pci_device_t {
    // Don't support multi-function devices yet
    int bus, device;
    bool disabled = false;

    pci_desc_t desc;
};