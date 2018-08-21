/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/printk.h>
#include <device/include/ata.h>
#include <x86/include/ioport.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>

static struct list_elem ata_device_list;

char * device_type_str[] = {
    "UNDEFINED_DEVICE_TYPE",
    "PATA_DEVICE",
    "SATA_DEVICE",
    "PATAPI_DEVICE",
    "SATAPI_DEVICE"

};
void
dump_ata_devices(void)
{
    struct list_elem * _list;
    struct ata_device * _device;
    LOG_INFO("Dump ATA family devices:\n");
    LIST_FOREACH_START(&ata_device_list, _list) {
        _device = CONTAINER_OF(_list, struct ata_device, list);
        LOG_INFO("   %s on %s bus as %s drive: io_base:0x%x, ctrl_base:0x%x\n",
            device_type_str[_device->type],
            _device->bus == ATA_PRIMARY ? "primary" : "secondary",
            _device->drive == ATA_MASTER ? "master" : "slave",
            _device->io_base,
            _device->ctrl_base);
    }
    LIST_FOREACH_END();
}
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
    uint8_t mid;
    uint8_t high;
    uint8_t device_type = UNDEFINED_DEVICE_TYPE;
    struct ata_device * _device;
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
    if (status == 0xff) {
        LOG_WARN("Identifing %s bus %s drive fails: Floating Bus\n",
            bus == ATA_PRIMARY? "primary" : "secondary",
            drive == ATA_MASTER? "master" : "slave");
        return;
    } else if (status == 0x00) {
        LOG_WARN("Identifing %s bus %s drive fails: Drive Not Present\n",
            bus == ATA_PRIMARY? "primary" : "secondary",
            drive == ATA_MASTER? "master" : "slave");
        return;
    }
    while((status = inb(port + STATUS_REGISTER_OFFSET)) & STATUS_BUSY);
    mid = inb(port + LBA_MID_OFFSET);
    high = inb(port + LBA_HIGH_OFFSET);
    if (mid == 0x14 && high == 0xeb)
        device_type = PATAPI_DEVICE;
    else if (mid == 0x69 && high == 0x96)
        device_type = SATAPI_DEVICE;
    else if (mid == 0x3c && high == 0xc3)
        device_type =SATA_DEVICE;
    else if (mid == 0x0 && high == 0x0)
        device_type = PATA_DEVICE;
    LOG_DEBUG("Indentifying %s bus %s drive as: %s\n",
        bus == ATA_PRIMARY? "primary" : "secondary",
        drive == ATA_MASTER? "master" : "slave",
        device_type_str[device_type]);
    /*
     * make an ata_device descriptor
     */
    _device = malloc(sizeof(struct ata_device));
    memset(_device, 0x0, sizeof(struct ata_device));
    _device->io_base = port;
    _device->ctrl_base = bus == ATA_PRIMARY?
        ATA_PRIMARY_BUS_CONTROL_BASE:
        ATA_SECONDARY_BUS_CONTROL_BASE;
    _device->bus = bus;
    _device->drive = drive;
    _device->type = device_type;
    list_append(&ata_device_list, &_device->list);
}

void
reset_ata_device(struct ata_device * _device)
{
    outb(_device->ctrl_base, CMD_RESET);
    ata_delay(_device->bus);
    outb(_device->ctrl_base, 0x0);
    ata_delay(_device->bus);
}

void
disable_interrupt(struct ata_device * _device)
{
    outb(_device->ctrl_base, CMD_DISABLE_INTERRUPT);
    ata_delay(_device->bus);
}

/*
 * CAVEAT: This will clean any state of the drive
 * make sure the SRST and HOB will not be influenced.
 */
void
enable_interrupt(struct ata_device * _device)
{
    outb(_device->ctrl_base, 0);
    ata_delay(_device->bus);
}

void
ata_init(void)
{
    list_init(&ata_device_list);
    identify_drive(ATA_PRIMARY, ATA_MASTER);
    identify_drive(ATA_PRIMARY, ATA_SLAVE);
    identify_drive(ATA_SECONDARY, ATA_MASTER);
    identify_drive(ATA_SECONDARY, ATA_SLAVE);
    dump_ata_devices();
}
