#pragma once

#include <cstdint>
#include <vector>
#include <array>

#include "mmu.hpp"

#define HV2_PIPELINE_SIZE 3

// Magic reset number
#define HV2_CAUSE_RESET           0xff130301

#define HV2_CAUSE_CPU             0x400000
#define HV2_CAUSE_SYSCALL         (HV2_CAUSE_CPU | 0)
#define HV2_CAUSE_DEBUG           (HV2_CAUSE_CPU | 1)
#define HV2_CAUSE_SEXCEPT         (HV2_CAUSE_CPU | 2)
#define HV2_CAUSE_TPL0            (HV2_CAUSE_CPU | 3)
#define HV2_CAUSE_TPL1            (HV2_CAUSE_CPU | 4)
#define HV2_CAUSE_TPL2            (HV2_CAUSE_CPU | 5)
#define HV2_CAUSE_TPL3            (HV2_CAUSE_CPU | 6)
#define HV2_CAUSE_ILLEGAL_INSTR   (HV2_CAUSE_CPU | 7)
#define HV2_CAUSE_INVALID_COPX    (HV2_CAUSE_CPU | 8)
#define HV2_CAUSE_INVALID_TPL     (HV2_CAUSE_CPU | 9)

#define HV2_COP0_CR0_XSTACKED_ISR    0x00000001
#define HV2_COP0_CR0_XFLUSH_ON_IRQ   0x00000002
#define HV2_COP0_CR0_XSTALL_ACCESS   0x00000004
#define HV2_COP0_CR0_XFLUSH_ON_FT    0x00000008

struct hv2_t {
    uint32_t r[32] = { 0 };

    uint32_t alu_t0 = 0, alu_t1 = 0;
    
    uint32_t pipeline[3] = { 0x00000000 };

    int pl = 0;

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
    uint32_t cop4_msel = 0;
    uint32_t cop4_i_cmap = 0;

    typedef std::array <hv2_mmu_entry_t, 32> mmu_map_t; 
    typedef std::array <mmu_map_t, 4> mmu_maps_t;

    mmu_maps_t mmu_maps = { 0 };

    // Internal
    int internal_map_idx = 0;
    bool internal_trace = false;
    bool internal_trace_elf = false;
};

hv2_t* hv2_create();
void hv2_privilege_transition(hv2_t*, int);
void hv2_init(hv2_t*);
uint32_t* hv2_get_cop_register(hv2_t*, uint32_t, uint32_t);
void hv2_flush(hv2_t*, uint32_t);
void hv2_execute(hv2_t*);
void hv2_cycle(hv2_t*);
void hv2_reset(hv2_t*);