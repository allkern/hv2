#pragma once

#include <cstdint>

#include "mmu_device.hpp"

#define HV2_CAUSE_MMU             0x800000
#define HV2_CAUSE_MMU_NOMAP       (HV2_CAUSE_MMU | 0)
#define HV2_CAUSE_MMU_PROT_READ   (HV2_CAUSE_MMU | 1)
#define HV2_CAUSE_MMU_PROT_WRITE  (HV2_CAUSE_MMU | 2)
#define HV2_CAUSE_MMU_PROT_EXEC   (HV2_CAUSE_MMU | 3)
#define HV2_CAUSE_MMU_XALIGN      (HV2_CAUSE_MMU | 4)
#define HV2_CAUSE_MMU_RWALIGN     (HV2_CAUSE_MMU | 5)

#define MMU_ATTR_EXEC  1
#define MMU_ATTR_WRITE 2
#define MMU_ATTR_READ  4

#define MMU_CTRL_ENABLE          0x00000001
#define MMU_CTRL_ENABLE_ON_TPL1  0x00001000
#define MMU_CTRL_DISABLE_ON_TPL0 0x00002000
#define MMU_CTRL_REMAP_ON_PLT    0x00004000
#define MMU_CTRL_RWALIGN_EXC     0x00000800

struct hv2_mmu_entry_t {
    uint32_t paddr;
    uint32_t vaddr;
    uint32_t size;
    uint32_t attr;
};

struct hv2_t;

uint32_t hv2_mmu_read(hv2_t*, uint32_t, int);
void hv2_mmu_write(hv2_t*, uint32_t, uint32_t, int);
hv2_mmu_entry_t* hv2_mmu_search_map(hv2_t*, uint32_t);
uint32_t hv2_mmu_v2p(hv2_mmu_entry_t*, uint32_t);
hv2_mmio_device_t* hv2_mmu_get_device_at_phys(hv2_t*, uint32_t);
uint32_t hv2_mmu_get_phys(hv2_t*, uint32_t, int);
void hv2_mmu_attach_device(hv2_t*, hv2_mmio_device_t*);
void hv2_mmu_create_mapping(hv2_t*, int, const hv2_mmu_entry_t&);