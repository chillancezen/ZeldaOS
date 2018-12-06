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


static struct list_elem pci_device_list;
static struct list_elem pci_driver_list;


uint32_t
pci_config_read_dword(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset)
{
    uint32_t addr = 0;
    addr |= bus << 16;
    addr |= (device & 0x1f) << 11;
    addr |= (function & 0x7) << 8;
    addr |= offset & 0xfc;
    addr |= 0x80000000;
    outl(PCI_OCNFIG_ADDRESS, addr);
    return inl(PCI_CONFIG_DATA);
}
uint16_t
pci_config_read_word(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset)
{
    uint32_t dword = pci_config_read_dword(bus, device, function, offset);
    uint16_t word = (dword >> ((offset & 0x3) * 8)) & 0xffff;
    return word;
}

uint8_t
pci_config_read_byte(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset)
{
    uint32_t dword = pci_config_read_dword(bus, device, function, offset);
    uint8_t byte = (dword >> ((offset & 0x3) * 8)) & 0xff;
    return byte;
}

void
pci_config_write_dword(uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t data)
{
    uint32_t addr = 0;
    addr |= bus << 16;
    addr |= (device & 0x1f) << 11;
    addr |= (function & 0x7) << 8;
    addr |= offset & 0xfc;
    addr |= 0x80000000;
    outl(PCI_OCNFIG_ADDRESS, addr);
    outl(PCI_CONFIG_DATA, data);
}

/*
 * all are deprecated below this line
 */
void
enumerate_pci_devices(void)
{
    int bar;
    int bus;
    int device;
    int function;
    uint16_t vendor_id;
    struct pci_device * _device;
    for(bus = 0; bus < 256; bus++) {
        for(device = 0; device < 32; device++) {
            for(function = 0; function < 8; function++) {
                vendor_id = pci_config_read_word(bus,
                    device,
                    function,
                    PCI_DEVICE_VENDOR_ID_OFFSET);
                if (vendor_id == 0xffff)
                    continue;
                _device = malloc(sizeof(struct pci_device));
                memset(_device, 0x0, sizeof(struct pci_device));
                _device->bus = bus;
                _device->device = device;
                _device->function = function;
                for (bar = 0; bar < 6; bar++) {
                    _device->bar[bar] = pci_device_bar(_device, bar);
                    _device->bar_size[bar] = pci_device_bar_size(_device, bar);
                    _device->bar_ioaddr[bar] = _device->bar[bar] & 1 ?
                        _device->bar[bar] & 0xfffffffc :
                        _device->bar[bar] & 0xfffffff0;
                        
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
        LOG_INFO("%x:%x.%x vendor_id:%x device_id:%x status:%x class:%x "
            "subclass:%x\n       "
            "header_type:%x prog_if:%x irq_pin:%x irq_line:%x\n",
            _device->bus,
            _device->device,
            _device->function,
            pci_device_vendor_id(_device),
            pci_device_device_id(_device),
            pci_device_status(_device),
            pci_device_class(_device),
            pci_device_subclass(_device),
            pci_device_header_type(_device),
            pci_device_prog_if(_device),
            pci_device_interrupt_pin(_device),
            pci_device_interrupt_line(_device));
        for(bar = 0; bar < 6; bar++) {
            if (!pci_device_bar(_device, bar))
                continue;
            printk("       bar %d: base:0x%x(%s), size:0x%x\n",
                bar,
                _device->bar_ioaddr[bar],
                _device->bar[bar] & 0x1 ? "io region" : "mem region",
                _device->bar_size[bar]);
        }
    }
    LIST_FOREACH_END();
}
void
register_pci_device_driver(struct pci_driver * driver)
{
    LOG_INFO("Register PCI device driver:%s 0x%x\n",
        driver->name,
        driver);
    list_append(&pci_driver_list, &driver->list);
}

void
pci_init(void)
{
    enumerate_pci_devices();
    dump_pci_devices();
}

__attribute__((constructor)) void
pci_pre_init(void)
{
    list_init(&pci_device_list);
    list_init(&pci_driver_list);
}
static struct pci_driver *
pci_device_probe_driver(struct pci_device * pdev)
{
    struct list_elem * _list = NULL;
    struct pci_driver * pdriver = NULL;
    struct pci_driver * target_driver = NULL;
    uint16_t vendor_id = pci_device_vendor_id(pdev);
    uint16_t device_id = pci_device_device_id(pdev);
    uint8_t class = pci_device_class(pdev);
    uint8_t subclass = pci_device_subclass(pdev);
    LIST_FOREACH_START(&pci_driver_list, _list) {
        pdriver = CONTAINER_OF(_list, struct pci_driver, list);
        if (pdriver->vendor_id == vendor_id &&
            pdriver->device_id == device_id &&
            pdriver->class == class &&
            pdriver->subclass == subclass) {
            target_driver = pdriver;
            break;
        }
    }
    LIST_FOREACH_END();
    return target_driver;
}
void
pci_post_init(void)
{
    struct list_elem * _list = NULL;
    struct pci_device * pdev = NULL;
    struct pci_driver * target_driver = NULL;
    LIST_FOREACH_START(&pci_device_list, _list) {
        pdev = CONTAINER_OF(_list, struct pci_device, list);
        if (pdev->driver)
            continue;
        if (!(target_driver = pci_device_probe_driver(pdev)))
            continue;
        pdev->driver = target_driver;
        LOG_INFO("associate pci device:%x:%x.%x with driver:%s(0x%x)\n",
            pdev->bus,
            pdev->device,
            pdev->function,
            target_driver->name,
            target_driver);
        if (target_driver->attach)
            target_driver->attach(pdev);
    }
    LIST_FOREACH_END();
}
