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
#define HV2_CAUSE_RESET             0xff485632

# Addresses
#define BIOS_RAM_BASE               0x80000
#define BIOS_RAM_END                0x90000
#define RAM_BASE                    0x80000000

# BIOS variables
#define BIOS_MEM_SIZE_DISCOVERY     0x0
#define BIOS_MEM_SIZE               0x4

#define a1 x20
#define a2 x21
#define a3 x22