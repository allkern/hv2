#define HV2_CAUSE_MMU               0x800000
#define HV2_CAUSE_MMU_NOMAP         0x800000
#define HV2_CAUSE_MMU_PROT_READ     0x800001
#define HV2_CAUSE_MMU_PROT_WRITE    0x800002
#define HV2_CAUSE_MMU_PROT_EXEC     0x800003
#define HV2_CAUSE_MMU_XALIGN        0x800004
#define HV2_CAUSE_MMU_RWALIGN       0x800005
#define HV2_CAUSE_CPU               0x400000
#define HV2_CAUSE_SYSCALL           0x400000
#define HV2_CAUSE_DEBUG             0x400001
#define HV2_CAUSE_SEXCEPT           0x400002
#define HV2_CAUSE_TPL0              0x400003
#define HV2_CAUSE_TPL1              0x400004
#define HV2_CAUSE_TPL2              0x400005
#define HV2_CAUSE_TPL3              0x400006
#define HV2_CAUSE_ILLEGAL_INSTR     0x400007
#define HV2_CAUSE_INVALID_COPX      0x400008
#define HV2_CAUSE_INVALID_TPL       0x400009
#define HV2_CAUSE_RESET             0xaa485632

# Addresses
#define BIOS_RAM_BASE               0x80000
#define BIOS_RAM_END                0x90000
#define RAM_BASE                    0x80000000

# BIOS variables
#define BIOS_MEM_SIZE_DISCOVERY     0x0
#define BIOS_MEM_SIZE               0x4

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

    #------------------ CAUSE_MMU_NOMAP --------------#
.I: 
    nop     x0
    nop     x0
    nop     x0
    b I

bios_reset_handler:
    b       bios_boot                   # Pass execution to our boot function

.I: 
    nop     x0
    nop     x0
    nop     x0
    nop     x0
    b I

bios_mmu_nomap_handler:
    li.w    x0, BIOS_RAM_BASE
    load.l  x1, [x0+BIOS_MEM_SIZE_DISCOVERY]
    bne     x1, r0, bios_detect_available_memory_done

    # Report MMU_NOMAP error

.I: 
    nop     x0
    nop     x0
    nop     x0
    nop     x0
    b I

bios_boot:
    li.w    a0, !BIOS_WELCOME_MSG
    call    !vga_print
    b       bios_detect_available_memory

.I: 
    nop     x0
    nop     x0
    nop     x0
    nop     x0
    b I

bios_detect_available_memory:
    li.w    a0, !BIOS_MEMORY_SIZE_MSG
    call    !vga_print
    li.w    x0, BIOS_RAM_BASE
    store.l [x0+BIOS_MEM_SIZE_DISCOVERY], x0
    li.w    x0, RAM_BASE
    xor.u   a0, a0, a0

.loop:
    load.l  r0, [x0+a0]
    add.u   a0, 4
    b       loop

bios_detect_available_memory_done:
    move    x1, a0
    lsr     a0, 10
    bleq    r0, r0, bios_bin_to_bcd
    nop     r0
    nop     r0
    nop     r0
    call    !vga_print_integer
    li.u    a0, 0x4b
    nop     r0
    call    !vga_vt_putchar
    li.w    x0, BIOS_RAM_BASE
    store.l [x0+BIOS_MEM_SIZE], x1

.I: b I
    nop     x0
    nop     x0
    nop     x0

bios_bin_to_bcd:
    move    x10, a0 # num
    li.s    x11, 0  # dig
    li.s    x12, 0  # count
    li.s    x13, 0  # bcd
    li      x14, 8  # n
    li.s    x15, 10 # ten
    li.s    x16, 0  # temp    

.L0:
    mod     x11, x10, x15 # dig = num % 10
    div     x10, x10, x15 # num = num / 10
    lsl     x16, x11, x12 # dig << count
    or      x13, x13, x16 # bcd = (dig << count) | bcd
    add.u   x12, 4
    sub.u   x14, 1
    bne     x14, r0, L0

.R:
    move    a0, x13
    move    pc, lr
    nop     x0
    nop     x0
    nop     x0

#include "bios/vga.s"
#include "bios/data.inc"

BIOS_PAD:
    nop     x0
    nop     x0
    nop     x0
    nop     x0