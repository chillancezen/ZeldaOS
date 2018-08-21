/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _PCI_H
#define _PCI_H

#include <lib/include/types.h>
#include <lib/include/list.h>

struct pci_device;


struct pci_driver {
    uint8_t * name;

    uint16_t pci_vendor_id;
    uint16_t pci_device_id;

    int (*probe)(struct pci_device * dev);
    int (*attach)(struct pci_device * dev);
    int (*detach)(struct pci_device * dev);
};

struct pci_device {
    struct list_elem list;
    
    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t header_type;

    uint8_t irq_line;
    uint8_t irq_pin;
    uint32_t bar[6];
    uint32_t bar_size[6];
    struct pci_driver * driver;
};
void pci_init(void);

uint16_t
read_pci_config_space(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset);

void
write_pci_config_space(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t data);

uint32_t
get_pci_bar(struct pci_device * _device, int32_t bar);
uint32_t
get_pci_bar_size(struct pci_device * _device, uint32_t bar);

#endif
