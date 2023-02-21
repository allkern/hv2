#pragma once

#include <cstdint>
#include <vector>

#include "mmu.hpp"

#define HV2_PIPELINE_SIZE 3

#define HV2_CAUSE_CPU             0x400000
#define HV2_CAUSE_SYSCALL         (HV2_CAUSE_CPU | 0)
#define HV2_CAUSE_DEBUG           (HV2_CAUSE_CPU | 1)
#define HV2_CAUSE_SEXCEPT         (HV2_CAUSE_CPU | 2)
#define HV2_CAUSE_ILLEGAL_INSTR   (HV2_CAUSE_CPU | 3)
#define HV2_CAUSE_INVALID_COPX    (HV2_CAUSE_CPU | 4)

#define HV2_COP0_CR0_XSTACKED_ISR    0x00000001
#define HV2_COP0_CR0_XFLUSH_ON_IRQ   0x00000002
#define HV2_COP0_CR0_XSTALL_ACCESS   0x00000004
#define HV2_COP0_CR0_XFLUSH_ON_FT    0x00000008

struct hv2_t {
    uint32_t r[32] = { 0 };

    uint32_t alu_t0 = 0, alu_t1 = 0;
    
    uint32_t pipeline[3] = { 0x00000000 };

    int cycle = 0;

    bool flush_pending = false;

    // COP0
    uint32_t cop0_cr0 = 0;
    uint32_t cop0_cr1 = 0;
    uint32_t cop0_xpc = 0;
    uint32_t cop0_xcause = 0;
    uint32_t cop0_xhaddr = 0;

    // COP4 (MMU)
    std::vector <hv2_mmio_device_t*> mmu_devices;

    // MMU Disabled on startup
    uint32_t cop4_ctrl = 0;

    hv2_mmu_entry_t mmu_map[32] = { 0 };
    hv2_mmu_entry_t mmu_previous_map[32] = { 0 };
    hv2_mmu_entry_t mmu_exception_mode_map[32] = { 0 };
};

hv2_t* hv2_create();
void hv2_init(hv2_t*);
uint32_t* hv2_get_cop_register(hv2_t*, uint32_t, uint32_t);
void hv2_flush(hv2_t*, uint32_t);
void hv2_execute(hv2_t*);
void hv2_cycle(hv2_t*);