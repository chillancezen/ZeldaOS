/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <network/include/virtio.h>
#include <kernel/include/printk.h>
#include <x86/include/ioport.h>
#include <memory/include/malloc.h>
#include <memory/include/paging.h>
#include <lib/include/string.h>

void
virtio_net_device_fetch_mac(struct pci_device * pdev)
{
    struct virtio_net * netdev = (struct virtio_net *)pdev->priv;
    int idx = 0;
    for (idx = 0; idx < 6; idx++) {
        netdev->mac[idx] = inb(pdev->bar_ioaddr[0] +
            VIRTIO_PCI_DEVICE_CONFIG + idx);
    }
    LOG_DEBUG("virtio network device:%x:%x.%x detected mac address:"
        "%x:%x:%x:%x:%x:%x\n",
        pdev->bus,
        pdev->device,
        pdev->function,
        netdev->mac[0],
        netdev->mac[1],
        netdev->mac[2],
        netdev->mac[3],
        netdev->mac[4],
        netdev->mac[5]);
}
uint16_t
virtio_net_device_fetch_status(struct pci_device * pdev)
{
    struct virtio_net * netdev = (struct virtio_net * )pdev->priv;
    netdev->status = inw(pdev->bar_ioaddr[0] +
        VIRTIO_NET_DEVICE_CONFIG_STATUS_OFFSET);
    LOG_DEBUG("virtio network device:%x:%x.%x status:0x%x\n",
        pdev->bus,
        pdev->device,
        pdev->function,
        netdev->status);
    return netdev->status;
}

uint16_t
virtio_net_device_fetch_max_queues(struct pci_device * pdev)
{
    struct virtio_net * netdev = (struct virtio_net * )pdev->priv;
    netdev->max_vq_pairs = inw(pdev->bar_ioaddr[0] +
        VIRTIO_NET_DEVICE_CONFIG_NR_VQ_OFFSET);
    if (((int16_t)netdev->max_vq_pairs) < 0)
        netdev->max_vq_pairs = 1;
        LOG_DEBUG("virtio network device:%x:%x.%x max virtqueue pairs:0x%x\n",
            pdev->bus,
            pdev->device,
            pdev->function,
            netdev->max_vq_pairs);
    return netdev->max_vq_pairs;
}

uint32_t
virtio_net_device_fetch_feature(struct pci_device * pdev)
{
    struct virtio_net * netdev = (struct virtio_net * )pdev->priv;
    netdev->feature = inl(pdev->bar_ioaddr[0] + VIRTIO_PCI_GUEST_FEATURES);
    LOG_DEBUG("virtio network device:%x:%x.%x feature:0x%x\n",
        pdev->bus,
        pdev->device,
        pdev->function,
        netdev->feature);
    return netdev->feature;
}

void
virtio_net_device_virt_queue_setup(struct pci_device * pdev, int vq_index)
{
    uint32_t queue_size;
    struct virtio_net * netdev = (struct virtio_net * )pdev->priv;
    ASSERT((netdev->max_vq_pairs * 2 + 1) > vq_index);
    outw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_SELECTOR, vq_index);
    queue_size = inw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_SIZE);
    printk("queue %d size:%d\n", vq_index, queue_size);
}
static int
virtio_device_attach(struct pci_device * pdev)
{
    struct virtio_net * netdev = NULL;
    uint8_t revision = pci_device_revision_id(pdev);

    if (revision) {
        LOG_WARN("pci device:%x:%x.%x recoginzed as not supported "
            "virtio device, revision id:%d\n",
            pdev->bus, pdev->device, pdev->function, revision);
        return -ERR_NOT_SUPPORTED;
    }
    // Prepare the private data for the pci device
    netdev = malloc_mapped(sizeof(struct virtio_net));
    ASSERT(netdev);
    memset(netdev, 0x0, sizeof(struct virtio_net));
    pdev->priv = netdev;
    // Reset the device and get ready to initialize the device
    virtio_dev_reset(pdev);
    virtio_dev_set_status(pdev, VIRTIO_PCI_STATUS_ACK);
    virtio_dev_set_status(pdev, VIRTIO_PCI_STATUS_DRIVER);
    // Do feature negotiation
    // Let Host handle partial checksum
    virtio_dev_negotiate_features(pdev,
        VIRTIO_NET_F_CSUM |
        VIRTIO_NET_F_CTRL_VQ |
        VIRTIO_NET_F_CTRL_RX |
        VIRTIO_NET_F_CTRL_VLAN);
    // Read the mac address from device specific space
    virtio_net_device_fetch_mac(pdev);
    virtio_net_device_fetch_max_queues(pdev);
    virtio_net_device_fetch_status(pdev);
    virtio_net_device_fetch_feature(pdev);
    // Do virtqueues setup
    {
        int queue_index = 0;
        for (; queue_index < ((netdev->max_vq_pairs * 2) + 1); queue_index++) {
            virtio_net_device_virt_queue_setup(pdev, queue_index);
        }
        {
            uint32_t * page_ptr = (uint32_t *)get_pages(5);
            printk("pages:%x\n", page_ptr[0]);
        }
    }
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
