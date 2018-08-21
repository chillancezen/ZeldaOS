/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/printk.h>
#include <device/include/ata.h>
#include <x86/include/ioport.h>

void
ata_delay(uint8_t bus)
{
    uint16_t port = bus == ATA_MASTER?
        ATA_PRIMARY_BUS_IO_BASE:
        ATA_SECONDARY_BUS_IO_BASE;
    int idx = 0;
    port += STATUS_REGISTER_OFFSET;
    for(idx = 0; idx < 4; idx++)
        inb(port);
}


int32_t
select_drive(uint8_t bus, uint8_t drive)
{
    uint8_t drive_byte = drive == ATA_MASTER ? 0xa0 : 0xb0;
    uint16_t port = bus == ATA_PRIMARY ?
        ATA_PRIMARY_BUS_IO_BASE:
        ATA_SECONDARY_BUS_IO_BASE;
    port += DRIVER_REGISTER_OFFSET;
    ASSERT(bus == ATA_PRIMARY || bus == ATA_SECONDARY);
    ASSERT(drive == ATA_MASTER || drive == ATA_SLAVE);
    outb(port, drive_byte);
    return OK;
}
void
identify_drive(uint8_t bus, uint8_t drive)
{
    uint16_t port = bus == ATA_PRIMARY ?
        ATA_PRIMARY_BUS_IO_BASE:
        ATA_SECONDARY_BUS_IO_BASE;
    uint8_t status;
    ASSERT(bus == ATA_PRIMARY || bus == ATA_SECONDARY);
    ASSERT(drive == ATA_MASTER || drive == ATA_SLAVE);
    select_drive(bus, drive);
    ata_delay(bus);
    outb(port + SECTOR_COUNTER_REGISTER_OFFSET, 0);
    outb(port + LBA_LOW_OFFSET, 0);
    outb(port + LBA_MID_OFFSET, 0);
    outb(port + LBA_HIGH_OFFSET, 0);
    outb(port + COMMAND_REGISTER_OFFSET, CMD_IDENTITY);
    status = inb(port + STATUS_REGISTER_OFFSET);
    printk("status:%x\n", status);
}
void
ata_init(void)
{
    select_drive(ATA_PRIMARY, ATA_MASTER);
    ata_delay(ATA_PRIMARY);
    identify_drive(ATA_PRIMARY, ATA_MASTER);
    identify_drive(ATA_PRIMARY, ATA_SLAVE);
    identify_drive(ATA_SECONDARY, ATA_MASTER);
    identify_drive(ATA_SECONDARY, ATA_SLAVE);
}
