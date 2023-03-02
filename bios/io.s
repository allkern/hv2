#include "bios/defs.inc"

#define IO_BASE 0x40000

bios_inb:
    li      x10, IO_BASE
    load.b  a0, [x10+a0]
    ret     x0

bios_ins:
    li      x10, IO_BASE
    load.s  a0, [x10+a0]
    ret     x0

bios_inl:
    li      x10, IO_BASE
    load.l  a0, [x10+a0]
    ret     x0

bios_outb:
    li      x10, IO_BASE
    store.b [x10+a1], a0
    ret     x0

bios_outs:
    li      x10, IO_BASE
    store.s [x10+a1], a0
    ret     x0

bios_outl:
    li      x10, IO_BASE
    store.l [x10+a1], a0
    ret     x0