/*
    ata.hpp - PCI ATA/ATAPI IDE Controller
    Copyright (C) 2022 Lycoder <github.com/lycoder>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "block.hpp"
#include "io_device.hpp"
#include "pci_device.hpp"

#include <string>
#include <cstring>
#include <cstdlib>

#define ATA_PRI_IO   0x1f0
#define ATA_PRI_CTRL 0x3f6
#define ATA_SEC_IO   0x170
#define ATA_SEC_CTRL 0x376

#define ATA_IO_BEGIN   ATA_PRI_IO
#define ATA_IO_SIZE    0xd
#define ATA_CTRL_BEGIN ATA_PRI_CTRL
#define ATA_CTRL_SIZE  0x3

// ATA Registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0a
#define ATA_REG_LBA5       0x0b
#define ATA_REG_CONTROL    0x0c
#define ATA_REG_ALTSTATUS  0x0c
#define ATA_REG_DEVADDRESS 0x0d

// Status Register
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// Error Register
#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

// ATA Commands
#define ATA_CMD_READ_PIO          0x20 // READ SECTORS
#define ATA_CMD_READ_PIO_EXT      0x24 // READ SECTORS EXT
#define ATA_CMD_READ_DMA          0xC8 // READ DMA
#define ATA_CMD_READ_DMA_EXT      0x25 // READ DMA EXT
#define ATA_CMD_WRITE_PIO         0x30 // WRITE SECTORS
#define ATA_CMD_WRITE_PIO_EXT     0x34 // WRITE SECTORS EXT
#define ATA_CMD_WRITE_DMA         0xCA // WRITE DMA
#define ATA_CMD_WRITE_DMA_EXT     0x35 // WRITE DMA EXT
#define ATA_CMD_CACHE_FLUSH       0xE7 // FLUSH CACHE
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA // FLUSH CACHE EXT
#define ATA_CMD_PACKET            0xA0 // PACKET (ATAPI?)
#define ATA_CMD_IDENTIFY_PACKET   0xA1 // IDENTIFY PACKET DEVICE (ATAPI?)
#define ATA_CMD_IDENTIFY          0xEC // IDENTIFY DEVICE

// #IDENTITY fields
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define ATA_PRIMARY     0
#define ATA_SECONDARY   1
#define ATA_MASTER      0
#define ATA_SLAVE       1

#define ATA_SECTOR_SIZE 0x200

struct ata_channel_t {
    int drive_number = ATA_MASTER;

    struct drive_t {
        block_dev_t blk;                        // Each drive gets a block device for outputting to a file
        uint64_t    rw_base_lba;                // This is the base LBA for RW ops
        uint16_t    rw_sectors;                 // Sector count for RW ops
        size_t      rw_pending_bytes;           // Pending RWs from the PIO port
        uint8_t     rw_buf[ATA_SECTOR_SIZE];    // Sector-sized buffer for RW ops
        bool        rw_direction;               // RW op direction (read, write)
        uint8_t     error;                      // Drive command error
        uint8_t     status;                     // Drive status
    } drive[2];
};

#define ATA_PRI_MASTER 0
#define ATA_PRI_SLAVE  1
#define ATA_SEC_MASTER 2
#define ATA_SEC_SLAVE  3

class io_device_ata_t : public io_device_t {
    pci_desc_t desc;

    // Channel index
    int index = ATA_PRIMARY;
    ata_channel_t channel[2];

    uint16_t pri_io_base   = ATA_PRI_IO;
    uint16_t pri_ctrl_base = ATA_PRI_CTRL;
    uint16_t sec_io_base   = ATA_SEC_IO;
    uint16_t sec_ctrl_base = ATA_SEC_CTRL;

#define ATA_ID_CFG_RESERVED1  0b0000000000000001
#define ATA_ID_CFG_UNUSED3    0b0000000000000010
#define ATA_ID_CFG_INCOMPLETE 0b0000000000000100 // 0 - Complete response, 1 - Incomplete response
#define ATA_ID_CFG_UNUSED2    0b0000000000111000
#define ATA_ID_CFG_FIXED      0b0000000001000000 // 0 - Not fixed device, 1 - Fixed device
#define ATA_ID_CFG_REMOVABLE  0b0000000010000000 // 0 - Non-removable device, 1 Removable device
#define ATA_ID_CFG_UNUSED1    0b0111111100000000
#define ATA_ID_CFG_DEVICETYPE 0b1000000000000000 // 0 - ATA device?, 1 - ???

#define CURRENT_CHANNEL channel[index]
#define CURRENT_DRIVE channel[index].drive[channel[index].drive_number]

    void store_identify_buffer() {
        uint16_t id_buf[ATA_SECTOR_SIZE / 2];

        id_buf[0] = ATA_ID_CFG_FIXED; // Fixed, non-removable, ATA device
        id_buf[1] = 65535; // logical cylinders
        id_buf[3] = 16; // logical heads
        id_buf[6] = 63; // sectors per track
        id_buf[22] = 4; // number of bytes available in READ/WRITE LONG cmds
        id_buf[47] = 0; // read-write multipe commands not implemented
        id_buf[49] = (1 << 9); // Capabilities - LBA supported, DMA supported
        id_buf[50] = (1 << 14); // Capabilities - bit 14 needs to be set as required by ATA/ATAPI-5 spec
        id_buf[51] = (4 << 8); // PIO data transfer cycle timing mode
        id_buf[53] = 1 | 2 | 4; // fields 54-58, 64-70 and 88 are valid
        id_buf[54] = 65535; // logical cylinders
        id_buf[55] = 16; // logical heads
        id_buf[56] = 63; // sectors per track
        id_buf[57] = 0xffff;
        id_buf[58] = 0xffff;
        id_buf[60] = 0xffff;
        id_buf[61] = 0xffff;
        id_buf[64] = 0; // advanced PIO modes not supported
        id_buf[67] = 1; // PIO transfer cycle time without flow control
        id_buf[68] = 1; // PIO transfer cycle time with IORDY flow control
        id_buf[80] = 1 << 6; // ATA major version
        id_buf[88] = 0; // UDMA mode 5 not supported

        char serial[20];

        // Generate serial
        for (int i = 0; i < 20; i++)
            serial[i] = "0123456789ABCDEF"[rand() % 16];
        
        serial[20] = 0;
        
        std::memcpy(&id_buf[10], serial, 20);
        std::memcpy(&id_buf[23], "hyvmidec", 8);
        std::memcpy(&id_buf[27], "WDC WD4005FZBX-00K5WB0\0                ", 40);

        std::memcpy(CURRENT_DRIVE.rw_buf, id_buf, ATA_SECTOR_SIZE);

        CURRENT_DRIVE.rw_pending_bytes = ATA_SECTOR_SIZE;
        CURRENT_DRIVE.rw_direction     = false;
    }

#define RW_READ  0
#define RW_WRITE 1

    bool ata_check_access_and_channel(uint32_t port) {
        // Figure out channel from port
        if ((port >= pri_io_base) && (port <= (pri_io_base + ATA_IO_SIZE))) {
            index = ATA_PRIMARY;            

            return true;
        } else if ((port >= sec_io_base) && (port <= (sec_io_base + ATA_IO_SIZE))) {
            index = ATA_SECONDARY;

            return true;
        }

        return false;
    }

    uint32_t data = 0x0;

    void ata_io_handle_hddevsel(bool rw) {
        if (!rw) {
            data = 0xe0 | (CURRENT_CHANNEL.drive_number << 4);

            return;
        } else {
            CURRENT_CHANNEL.drive_number = (data >> 4) & 0x1;

            return;
        }
    }

    void ata_io_handle_command(bool rw) {
        if (rw == RW_READ) {
            data = CURRENT_DRIVE.status;

            return;
        } else {
            switch (data) {
                case ATA_CMD_READ_PIO: {

                } break;

                case ATA_CMD_IDENTIFY: {
                    if (!CURRENT_DRIVE.blk.is_open()) {
                        // No drive here
                        CURRENT_DRIVE.status = 0x0;

                        break;
                    }

                    // Immediately set status to ready and request available
                    CURRENT_DRIVE.status = ATA_SR_DRDY | ATA_SR_DRQ;

                    // Identify an ATA drive through LBA1 and LBA2
                    CURRENT_DRIVE.rw_base_lba = 0x0;
                    //CURRENT_DRIVE.rw_base_lba = 0x0;

                    // Clear error
                    CURRENT_DRIVE.error = 0x0;

                    store_identify_buffer();
                }
            }
        }
    }

    void ata_io_handle_data(bool rw, int size) {
        if (!CURRENT_DRIVE.rw_pending_bytes) return;

        uint32_t data = 0x0;

        switch (rw) {
            case RW_READ: {
                if (CURRENT_DRIVE.rw_direction != RW_READ) {
                    // Attempted reading from ATA_REG_DATA during a write
                    // operation

                    return;
                }
                
                int rw_index = ATA_SECTOR_SIZE - CURRENT_DRIVE.rw_pending_bytes;

                switch (size) {
                    case 0: {
                        data = CURRENT_DRIVE.rw_buf[rw_index];

                        CURRENT_DRIVE.rw_pending_bytes -= 1;
                    } break;

                    case 1: {
                        data = *(uint16_t*)(&CURRENT_DRIVE.rw_buf[rw_index]);

                        CURRENT_DRIVE.rw_pending_bytes -= 2;
                    } break;

                    case 2: default: {
                        data = *(uint32_t*)(&CURRENT_DRIVE.rw_buf[rw_index]);

                        CURRENT_DRIVE.rw_pending_bytes -= 4;
                    }
                }
            } break;

            case RW_WRITE: {
                // To-do
            } break;
        }
    }

public:
    bool attach_drive(const std::string& path, int attachment) {
        switch (attachment) {
            case ATA_PRI_MASTER: return channel[ATA_PRIMARY].drive[ATA_MASTER].blk.open(path, ATA_SECTOR_SIZE);
            case ATA_PRI_SLAVE : return channel[ATA_PRIMARY].drive[ATA_SLAVE].blk.open(path, ATA_SECTOR_SIZE);
            case ATA_SEC_MASTER: return channel[ATA_SECONDARY].drive[ATA_MASTER].blk.open(path, ATA_SECTOR_SIZE);
            case ATA_SEC_SLAVE : return channel[ATA_SECONDARY].drive[ATA_SLAVE].blk.open(path, ATA_SECTOR_SIZE);
            default: return false;
        }

        return false;
    }

    void redefine_ports(
        uint16_t pri_io_base   = ATA_PRI_IO,
        uint16_t pri_ctrl_base = ATA_PRI_CTRL,
        uint16_t sec_io_base   = ATA_SEC_IO,
        uint16_t sec_ctrl_base = ATA_SEC_CTRL) {
        
        pri_io_base   = pri_io_base;
        pri_ctrl_base = pri_ctrl_base;
        sec_io_base   = sec_io_base;
        sec_ctrl_base = sec_ctrl_base;

        ports[0] = pri_io_base;
        ports[1] = pri_ctrl_base;
        ports[2] = sec_io_base;
        ports[3] = sec_ctrl_base;

        // Update PCI BAR data
        desc.bar[0] = PCI_BAR_IO | (pri_io_base   << 2); // Primary Channel IO
        desc.bar[1] = PCI_BAR_IO | (pri_ctrl_base << 2); // Primary Channel CTRL
        desc.bar[2] = PCI_BAR_IO | (sec_io_base   << 2); // Secondary Channel IO
        desc.bar[3] = PCI_BAR_IO | (sec_ctrl_base << 2); // Secondary Channel CTRL
    }

    io_device_port_list_t ports = {
        pri_io_base,
        pri_ctrl_base,
        sec_io_base,
        sec_ctrl_base
    };

    io_device_port_list_t* get_port_list() {
        return &ports;
    }

    pci_desc_t get_pci_desc() {
        return {
            0x0000,
            0x8086,  // Intel Corporation
            0x0000,
            0x00ff,
            0x0001,    // Mass-storage Device
            0x0001,    // IDE Controller
            0x0000,    // PCI Native Interface
            0x0001,    // Rev. 1
            0x0000,
            0x0000,    // Header Type 0
            0x0000,
            0x0000,
            PCI_IO_BAR(pri_io_base  ), // Primary Channel IO
            PCI_IO_BAR(pri_ctrl_base), // Primary Channel CTRL
            PCI_IO_BAR(sec_io_base  ), // Secondary Channel IO
            PCI_IO_BAR(sec_ctrl_base), // Secondary Channel CTRL
            0x00000000,
            0x00000000
        };
    }

    void init() {
        pri_io_base   = ATA_PRI_IO;
        pri_ctrl_base = ATA_PRI_CTRL;
        sec_io_base   = ATA_SEC_IO;
        sec_ctrl_base = ATA_SEC_CTRL;

        // Initialize PCI desc structure
        desc.devid    = 0x0000;
        desc.vendor   = 0x8086;  // Intel Corporation
        desc.status   = 0x00;
        desc.command  = 0xff;
        desc.devclass = 0x01;    // Mass-storage Device
        desc.subclass = 0x01;    // IDE Controller
        desc.pif      = 0x00;    // PCI Native Interface
        desc.rev      = 0x01;    // Rev. 1
        desc.bist     = 0x00;
        desc.hdr      = 0x00;    // Header Type 0
        desc.lat      = 0x00;
        desc.clsize   = 0x00;
        desc.bar[0]   = PCI_BAR_IO | (pri_io_base   << 2); // Primary Channel IO
        desc.bar[1]   = PCI_BAR_IO | (pri_ctrl_base << 2); // Primary Channel CTRL
        desc.bar[2]   = PCI_BAR_IO | (sec_io_base   << 2); // Secondary Channel IO
        desc.bar[3]   = PCI_BAR_IO | (sec_ctrl_base << 2); // Secondary Channel CTRL
    }

    uint32_t read(uint32_t port, int size) override {
        switch (port & 0xf) {
            case ATA_REG_DATA: {
                ata_io_handle_data(RW_READ, size);
            } break;

            case ATA_REG_ERROR: { // for R, ATA_REG_FEATURES for W

            } break;

            case ATA_REG_HDDEVSEL: {
                ata_io_handle_hddevsel(RW_READ);
            } break;

            case ATA_REG_COMMAND: { // for W, ATA_REG_STATUS for R
                ata_io_handle_command(RW_READ);
            } break;
        }

        return data;
    }

    void write(uint32_t port, uint32_t value, int size) override {
        switch (port & 0xf) {
            case ATA_REG_DATA: {
                ata_io_handle_data(RW_WRITE, size);
            } break;

            case ATA_REG_ERROR: { // for R, ATA_REG_FEATURES for W

            } break;

            case ATA_REG_HDDEVSEL: {
                ata_io_handle_hddevsel(RW_WRITE);
            } break;

            case ATA_REG_COMMAND: { // for W, ATA_REG_STATUS for R
                ata_io_handle_command(RW_WRITE);
            } break;
        }
    }
    // ATA has two buses, a "Primary" bus, and a "Secondary" bus
    // supporting up to two drives each, named "Master" and "Slave"
};