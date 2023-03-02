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
    move    at, lr
    sub.u   at, 0x8
    move    pc, at
    nop     x0
    nop     x0
    nop     x0

bios_lock:
    nop     x0
    nop     x0
    b       bios_lock
    nop     x0
    nop     x0