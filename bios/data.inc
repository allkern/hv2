BIOS_WELCOME_MSG:
    .asciiz "Welcome to "
BIOS_NAME_MSG:
    .asciiz "HV2 VGABIOS\n\n"

BIOS_COPYRIGHT_MSG:
    .ascii  "Copyright (C) 2023 Allkern\n"
    .ascii  "This is free software; see the source for copying conditions.  There is NO\n"
    .asciiz "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"

BIOS_MEMORY_SIZE_MSG:
    .asciiz "Detecting available memory... "

BIOS_MMU_NOMAP_MSG:
    .asciiz "\nMMU exception occurred (CPU tried accessing unmapped area), system halted"

BIOS_UNKNOWN_EXC_MSG:
    .asciiz "\nAn unknown exception occured, system halted."

BIOS_UNHANDLED_SYSCALL_MSG:
    .asciiz "\nUnhandled syscall, system halted."

BIOS_PCI_ENUMERATE_MSG:
    .asciiz "\nPCI devices:\n"

BIOS_TAB:
    .asciiz "    "

BIOS_DONE_MSG:
    .asciiz "Done"

BIOS_PCI_AT_BUS_MSG:
    .asciiz " bus 0, device "

BIOS_PCI_DEVCLASS_TABLE:
    .asciiz "Unclassified                   "
    .asciiz "Mass storage device            "
    .asciiz "Network controller             "
    .asciiz "Display controller             "
    .asciiz "Multimedia controller          "
    .asciiz "Memory controller              "
    .asciiz "Bridge                         "
    .asciiz "Simple communication controller"
    .asciiz "Base system peripheral         "
    .asciiz "Input device controller        "
    .asciiz "Docking station                "
    .asciiz "Processor                      "
    .asciiz "Serial bus controller          "
    .asciiz "Wireless controller            "
    .asciiz "Intelligent controller         "
    .asciiz "Satellite comm. controller     "

BIOS_PCI_INPUT_DEVICE_TABLE:
    .asciiz "Keyboard controller             "
    .asciiz "Digitizer pen                   "
    .asciiz "Mouse Controller                "
    .asciiz "Scanner Controller              "
    .asciiz "Gameport Controller             "