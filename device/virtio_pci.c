/*
 * Copyright (c) 2018 Jie Zheng/
 */

#include <device/include/virtio_pci.h>
#include <x86/include/ioport.h>
#include <kernel/include/printk.h>

void
virtio_dev_set_status(struct pci_device * pdev, uint8_t status)
{
    uint8_t old_status = 0;
    if (status) {
        old_status = inb(pdev->bar_ioaddr[0] + VIRTIO_PCI_STATUS);
    }
    outb(pdev->bar_ioaddr[0] + VIRTIO_PCI_STATUS, status | old_status);
}

void
virtio_dev_reset(struct pci_device * pdev)
{
    virtio_dev_set_status(pdev, VIRTIO_PCI_STATUS_RESET);
}

uint32_t
virtio_dev_negotiate_features(struct pci_device * pdev, uint32_t features)
{
    uint32_t feature_request = 0x0;
    feature_request = inl(pdev->bar_ioaddr[0] + VIRTIO_PCI_HOST_FEATURES);
    LOG_DEBUG("virtio device %x:%x.%x negotiates device feature: 0x%x\n",
        pdev->bus,
        pdev->device,
        pdev->function,
        feature_request);
    feature_request &= features;
    outl(pdev->bar_ioaddr[0] + VIRTIO_PCI_GUEST_FEATURES, feature_request);
    feature_request = inl(pdev->bar_ioaddr[0] + VIRTIO_PCI_GUEST_FEATURES);
    LOG_DEBUG("virtio devcie %x:%x.%x negotiates guest feature: 0x%x\n",
        pdev->bus,
        pdev->device,
        pdev->function,
        feature_request);
    return feature_request;
}
