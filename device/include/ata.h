/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ATA_H
#define _ATA_H
#include <lib/include/types.h>
#include <lib/include/list.h>

#define ATA_PRIMARY 0x1
#define ATA_SECONDARY 0x0

#define ATA_MASTER 0x1
#define ATA_SLAVE 0x0

struct ata_device {
    struct list_elem list;
    uint16_t io_base;
    uint16_t ctrl_base;

    uint8_t primary_bus;
    uint8_t master_drive;
};

/*
 * for more details, please refer to:
 * https://wiki.osdev.org/ATA_PIO_Mode
 */
#define ATA_PRIMARY_BUS_IO_BASE 0x1f0
#define ATA_PRIMARY_BUS_CONTROL_BASE 0x3f6

#define ATA_SECONDARY_BUS_IO_BASE 0x170
#define ATA_SECONDARY_BUS_CONTROL_BASE 0x376

#define DATA_REGISTER_OFFSET 0x0
#define ERROR_REGISTER_OFFSET 0x1
#define FEATURE_REGISTER_OFFSET ERROR_REGISTER_OFFSET
#define SECTOR_COUNTER_REGISTER_OFFSET 0x2
#define SECTOR_NUMBER_REGISTER_OFFSET 0x3
#define LBA_LOW_OFFSET SECTOR_NUMBER_REGISTER_OFFSET
#define CYLINDER_LOW_REGISTER_OFFSET 0x4
#define LBA_MID_OFFSET CYLINDER_LOW_REGISTER_OFFSET
#define CYLINDER_HIGH_REGISTER_OFFSET 0x5
#define LBA_HIGH_OFFSET CYLINDER_HIGH_REGISTER_OFFSET
#define DRIVER_REGISTER_OFFSET 0x6
#define HEAD_REGISTER_OFFSET DRIVER_REGISTER_OFFSET
#define STATUS_REGISTER_OFFSET 0x7
#define COMMAND_REGISTER_OFFSET STATUS_REGISTER_OFFSET


#define STATUS_ERROR 0x01
#define STATUS_INDEX 0x02
#define STATUS_CORRECT 0x04
#define STATUS_DATA_REQUEST 0x08
#define STATUS_SERVICE 0x10
#define STATUS_DRIVE_FAULT 0x20
#define STATUS_READY 0x40
#define STATUS_BUSY 0x80

#define CMD_IDENTITY 0xec

void
ata_init(void);
#endif
