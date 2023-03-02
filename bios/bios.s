#include "bios/defs.inc"
#include "bios/entry.s"
#include "bios/util.s"
#include "bios/io.s"
#include "bios/handler.s"
#include "bios/pci.s"
#include "bios/vga.s"

bios_boot:
    # Set initial color
    li      a0, 0x17
    call    !vga_set_color
    call    !vga_set_color

    # Clear screen and VT state
    call    !vga_vt_clear

    # Print welcome message
    li.w    a0, !BIOS_WELCOME_MSG
    call    !vga_print

    # Set color to red
    li      a0, 0x1c
    call    !vga_set_color

    # Print BIOS name
    li.w    a0, !BIOS_NAME_MSG
    call    !vga_print

    call    !vga_restore_color

    # Print copyright information
    li.w    a0, !BIOS_COPYRIGHT_MSG
    call    !vga_print
    call    !vga_restore_color

    # Start detecting memory
    b       bios_detect_available_memory
    nop     x0
    nop     x0

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
    li.w    x0, BIOS_RAM_BASE
    store.l [x0+BIOS_MEM_SIZE_DISCOVERY], r0
    move    x1, a0
    lsr     a0, 10
    bleq    r0, r0, bios_bin_to_bcd
    move    x1, a0
    li      a0, 0x1f
    call    !vga_set_color
    move    a0, x1
    call    !vga_print_integer
    li.u    a0, 0x4b
    call    !vga_vt_putchar
    call    !vga_restore_color
    li.u    a0, 0x0a
    call    !vga_vt_putchar
    li.w    x0, BIOS_RAM_BASE
    store.l [x0+BIOS_MEM_SIZE], x1

bios_enumerate_pci_devices:
    li.w    a0, !BIOS_PCI_ENUMERATE_MSG
    call    !vga_print
    move    a1, r0
    li      x1, 32
    move    x2, r0
    xor.s   x2, 0xffff

.L0:
    beq     x1, r0, E0
    sub.u   x1, 1
    move    a0, r0
    move    a1, x1
    move    a2, r0
    call    !pci_read_cfg_register
    beq     a0, x2, nodevice

.device:
    move    x3, a0
    li.w    a0, !BIOS_TAB
    call    !vga_print
    move    a0, r0
    move    a1, x1
    li      a2, 0x8
    call    !pci_read_cfg_register
    lsr.u   a0, 24
    li.w    x3, !BIOS_PCI_DEVCLASS_TABLE
    lea.l   a0, [x3+a0:5]
    call    !vga_print
    li      a0, 0x3a
    call    !vga_vt_putchar

    li.w    a0, !BIOS_PCI_AT_BUS_MSG
    call    !vga_print
    move    a0, x1
    add.u   a0, 0x30
    call    !vga_vt_putchar
    li      a0, 0x0a
    call    !vga_vt_putchar

.nodevice:
    b       L0

.E0:
    call    !bios_lock

#include "bios/data.inc"