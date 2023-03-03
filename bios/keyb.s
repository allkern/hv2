#include "bios/defs.inc"

#define IO_PS2_DATA 0x60
#define IO_PS2_STAT 0x64
#define IO_PS2_COMM 0x64
#define IO_BASE_K   0x40000

# a0 = command
bios_8042_send_command:
    li.u    at, IO_BASE_K
    store.b [at+IO_PS2_COMM], a0
    ret     r0

bios_8042_init:
    li.w    a0, !BIOS_KEYB_INIT_MSG
    call    !vga_print
    li.u    at, IO_BASE_K

    # Disable devices
    li      a0, 0xad
    call    !bios_8042_send_command
    li      a0, 0xa7
    call    !bios_8042_send_command
    
    # Read CCB
    li      a0, 0x20
    call    !bios_8042_send_command
    load.b  x10, [at+IO_PS2_DATA]
    push    x10

    and.u   x10, 0x0020
    beq     x10, r0, single
    b       double

.single:
    li.w    a0, !BIOS_KEYB_SINGLE_CHANNEL
    call    !vga_print
    b       continue0

.double:
    li.w    a0, !BIOS_KEYB_DOUBLE_CHANNEL
    call    !vga_print

.continue0:
    li      a0, 0x0a
    call    !vga_vt_putchar
    
    # Disable IRQs, disable xlat
    li      a0, 0x60
    call    !bios_8042_send_command
    pop     x10
    and.u   x10, 0b10111100
    li.u    at, IO_BASE_K
    store.b [at+IO_PS2_DATA], x10
    li      a0, 0xaa
    call    !bios_8042_send_command
    li.u    at, IO_BASE_K
    load.b  x10, [at+IO_PS2_DATA]
    move    a0, x10
    call    0xdeadc0de

bios_handle01:
    call    !bios_lock
