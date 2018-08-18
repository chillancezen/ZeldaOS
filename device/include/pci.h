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
    uint8_t header_type;

    uint8_t irq_line;
    uint8_t irq_pin;

    struct pci_driver * driver;
};
void pci_init(void);

#endif
