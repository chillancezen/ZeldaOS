/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _VIRTIO_PCI_H
#define _VIRTIO_PCI_H
#include <device/include/pci.h>
// Refer to:https://elixir.bootlin.com/linux/v2.6.39.4/source/include/linux/virtio_pci.h#L60


#define VIRTIO_PCI_HOST_FEATURES        0x0     //4 bytes
#define VIRTIO_PCI_GUEST_FEATURES       0x4     //4 bytes
#define VIRTIO_PCI_QUEUE_PFN            0x8     //4 bytes
#define VIRTIO_PCI_QUEUE_SIZE           0xc     //2 bytes
#define VIRTIO_PCI_QUEUE_SELECTOR       0xe     //2 bytes
#define VIRTIO_PCI_QUEUE_QUEUE_NOTIFY   0x10    //2 bytes
#define VIRTIO_PCI_STATUS               0x12    //1 byte
#define VIRTIO_PCI_ISR                  0x13    //1 byte
#define VIRTIO_PCI_DEVICE_CONFIG        0x14

#define VIRTIO_PCI_STATUS_RESET         0x0
#define VIRTIO_PCI_STATUS_ACK           0x1
#define VIRTIO_PCI_STATUS_DRIVER        0x2
#define VIRTIO_PCI_STATUS_DRIVER_OK     0x4
#define VIRTIO_PCI_STATUS_FAIL          0x80 


#define VIRTIO_F_RING_INDIRECT_DESC         (1 << 28)
#define VIRTIO_F_RING_EVENT_IDX             (1 << 29)
void
virtio_dev_set_status(struct pci_device * pdev, uint8_t status);

void
virtio_dev_reset(struct pci_device * pdev);

uint32_t
virtio_dev_negotiate_features(struct pci_device * pdev, uint32_t features);

#endif
