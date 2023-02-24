.entry !ENTRY

.org 0x40000

.align 4
.section .text

ENTRY:
    li.w    x0, 0xdeadbeee
    li.w    x1, 0xdeadbeef
    sge     a0, x0, x1
    debug   0xdeadc0de

.align 16
.section .rodata
.align 4
