#include "bios/defs.inc"

bios_reset_handler:
    b       bios_boot                   # Pass execution to our boot function

bios_mmu_nomap_handler:
    li.w    x0, BIOS_RAM_BASE
    load.l  x1, [x0+BIOS_MEM_SIZE_DISCOVERY]
    bne     x1, r0, bios_detect_available_memory_done

    # Report MMU_NOMAP error and lock up
    li.w    a0, !BIOS_MMU_NOMAP_MSG
    call    !vga_print
    b       bios_lock
    nop     x0
    nop     x0

bios_syscall_handler:
    li.w    a0, !BIOS_UNHANDLED_SYSCALL_MSG
    call    !vga_print
    call    !bios_lock