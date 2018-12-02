/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <network/include/virtio.h>
#include <kernel/include/printk.h>

static int
virtio_device_attach(struct pci_device * pdev)
{
    uint8_t cap_offset = 0;
    struct pci_device_capability_space * pcap = NULL;
    printk("virtio device:%x\n", pdev);
    FOREACH_CAPABILITY_START(pdev, pcap, cap_offset) {
        printk("cap offset:%x %x id:%x %x %x\n", cap_offset, __capability_dword,
        pcap->cap_id,
        pcap->cap_next,
        pcap->cap_len);
    }
    FOREACH_CAPABILITY_END();
    return OK;
}

struct pci_driver virtio_net_driver = {
    .name = "virtio-net",
    .vendor_id = 0x1af4,
    .device_id = 0x1000,
    .class = 2,
    .subclass = 0,
    .attach = virtio_device_attach,
    .detach = NULL
};

void
virtio_net_init(void)
{
    register_pci_device_driver(&virtio_net_driver);
}
