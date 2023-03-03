#pragma once

#include <vector>
#include <cstdio>

#include "io_device.hpp"
#include "pci_device.hpp"
#include "hv2/exception.hpp"
#include "hv2/hv2.hpp"

#define HV2_CAUSE_KBC 0xd0000000

#define PS2_DATA 0x60
#define PS2_STAT 0x64
#define PS2_COMM 0x64

#define PS2_STAT_OUT_FULL 0b00000001
#define PS2_STAT_INP_FULL 0b00000010
#define PS2_STAT_SYSTEM   0b00000100
#define PS2_STAT_COMMAND  0b00001000
#define PS2_STAT_UNK4     0b00010000
#define PS2_STAT_UNK5     0b00100000
#define PS2_STAT_TIMEOUT  0b01000000
#define PS2_STAT_PARITY   0b10000000

#define PS2_CMD_RRAM0     0x20
#define PS2_CMD_RRAMB     0x21
#define PS2_CMD_RRAME     0x3f
#define PS2_CMD_WRAM0     0x60
#define PS2_CMD_WRAMB     0x61
#define PS2_CMD_WRAME     0x7f
#define PS2_CMD_SPDI      0xa7
#define PS2_CMD_SPEN      0xa8
#define PS2_CMD_SPTEST    0xa9
#define PS2_CMD_CNTEST    0xaa
#define PS2_CMD_FPTEST    0xab
#define PS2_CMD_DIAGD     0xac
#define PS2_CMD_FPDI      0xad
#define PS2_CMD_FPEN      0xae
#define PS2_CMD_RCIP      0xc0
#define PS2_CMD_MI03S47   0xc1
#define PS2_CMD_MI47S47   0xc2
#define PS2_CMD_RCOP      0xd0
#define PS2_CMD_WCOP      0xd1
#define PS2_CMD_WFPO      0xd2
#define PS2_CMD_WSPO      0xd3
#define PS2_CMD_WSPI      0xd4
#define PS2_CMD_SYSRESET  0xfe

#define PS2_CFG_FPIRQ     0b00000001
#define PS2_CFG_SPIRQ     0b00000010
#define PS2_CFG_SYSTEM    0b00000100
#define PS2_CFG_CLEAR3    0b00001000
#define PS2_CFG_FPCLK     0b00010000
#define PS2_CFG_SPCLK     0b00100000
#define PS2_CFG_FPXLAT    0b01000000
#define PS2_CFG_CLEAR7    0b10000000

#define PS2_COP_SYSRESET  0b00000001
#define PS2_COP_A20GATE   0b00000010
#define PS2_COP_SPCLK     0b00000100
#define PS2_COP_SPDATA    0b00001000
#define PS2_COP_FPOUTF    0b00010000
#define PS2_COP_SPOUTF    0b00100000
#define PS2_COP_FPCLK     0b01000000
#define PS2_COP_FPDATA    0b10000000

typedef void (*i8042_kevent_t)(uint32_t);

class io_device_i8042_t : public io_device_t {
    hv2_t* cpu = nullptr;

    // Controller Configuration Byte
    uint8_t cfg = 0;

    // I/O buffers
    uint8_t out = 0, in = 0;

    // Status register
    uint8_t stat = 0;

    // Last executed command
    uint8_t cmd = 0;

    // Controller Output Port
    uint8_t cop = 0;

    // Device I/O
    uint8_t fpo = 0;
    uint8_t spo = 0;
    uint8_t fpi = 0;
    uint8_t spi = 0;
    
    io_device_port_list_t ports = {
        PS2_DATA, PS2_STAT
    };

public:
    pci_desc_t get_pci_desc() {
        return {
            0x00008042, // i8042 PS/2 Keyboard Controller
            0x00008086, // Intel Corporation
            0x00000000, // Status
            0x00000000, // Command
            0x00000009, // Input Device Controller
            0x00000000, // Keyboard Controller
            0x00000000, // PIF
            0x00008042, // Revision
            0x00000000, // BIST-capable
            0x00000000, // Header type 0x0
            0x00000000, // LAT
            0x00000000, // CLSIZE
            // BARs:
            PCI_IO_BAR(PS2_DATA),
            PCI_IO_BAR(PS2_STAT),
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000
        };
    }

    io_device_port_list_t* get_port_list() override {
        return &ports;
    }

#define SET_STATUS(b) { stat |= (b); }
#define CLR_STATUS(b) { stat &= ~(b); }
#define SET_CFG_BIT(b) { cfg |= (b); }
#define CLR_CFG_BIT(b) { cfg &= ~(b); }
#define CFG_BIT_SET(b) (cfg & (b))
#define CFG_BIT_CLR(b) (!(cfg & (b)))
#define STATUS_SET(b) (stat & (b))
#define STATUS_CLR(b) (!(stat & (b)))

    uint32_t read(uint32_t port, int size) override {
        switch (port) {
            case PS2_DATA: {
                CLR_STATUS(PS2_STAT_OUT_FULL)

                uint8_t ret = out;

                out = 0x00;

                return ret;
            } break;

            case PS2_STAT: {
                return stat;
            } break;
        }

        return 0xff;
    }

    void write(uint32_t port, uint32_t value, int size) override {
        switch (port) {
            case PS2_DATA: {
                if (STATUS_CLR(PS2_STAT_INP_FULL)) {
                    in = value;

                    SET_STATUS(PS2_STAT_INP_FULL)
                }

                switch (cmd) {
                    case PS2_CMD_WRAM0: {
                        cfg = in;

                        CLR_STATUS(PS2_STAT_INP_FULL)
                    } break;

                    case PS2_CMD_WCOP: {
                        cop = in;

                        CLR_STATUS(PS2_STAT_INP_FULL)
                    } break;

                    case PS2_CMD_WFPO: {
                        fpo = in;

                        CLR_STATUS(PS2_STAT_INP_FULL)
                    } break;

                    case PS2_CMD_WSPO: {
                        spo = in;

                        CLR_STATUS(PS2_STAT_INP_FULL)
                    } break;

                    case PS2_CMD_WSPI: {
                        spi = in;

                        CLR_STATUS(PS2_STAT_INP_FULL)
                    } break;

                    default: {
                        cmd = 0x00;
                    } break;
                }
            } break;
            
            // Command
            case PS2_STAT: {
                cmd = value & 0xff;

                switch (cmd) {
                    case PS2_CMD_RRAM0: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        out = cfg;
                    } break;

                    // Read internal RAM
                    case 0x21: case 0x22: case 0x23: case 0x24:
                    case 0x25: case 0x26: case 0x27: case 0x28:
                    case 0x29: case 0x2a: case 0x2b: case 0x2c:
                    case 0x2d: case 0x2e: case 0x2f: case 0x30:
                    case 0x31: case 0x32: case 0x33: case 0x34:
                    case 0x35: case 0x36: case 0x37: case 0x38:
                    case 0x39: case 0x3a: case 0x3b: case 0x3c:
                    case 0x3d: case 0x3e: case 0x3f: {
                        SET_STATUS(PS2_STAT_OUT_FULL);

                        out = 0;
                    } break;

                    case PS2_CMD_WRAM0: {
                        // Nothing?
                    } break;

                    // Write to internal RAM
                    case 0x61: case 0x62: case 0x63: case 0x64:
                    case 0x65: case 0x66: case 0x67: case 0x68:
                    case 0x69: case 0x6a: case 0x6b: case 0x6c:
                    case 0x6d: case 0x6e: case 0x6f: case 0x70:
                    case 0x71: case 0x72: case 0x73: case 0x74:
                    case 0x75: case 0x76: case 0x77: case 0x78:
                    case 0x79: case 0x7a: case 0x7b: case 0x7c:
                    case 0x7d: case 0x7e: case 0x7f: {
                        // Nothing
                    } break;

                    case PS2_CMD_SPDI: case PS2_CMD_SPEN: {
                        if (cmd & 1) SET_CFG_BIT(PS2_CFG_SPCLK) else CLR_CFG_BIT(PS2_CFG_SPCLK);
                    } break;

                    case PS2_CMD_SPTEST: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        // Test passed
                        out = 0x00;
                    } break;

                    case PS2_CMD_CNTEST: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        // Test passed
                        out = 0x55;
                    } break;

                    case PS2_CMD_FPTEST: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        // Test passed
                        out = 0x00;
                    } break;

                    case PS2_CMD_DIAGD: {
                        // Unknown
                    } break;

                    case PS2_CMD_FPDI: case PS2_CMD_FPEN: {
                        if (cmd & 1) SET_CFG_BIT(PS2_CFG_FPCLK) else CLR_CFG_BIT(PS2_CFG_FPCLK);
                    } break;

                    case PS2_CMD_RCIP: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        out = 0x00;
                    } break;

                    case PS2_CMD_MI03S47: case PS2_CMD_MI47S47: {
                        stat &= 0x0f;
                    } break;

                    case PS2_CMD_RCOP: {
                        SET_STATUS(PS2_STAT_OUT_FULL)

                        out = cop;
                    } break;

                    case PS2_CMD_WCOP: {

                    } break;

                    case PS2_CMD_WFPO: case PS2_CMD_WSPO: case PS2_CMD_WSPI: {

                    } break;

                    case PS2_CMD_SYSRESET: {
                        hv2_exception(cpu, HV2_CAUSE_RESET);
                    } break;
                }
            } break;
        }
    }

    void keydown(uint32_t kcode) {
        if (CFG_BIT_CLR(PS2_CFG_FPCLK))
            return;

        if (CFG_BIT_SET(PS2_CFG_FPIRQ))
            hv2_exception(cpu, HV2_CAUSE_KBC);
        
        SET_STATUS(PS2_STAT_OUT_FULL)

        out = kcode & 0xff;
    }

    void init(hv2_t* cpu) {
        SET_CFG_BIT(PS2_CFG_FPCLK)
        SET_CFG_BIT(PS2_CFG_FPIRQ)
        SET_CFG_BIT(PS2_CFG_SPCLK)
        CLR_CFG_BIT(PS2_CFG_SPIRQ)

        this->cpu = cpu;
    }
};