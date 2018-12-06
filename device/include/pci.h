/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PCI_H
#define _PCI_H

#include <lib/include/types.h>
#include <lib/include/list.h>

struct pci_device_capability_space {
    uint8_t cap_id;
    uint8_t cap_next;
    uint8_t cap_len;
    uint8_t dummy0;
}__attribute__((packed));
struct pci_device;

struct pci_driver {
    struct list_elem list;
    char * name;

    // fields to identify the pci device
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class;
    uint8_t subclass;

    int (*attach)(struct pci_device * dev);
    int (*detach)(struct pci_device * dev);
};

struct pci_device {
    struct list_elem list;
    
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    // here we cache all BARs
    uint32_t bar[6];
    uint32_t bar_size[6];
    uint32_t bar_ioaddr[6];
    // if the driver is set, it should not be associated with a driver again
    // until it's detached. 
    struct pci_driver * driver;

    void * priv;
};

#define FOREACH_CAPABILITY_START(pdev, pcap, cap_offset) {\
    uint16_t __device_status = pci_device_status((pdev)); \
    uint8_t __cap_offset = pci_device_capabilities_pointer((pdev)); \
    uint32_t __capability_dword = 0; \
    struct pci_device_capability_space * __pcap = NULL; \
    if (__device_status & 0x10) { \
        for (; __cap_offset;) { \
            __capability_dword = pci_config_read_dword((pdev)->bus, \
                (pdev)->device, \
                (pdev)->function, \
                __cap_offset); \
            __pcap = (struct pci_device_capability_space *)&__capability_dword; \
            (pcap) = __pcap; \
            (cap_offset) = __cap_offset; \
            __cap_offset = __pcap->cap_next; \
            {

#define FOREACH_CAPABILITY_END() }}}}

void pci_init(void);
void pci_post_init(void);
void
register_pci_device_driver(struct pci_driver * driver);

#define PCI_DEVICE_VENDOR_ID_OFFSET         0x0
#define PCI_DEVICE_DEVICE_ID_OFFSET         0x2
#define PCI_DEVICE_COMMAND_OFFSET           0x4
#define PCI_DEVICE_STATUS_OFFSET            0x6
#define PCI_DEVICE_REVISION_ID_OFFSET       0x8
#define PCI_DEVICE_PROGRAM_IF_OFFSET        0x9
#define PCI_DEVICE_SUBCLASS_OFFSET          0xa
#define PCI_DEVICE_CLASS_OFFSET             0xb
#define PCI_DEVICE_CACHELINE_SIZE_OFFSET    0xc
#define PCI_DEVICE_HEADER_TYPE_OFFSET       0xe
#define PCI_DEVICE_BIST_OFFSET              0xf
#define PCI_DEVICE_BAR0_OFFSET              0x10
#define PCI_DEVICE_BAR1_OFFSET              0x14
#define PCI_DEVICE_BAR2_OFFSET              0x18
#define PCI_DEVICE_BAR3_OFFSET              0x1c
#define PCI_DEVICE_BAR4_OFFSET              0x20
#define PCI_DEVICE_BAR5_OFFSET              0x24
#define PCI_DEVICE_SUB_VENDOR_ID_OFFSET     0x2c
#define PCI_DEVICE_SUBSYSTEM_ID_OFFSET      0x2e
#define PCI_DEVICE_CAPABILITIES_OFFSET      0x34
#define PCI_DEVICE_INTERRUPT_LINE_OFFSET    0x3c
#define PCI_DEVICE_INTERRUPT_PIN_OFFSET     0x3d
uint32_t
pci_config_read_dword(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset);

uint16_t
pci_config_read_word(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset);

uint8_t
pci_config_read_byte(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset);

void
pci_config_write_dword(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t data);

static inline uint16_t
pci_device_vendor_id(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_VENDOR_ID_OFFSET);
}
static inline uint16_t
pci_device_device_id(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_DEVICE_ID_OFFSET);
}

static inline uint16_t
pci_device_command(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_COMMAND_OFFSET);
}

static inline uint16_t
pci_device_status(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_STATUS_OFFSET);
}

static inline uint8_t
pci_device_revision_id(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_REVISION_ID_OFFSET);
}

static inline uint8_t
pci_device_prog_if(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_PROGRAM_IF_OFFSET);
}
static inline uint8_t
pci_device_subclass(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_SUBCLASS_OFFSET);
}

static inline uint8_t
pci_device_class(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_CLASS_OFFSET);
}

static inline uint8_t
pci_device_header_type(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_HEADER_TYPE_OFFSET);
}

static inline uint16_t
pci_device_subsystem_vendor_id(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_SUB_VENDOR_ID_OFFSET);
}
static inline uint16_t
pci_device_subsystem_id(struct pci_device * pdev)
{
    return pci_config_read_word(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_SUBSYSTEM_ID_OFFSET);
}

static inline uint8_t
pci_device_capabilities_pointer(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_CAPABILITIES_OFFSET);
}

static inline uint32_t
pci_device_bar(struct pci_device * pdev, int bar)
{
    if (bar < 0 || bar > 5)
        return 0;
    return pci_config_read_dword(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_BAR0_OFFSET + 4 * bar);
}
static inline uint32_t
pci_device_bar_size(struct pci_device * pdev, int bar)
{
    uint32_t bar_size;
    uint32_t original_bar_data;
    if (bar < 0 || bar > 5)
        return 0;
    original_bar_data = pci_config_read_dword(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_BAR0_OFFSET + 4 * bar);
    if (!original_bar_data)
        return 0;
    pci_config_write_dword(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_BAR0_OFFSET + 4 * bar,
        0xffffffff);
    bar_size = pci_config_read_dword(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_BAR0_OFFSET + 4 * bar);
    bar_size &= bar_size & 0x1 ? 0xfffffffc : 0xfffffff0;
    bar_size = ~bar_size;
    bar_size += 1;
    pci_config_write_dword(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_BAR0_OFFSET + 4 * bar,
        original_bar_data);
    return bar_size;
}
static inline uint8_t
pci_device_interrupt_pin(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_INTERRUPT_PIN_OFFSET);
}
static inline uint8_t
pci_device_interrupt_line(struct pci_device * pdev)
{
    return pci_config_read_byte(pdev->bus,
        pdev->device,
        pdev->function,
        PCI_DEVICE_INTERRUPT_LINE_OFFSET);
}
#endif
