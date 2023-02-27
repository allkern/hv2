#include "mmu.hpp"
#include "hv2.hpp"
#include "exception.hpp"

hv2_mmu_entry_t* hv2_mmu_search_map(hv2_t* cpu, uint32_t vaddr) {
    hv2_t::mmu_map_t map = cpu->mmu_maps[cpu->cop4_i_cmap];

    for (int i = 0; i < 32; i++) {
        uint32_t map_vaddr = map[i].vaddr;
        uint32_t map_size = map[i].size;
    
        if ((vaddr >= map_vaddr) && (vaddr < (map_vaddr + map_size)))
            return &map[i];
    }

    return nullptr;
}

#include <cstdio>

uint32_t hv2_mmu_v2p(hv2_mmu_entry_t* me, uint32_t vaddr) {
    return me->paddr + (vaddr - me->vaddr);
}

hv2_mmio_device_t* hv2_mmu_get_device_at_phys(hv2_t* cpu, uint32_t paddr) {
    for (hv2_mmio_device_t* dev : cpu->mmu_devices) {
        // Get this device's physical memory range
        hv2_range_t range = dev->get_physical_range();

        if (paddr >= range.start && paddr < range.end) {
            return dev;
        }
    }

    return nullptr;
}

/**
 * @brief Translate virtual or physical address to
 *        physical, potentially generating exceptions
 * 
 * @param cpu HV2 core
 * @param addr Virtual or physical address
 * @param size Access size
 * @return Physical address (uint32_t)
 */
uint32_t hv2_mmu_get_phys(hv2_t* cpu, uint32_t addr, int size) {
    uint32_t phys = 0;

    if (cpu->cop4_ctrl & MMU_CTRL_ENABLE) {
        hv2_mmu_entry_t* me = hv2_mmu_search_map(cpu, addr);

        if (!me) {
            hv2_exception(cpu, HV2_CAUSE_MMU_NOMAP);

            return 0x00000000;
        }

        if ((size == HV2_EXEC) && !(me->attr & MMU_ATTR_EXEC)) {
            hv2_exception(cpu, HV2_CAUSE_MMU_PROT_EXEC);
        }

        if (!(me->attr & MMU_ATTR_READ)) {
            hv2_exception(cpu, HV2_CAUSE_MMU_PROT_READ);

            return 0x00000000;
        }

        // Translate virtual to physical
        phys = hv2_mmu_v2p(me, addr);
    } else {
        phys = addr;
    }

    // If address is not 4-byte aligned
    // throw exception
    if (size == HV2_EXEC) {
        if (phys & 0x3)
            hv2_exception(cpu, HV2_CAUSE_MMU_XALIGN);
    } else if (cpu->cop4_ctrl & MMU_CTRL_RWALIGN_EXC) {
        // Address is not 4-byte aligned
        if ((size == HV2_LONG) && (phys & 0x3))
            hv2_exception(cpu, HV2_CAUSE_MMU_RWALIGN);

        // Address is not 2-byte aligned
        if ((size == HV2_SHORT) && (phys & 0x1))
            hv2_exception(cpu, HV2_CAUSE_MMU_RWALIGN);
    }

    return phys;
}

uint32_t hv2_mmu_read(hv2_t* cpu, uint32_t addr, int size) {
    uint32_t phys = hv2_mmu_get_phys(cpu, addr, size);

    hv2_mmio_device_t* dev = hv2_mmu_get_device_at_phys(cpu, phys);

    if (!dev) {
        // Nothing mapped at address (virtual or physical)
        hv2_exception(cpu, HV2_CAUSE_MMU_NOMAP);

        return 0x00000000;
    }
        //std::printf("MMU read virt=%08x, phys=%08x, return=%08x\n", addr, phys, dev->read(phys, size));

    return dev->read(phys, size);
}

void hv2_mmu_write(hv2_t* cpu, uint32_t addr, uint32_t value, int size) {
    // std::printf("MMU write virt=%08x, value=%08x\n", addr, value);

    uint32_t phys = hv2_mmu_get_phys(cpu, addr, size);

    hv2_mmio_device_t* dev = hv2_mmu_get_device_at_phys(cpu, phys);

    if (!dev) {
        // Nothing mapped at address (virtual or physical)
        hv2_exception(cpu, HV2_CAUSE_MMU_NOMAP);

        return;
    }

    dev->write(phys, value, size);
}

void hv2_mmu_attach_device(hv2_t* cpu, hv2_mmio_device_t* dev) {
    cpu->mmu_devices.push_back(dev);
}

void hv2_mmu_create_mapping(hv2_t* cpu, int idx, const hv2_mmu_entry_t& me) {
    cpu->mmu_maps[cpu->cop4_i_cmap][idx] = me;
}