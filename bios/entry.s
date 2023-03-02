#include "bios/defs.inc"

# The BIOS' entry is also our exception handler
BIOS_ENTRY:
    li.w    sp, BIOS_RAM_END            # Setup stack
    mfcr    x0, cop0_xcause             # Get exception cause

    #------------------ CAUSE_RESET ------------------#
    li.w    x1, HV2_CAUSE_RESET
    beq     x0, x1, bios_reset_handler  # If the system was just reset
                                        # jump to our reset handler
    #------------------ CAUSE_MMU_NOMAP --------------#
    li.w    x1, HV2_CAUSE_MMU_NOMAP
    beq     x0, x1, bios_mmu_nomap_handler

    #------------------ CAUSE_SYSCALL ----------------#
    li.w    x1, HV2_CAUSE_SYSCALL
    beq     x0, x1, bios_syscall_handler

    li.w    a0, !BIOS_UNKNOWN_EXC_MSG
    call    !vga_print
    call    !bios_lock

BIOS_PAD:
    nop     x0
    nop     x0
    nop     x0
    nop     x0