/*
 * Copyright (c) 2018 Jie Zheng
 * https://wiki.osdev.org/PCI#Memory_Mapped_PCI_Configuration_Space_Access
 */

#include <device/include/pci.h>
#include <x86/include/ioport.h>
#include <kernel/include/printk.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>

#define PCI_OCNFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc


struct list_elem pci_device_list;


uint16_t
read_pci_config_space(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset)
{
    uint32_t addr = 0;
    uint32_t data = 0;
    addr |= bus << 16;
    addr |= (device & 0x0f) << 11;
    addr |= (function & 0x7) << 8;
    addr |= offset & 0xfc;
    addr |= 0x80000000;
    outl(PCI_OCNFIG_ADDRESS, addr);
    data = inl(PCI_CONFIG_DATA);
    return (data >> ((offset & 2) * 8)) & 0xffff;
}
void write_pci_config_space(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t data)
{
    uint32_t addr = 0;
    addr |= bus << 16;
    addr |= (device & 0x0f) << 11;
    addr |= (function & 0x7) << 8;
    addr |= offset & 0xfc;
    addr |= 0x80000000;
    outl(PCI_OCNFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, data);
}

uint32_t
get_pci_bar(struct pci_device * _device, int32_t bar)
{
    uint8_t offset = 0x10 + bar * 4;
    uint16_t hi;
    uint16_t lo;
    if(bar < 0 || bar >= 6)
        return -1;
    lo = read_pci_config_space(_device->bus,
        _device->device,
        _device->function,
        offset);
    hi = read_pci_config_space(_device->bus,
        _device->device,
        _device->function,
        offset + 2);
    return hi << 16 | lo; 
}
uint32_t
get_pci_bar_size(struct pci_device * _device, uint32_t bar)
{
    uint8_t offset = 0x10 +bar * 4 ;
    uint32_t bar_origin = get_pci_bar(_device, bar);
    uint32_t size = 0;
    if (bar < 0 || bar >= 6)
        return -1;
    write_pci_config_space(_device->bus,
        _device->device,
        _device->function,
        offset,
        0xffffffff);
    size = get_pci_bar(_device, bar);
    size = ~size;
    size += 1;
    write_pci_config_space(_device->bus,
        _device->device,
        _device->function,
        offset,
        bar_origin);
    return size;
}
uint16_t
get_pci_device_vendor_id(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    return read_pci_config_space(bus, device, function, 0x0);
}

uint16_t
get_pci_device_device_id(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    return read_pci_config_space(bus, device, function, 0x2);
}

uint8_t
get_pci_device_class(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    uint16_t word =  read_pci_config_space(bus, device, function, 0xa);
    return (word >> 8) & 0xff;
}

uint8_t
get_pci_device_subclass(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    uint16_t word =  read_pci_config_space(bus, device, function, 0xa);
    return word & 0xff;
}

uint8_t
get_pci_device_header_type(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    uint16_t word =  read_pci_config_space(bus, device, function, 0xe);
    return word & 0xff;
}
uint16_t
get_pci_device_interrupt(uint8_t bus,
    uint8_t device,
    uint8_t function)
{
    return read_pci_config_space(bus, device, function, 0x3c);
}
void
enumerate_pci_devices(void)
{
    int bar;
    int bus;
    int device;
    int function;
    uint8_t class;
    uint8_t subclass;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t header_type;
    uint16_t interrupt;
    struct pci_device * _device;
    for(bus = 0; bus < 256; bus++) {
        for(device = 0; device < 32; device++) {
            for(function = 0; function < 8; function++) {
                vendor_id = get_pci_device_vendor_id(bus, device, function);
                if (vendor_id == 0xffff)
                    continue;
                device_id = get_pci_device_device_id(bus, device, function);
                class = get_pci_device_class(bus, device, function);
                subclass = get_pci_device_subclass(bus, device, function);
                header_type = get_pci_device_header_type(bus, device, function);
                interrupt = get_pci_device_interrupt(bus, device, function);

                _device = malloc(sizeof(struct pci_device));
                memset(_device, 0x0, sizeof(struct pci_device));
                _device->bus = bus;
                _device->device = device;
                _device->function = function;
                _device->vendor_id = vendor_id;
                _device->device_id = device_id;
                _device->class = class;
                _device->subclass = subclass;
                _device->header_type = header_type;
                _device->irq_line = interrupt & 0xff;
                _device->irq_pin = (interrupt >> 8) & 0xff;
                for(bar = 0; bar < 6; bar++) {
                    _device->bar[bar] = get_pci_bar(_device, bar);
                    _device->bar_size[bar] = get_pci_bar_size(_device, bar); 
                }
                list_append(&pci_device_list, &_device->list);
            }
        }
    }
}

void
dump_pci_devices(void)
{
    uint8_t bar;
    struct list_elem * _list;
    struct pci_device * _device;
    LIST_FOREACH_START(&pci_device_list, _list) {
        _device = CONTAINER_OF(_list, struct pci_device, list);
        LOG_INFO("%x:%x.%x vendor_id:%x device_id:%x class:%x "
            "subclass:%x\n       "
            "header_type:%x irq_pin:%x irq_line:%x\n",
            _device->bus,
            _device->device,
            _device->function,
            _device->vendor_id,
            _device->device_id,
            _device->class,
            _device->subclass,
            _device->header_type,
            _device->irq_pin,
            _device->irq_line);
        for(bar = 0; bar < 6; bar++) {
            if (!_device->bar[bar])
                continue;
            printk("       bar %d: base:0x%x, size:0x%x\n",
                bar,
                _device->bar[bar],
                _device->bar_size[bar]);
        }
    }
    LIST_FOREACH_END();
}
void
pci_init(void)
{
    list_init(&pci_device_list);
    enumerate_pci_devices();
    dump_pci_devices();
}
