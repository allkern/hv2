#define VGA_RAM_BASE                0xb8000
#define VGA_RAM_END                 0xb9000
#define VGA_BUFFER_WIDTH            80
#define VGA_BUFFER_HEIGHT           25
#define VGA_BUFFER_LAST_LINE        24
#define VGA_LINE_WIDTH              160

# VGA VT variables
#define VGA_VT_X                    0x4
#define VGA_VT_Y                    0x8

#define a1 x20
#define a2 x21
#define a3 x22

# a0 = dst
# a1 = src
# a2 = len
vga_memcpy:
.L0:
    beq     a2, r0, E0
    load.s  x11, [a1]
    store.s [a0], x11
    add.u   a0, 2
    add.u   a1, 2
    sub.u   a2, 2
    b       L0

.E0:
    ret     r0

# a0 = dst
# a1 = value
# a2 = len
vga_memset:
.L0:
    beq     a2, r0, E0
    store.s [a0], a1
    add.u   a0, 2
    sub.u   a2, 2
    b       L0

.E0:
    ret     r0

vga_vt_scroll:
    li.w    x10, VGA_RAM_BASE
    li.w    x11, VGA_BUFFER_LAST_LINE
    move    a1, x11
    mul.u   a1, VGA_BUFFER_WIDTH
    li.w    a2, VGA_BUFFER_WIDTH

.L0:
    beq     x11, r0, E0
    li.u    x12, 1
    sub.u   x12, x11, x12
    move    a0, x12
    mul.u   a0, VGA_BUFFER_WIDTH
    call    !vga_memcpy
    b       L0

.E0:
    li.w    a0, VGA_BUFFER_LAST_LINE
    mul.u   a0, VGA_BUFFER_WIDTH
    li.u    a1, 0x0700
    call    !vga_memset
    ret     r0
    nop     x0
    nop     x0

vga_vt_newline:
    li.w    x10, VGA_RAM_END
    load.l  x11, [x10-VGA_VT_Y]
    li.w    x12, VGA_BUFFER_LAST_LINE
    beq     x11, x12, scroll
    add.u   x11, 1
    store.s [x10-VGA_VT_X], r0
    store.s [x10-VGA_VT_Y], x11
    ret     r0

.scroll:
    call    !vga_vt_scroll
    ret     r0

vga_vt_putchar:
    li.w    x10, VGA_RAM_END
    load.l  x11, [x10-VGA_VT_X]
    load.l  x12, [x10-VGA_VT_Y]
    li.u    x13, VGA_BUFFER_WIDTH
    bleq    x11, x13, overflow
    li.u    x10, 0x0a
    beq     a0, x10, newline
    or.u    a0, 0x0700
    li.w    x10, VGA_RAM_BASE
    mul.u   x12, VGA_LINE_WIDTH
    add     x10, x10, x12
    store.s [x10+x11*2], a0
    li.w    x10, VGA_RAM_END
    load.l  x11, [x10-VGA_VT_X]
    add.u   x11, 1
    store.l [x10-VGA_VT_X], x11
    ret     x0

.overflow:
    call    !vga_vt_newline
    move    pc, lr

.newline:
    call    !vga_vt_newline
    ret     x0

vga_print:
    move    x17, a0

.L0:
    load.b  a0, [x17]
    beq     a0, r0, E0
    call    !vga_vt_putchar
    inc     x17
    b       L0

.E0:
    ret     r0

# get_hex_length:
#     li      x10, 2

# .L0:
#     beq     a0, r0, E0 
#     lsr     a0, 8
#     add.u   x10, 2
#     b       L0

# .E0:
#     move    a0, x10
#     ret     r0

get_hex_length:
.L0:
    beq     a0, r0, E0 
    lsr.u   a0, 4
    inc     x10
    b       L0

.E0:
    move    a0, x10
    beq     a0, r0, ADD1
    ret     r0

.ADD1:
    li.u    a0, 1
    ret     r0

vga_print_integer:
    move    x17, a0
    call    !get_hex_length
    move    x18, a0
    move    x16, x18
    move    x19, x18
    sub.u   x19, 1
    lsl.u   x19, 2
    move    x16, x19
    li.u    x10, 0xf
    lsl.u   x19, x10, x19

.L0:
    move    a0, x17
    and     a0, a0, x19
    lsr.u   a0, a0, x16
    add.u   a0, !VGA_INTEGER_LUT
    load.b  a0, [a0]
    call    !vga_vt_putchar
    lsl.u   x17, 0x4
    sub.u   x18, 0x1
    bne     x18, r0, L0

.R:
    ret     r0
    nop     r0
    nop     r0
    nop     r0

VGA_INTEGER_LUT:
    .db 0x30, 0x31, 0x32, 0x33
    .db 0x34, 0x35, 0x36, 0x37
    .db 0x38, 0x39, 0x41, 0x42
    .db 0x43, 0x44, 0x45, 0x46