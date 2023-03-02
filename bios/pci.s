#include "bios/defs.inc"

#define PCI_CFG_ADDR 0xcf8
#define PCI_CFG_DATA 0xcfc

# a0: bus
# a1: slot
# a2: reg
pci_read_cfg_register:
    lsl.u   a0, 16
    lsl.u   a1, 11
    and.u   a2, 0xfc
    or      a1, a1, a2
    or      a0, a0, a1
    li      a1, PCI_CFG_ADDR
    call    !bios_outl
    li      a0, PCI_CFG_DATA
    call    !bios_inl
    ret     r0
    nop     r0
    nop     r0
    nop     r0
    nop     r0