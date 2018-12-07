/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <network/include/virtio.h>
#include <kernel/include/printk.h>
#include <x86/include/ioport.h>
#include <memory/include/malloc.h>
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <lib/include/string.h>
#define VIRTIO_SIZE_ROUND(a) (((a) + PAGE_MASK) & ~PAGE_MASK)

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
#if 0
    netdev->max_vq_pairs = inw(pdev->bar_ioaddr[0] +
        VIRTIO_NET_DEVICE_CONFIG_NR_VQ_OFFSET);
    if (((int16_t)netdev->max_vq_pairs) < 0)
#else
        netdev->max_vq_pairs = 1;
#endif
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
    uint8_t vma_mapping_name[64];
    uint32_t queue_size;
    uint32_t size_virt_desc = 0;
    uint32_t size_queue_avail = 0;
    uint32_t size_queue_used = 0;
    uint32_t size_virtq = 0;
    uint32_t virt_queue_base = 0;
    uint32_t phy_queue_base = 0;
    struct virtio_net * netdev = (struct virtio_net * )pdev->priv;
    ASSERT((netdev->max_vq_pairs * 2 + 1) > vq_index);
    outw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_SELECTOR, vq_index);
    queue_size = inw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_SIZE);
    size_virt_desc = sizeof(struct virtq_desc) * queue_size;
    size_queue_avail = sizeof(uint16_t) * (queue_size + 3);
    size_queue_used = sizeof(struct virt_used_elem) * queue_size +
        sizeof(uint16_t) * 3;
    size_virtq = VIRTIO_SIZE_ROUND(size_virt_desc + size_queue_avail);
    size_virtq += VIRTIO_SIZE_ROUND(size_queue_used);
    {
        ASSERT(size_virtq == VIRTIO_SIZE_ROUND(size_virtq));
        memset(vma_mapping_name, 0x0, sizeof(vma_mapping_name));
        sprintf((char *)vma_mapping_name, "virtqueue-%x:%x.%x-%d",
            pdev->bus,
            pdev->device,
            pdev->function,
            vq_index);
        ASSERT((phy_queue_base = get_pages(size_virtq >> 12)));
        ASSERT((virt_queue_base = kernel_map_vma(vma_mapping_name,
            1,
            1,
            phy_queue_base,
            size_virtq,
            PAGE_PERMISSION_READ_WRITE,
            PAGE_WRITEBACK,
            PAGE_CACHE_ENABLED)));
        memset((void *)virt_queue_base, 0x0, size_virtq);
        netdev->virtqueues[vq_index].queue_size = queue_size;
        netdev->virtqueues[vq_index].queue_vaddr = virt_queue_base;
        netdev->virtqueues[vq_index].queue_paddr = phy_queue_base;
        netdev->virtqueues[vq_index].nr_pages = size_virtq >> 12;
        {
            uint32_t kernel_page_directory = get_kernel_page_directory();
            ASSERT(virt2phy((uint32_t *)kernel_page_directory,
                netdev->virtqueues[vq_index].queue_vaddr) ==
                netdev->virtqueues[vq_index].queue_paddr);
        }
        LOG_DEBUG("%s setup {queue_size:%d, vaddr:0x%x, paddr:0x%x, "
            "nr_pages:%d}\n",
            vma_mapping_name,
            netdev->virtqueues[vq_index].queue_size,
            netdev->virtqueues[vq_index].queue_vaddr,
            netdev->virtqueues[vq_index].queue_paddr,
            netdev->virtqueues[vq_index].nr_pages);
    }
    outl(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_PFN,
        netdev->virtqueues[vq_index].queue_paddr);
}


#include <memory/include/kernel_vma.h>
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
    }
    printk("virtio-net device:%x\n", virtio_dev_get_status(pdev));
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
