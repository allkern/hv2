#include "exception.hpp"
#include "hv2.hpp"

#include <cstring>
#include <cstdio>
#include <cstdlib>

void hv2_exception(hv2_t* cpu, uint32_t cause) {
    printf("\nSoftware exception @ %08x cause=%08x\n", cpu->r[31], cause);

    std::exit(1);
    
    cpu->cop0_xcause = cause;
    cpu->cop0_xpc = cpu->r[31];
    cpu->r[31] = cpu->cop0_xhaddr;

    if ((cpu->cop4_ctrl & HV2_MMU_CTRL_ENABLE) && (cpu->cop4_ctrl & HV2_MMU_CTRL_EXC_SWAP_MAP)) {
        std::memcpy(cpu->mmu_previous_map, cpu->mmu_map, 32 * sizeof(hv2_mmu_entry_t));
        std::memcpy(cpu->mmu_map, cpu->mmu_exception_mode_map, 32 * sizeof(hv2_mmu_entry_t));
    }

    hv2_flush(cpu, 31);
}
