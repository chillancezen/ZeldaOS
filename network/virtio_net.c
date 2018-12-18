/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <network/include/virtio_net.h>
#include <kernel/include/printk.h>
#include <x86/include/ioport.h>
#include <memory/include/malloc.h>
#include <memory/include/paging.h>
#include <memory/include/kernel_vma.h>
#include <lib/include/string.h>
#include <network/include/net_packet.h>
#include <network/include/ethernet.h>
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
        netdev->virtqueues[vq_index].last_used_idx = 0;
        netdev->virtqueues[vq_index].vq_desc =
            (struct virtq_desc *)virt_queue_base;
        netdev->virtqueues[vq_index].vq_avail =
            (struct virtq_avail *)(virt_queue_base + size_virt_desc);
        netdev->virtqueues[vq_index].vq_used =
            (struct virtq_used *)(virt_queue_base +
                VIRTIO_SIZE_ROUND(size_virt_desc + size_queue_avail));
        {
            // Do paging verification
            uint32_t kernel_page_directory = get_kernel_page_directory();
            int32_t paging_index = 0;
            for (; paging_index < netdev->virtqueues[vq_index].nr_pages;
                paging_index++) {
                ASSERT(virt2phy((uint32_t *)kernel_page_directory,
                    netdev->virtqueues[vq_index].queue_vaddr +
                        paging_index * PAGE_SIZE) ==
                    (netdev->virtqueues[vq_index].queue_paddr +
                        paging_index * PAGE_SIZE));
            }
        }
        LOG_DEBUG("%s setup {queue_size:%d, vaddr:0x%x, paddr:0x%x, "
            "nr_pages:%d}\n",
            vma_mapping_name,
            netdev->virtqueues[vq_index].queue_size,
            netdev->virtqueues[vq_index].queue_vaddr,
            netdev->virtqueues[vq_index].queue_paddr,
            netdev->virtqueues[vq_index].nr_pages);
    }
    outw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_SELECTOR, vq_index);
    outl(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_PFN,
        netdev->virtqueues[vq_index].queue_paddr >> 12);
    {
        // Allocate local ring buffer to store free descriptor.
        // initially, the ring buffer is filled with all descriptors
        int idx = 0;
        uint8_t idx_high;
        uint8_t idx_low;
        ASSERT((netdev->virtqueues[vq_index].free_desc_ring =
            malloc(sizeof(struct ring) + queue_size * 2 + 1)));
        netdev->virtqueues[vq_index].free_desc_ring->ring_size =
            queue_size * 2 + 1;
        ring_reset(netdev->virtqueues[vq_index].free_desc_ring);
        for (idx = 0; idx < queue_size; idx++) {
            idx_high = HIGH_BYTE(idx);
            idx_low = LOW_BYTE(idx);
            ASSERT(ring_enqueue(netdev->virtqueues[vq_index].free_desc_ring,
                idx_low));
            ASSERT(ring_enqueue(netdev->virtqueues[vq_index].free_desc_ring,
                idx_high));
        }
    }

    {
        // Allocate separate descriptor array to store the
        // virtual addresses of a descriptor
        ASSERT((netdev->virtqueues[vq_index].opaque =
            (void **)malloc_mapped(sizeof(uint32_t) * queue_size)));
        memset(netdev->virtqueues[vq_index].opaque,
            0x0,
            sizeof(uint32_t) * queue_size);
    }
}

void
virtio_net_device_virtqueue_disable_interrupt(struct pci_device * pdev,
    int vq_index)
{
    struct virtio_net * netdev = (struct virtio_net *)pdev->priv;
    struct virtq_avail * avail = NULL;
    ASSERT((MAX_VIRTIO_NET_VQ_PAIRS * 2 + 1) > vq_index);
    avail = netdev->virtqueues[vq_index].vq_avail;
    avail->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;
    sfence();
}

void
virtio_net_device_virtqueue_enable_interrupt(struct pci_device * pdev,
    int vq_index)
{
    struct virtio_net * netdev = (struct virtio_net *)pdev->priv;
    struct virtq_avail * avail = NULL;
    ASSERT((MAX_VIRTIO_NET_VQ_PAIRS * 2 + 1) > vq_index);
    avail = netdev->virtqueues[vq_index].vq_avail;
    avail->flags &= ~VIRTQ_AVAIL_F_NO_INTERRUPT;
    sfence();
}
int32_t
virtio_net_device_raw_send(struct pci_device * pdev,
    int vq_index,
    struct virtq_desc * descs,
    void ** opaque,
    int nr_buffer)
{
    int idx = 0;
    int32_t nr_added = 0x0;
    int free_desc_index = 0;
    struct virtq_desc * desc = NULL;
    struct virtq_avail * avail = NULL;
    struct virtq_used * used = NULL;
    struct virtio_net * netdev = NULL;
    int32_t queue_size = 0x0;
    ASSERT((MAX_VIRTIO_NET_VQ_PAIRS * 2 + 1) > vq_index);
    netdev = (struct virtio_net *)pdev->priv;
    queue_size = netdev->virtqueues[vq_index].queue_size;
    avail = netdev->virtqueues[vq_index].vq_avail;
    used = netdev->virtqueues[vq_index].vq_used;
    for (idx = 0; idx < nr_buffer; idx++) {
        if (ring_empty(netdev->virtqueues[vq_index].free_desc_ring))
            break;
        free_desc_index = 0;
        ASSERT(read_ring(netdev->virtqueues[vq_index].free_desc_ring,
            &free_desc_index,
            2) == 2);
        ASSERT(free_desc_index < netdev->virtqueues[vq_index].queue_size &&
            free_desc_index >= 0);
        // Until now, we have a free descriptor, next we place the buffer into
        // the descriptor
        netdev->virtqueues[vq_index].opaque[free_desc_index] = opaque[idx];
        desc = &netdev->virtqueues[vq_index].vq_desc[free_desc_index];
        desc->addr = descs[idx].addr;
        desc->len = descs[idx].len;
        desc->flags = descs[idx].flags;
        desc->next = 0x0;
        avail->ring[(avail->idx + nr_added++) % queue_size] = free_desc_index;
    }
    mfence();
    avail->idx += nr_added;
    if (!(used->flags & VIRTQ_USED_F_NO_NOTIFY)) {
        outw(pdev->bar_ioaddr[0] + VIRTIO_PCI_QUEUE_QUEUE_NOTIFY, vq_index);
    }
    return nr_added;
}

int32_t
virtio_net_device_raw_receive(struct pci_device * pdev,
    int vq_index,
    void ** opaque,
    int32_t * pnr_buffer)
{
    int iptr = 0;
    int  nr_left = (int)*pnr_buffer;
    struct virtio_net * netdev = (struct virtio_net *)pdev->priv;
    //struct virtq_desc * desc = NULL;
    struct virtq_used * used = NULL;
    uint16_t latest_used_index = netdev->virtqueues[vq_index].last_used_idx;
    uint16_t used_elem_index = 0;
    used = netdev->virtqueues[vq_index].vq_used;
    while (nr_left > 0 && latest_used_index != used->idx) {
        used_elem_index =
            latest_used_index % netdev->virtqueues[vq_index].queue_size;
        ASSERT(used->ring[used_elem_index].id <
            netdev->virtqueues[vq_index].queue_size);
        opaque[iptr] =
            netdev->virtqueues[vq_index].opaque[used->ring[used_elem_index].id];
        printk("idx:%d len:%d 0x%x\n", used->ring[used_elem_index].id,
            used->ring[used_elem_index].len,
            opaque[iptr]);

        latest_used_index++;
        iptr += 1; 
        nr_left -= 1;
    }
    netdev->virtqueues[vq_index].last_used_idx = latest_used_index;
    return 0;
}
static uint32_t
virtio_device_interrupt_handler(struct x86_cpustate * cpu, void * blob)
{
    struct pci_device * pdev = blob;
    uint8_t isr = virtio_dev_get_isr(pdev);
    void * packet[64];
    int32_t nr_buff = 64;
    printk("interrupt:%x\n", isr);
    if (isr) {
        virtio_net_device_raw_receive(pdev, 0, packet, &nr_buff);
    }
    return OK;
}
/*
 * This is to supply buffer to virtqueue 0 of virtio-net pci device
 * return actual number of packets which are put into that queue, aka
 * receive buffers.
 */
static uint32_t
virtio_net_dev_supply_packet(struct pci_device * pdev)
{
#define ONESHORT_PACKET_LENGTH  64
    int idx = 0;
    struct packet * pkts[ONESHORT_PACKET_LENGTH];
    struct virtq_desc tmp_descs[ONESHORT_PACKET_LENGTH];
    void * opaque[ONESHORT_PACKET_LENGTH];

    struct virtio_net * virt_netdev = (struct virtio_net *)pdev->priv;
    int nr_packets_to_supply;
    int free_descriptor_num =
        ring_length(virt_netdev->virtqueues[0].free_desc_ring);
    ASSERT(!(free_descriptor_num & 0x1));
    free_descriptor_num = free_descriptor_num >> 1;
    nr_packets_to_supply = MIN(ONESHORT_PACKET_LENGTH, free_descriptor_num);
    ASSERT(nr_packets_to_supply >= 0);
    // No room for new packets
    if (!nr_packets_to_supply)
        return 0;
    for (idx = 0; idx < nr_packets_to_supply; idx++) {
        pkts[idx] = get_packet();
        if (!pkts[idx])
            break;
    }
    nr_packets_to_supply = idx;
    // Not even one packet is available to be allocated, we should not proceed
    if (!nr_packets_to_supply)
        return 0;
    for (idx = 0; idx < nr_packets_to_supply; idx++) {
        ASSERT(!packet_extend_right(pkts[idx],
            virt_netdev->mtu + sizeof(struct virtio_net_hdr)));
        opaque[idx] = pkts[idx];
        tmp_descs[idx].addr =
            pkts[idx]->packet_paddr + pkts[idx]->payload_offset;
        tmp_descs[idx].len = pkts[idx]->payload_length;
        tmp_descs[idx].flags = VIRTQ_DESC_F_WRITE;
        tmp_descs[idx].next = 0;
    }
    ASSERT(nr_packets_to_supply ==
        virtio_net_device_raw_send(pdev,
            0,
            tmp_descs,
            opaque,
            nr_packets_to_supply));
    return nr_packets_to_supply;
}

static uint8_t *
virtio_net_dev_get_mac(struct ethernet_device * eth_dev)
{
    struct pci_device * pdev = eth_dev->priv;
    struct virtio_net * virt_netdev = (struct virtio_net *)pdev->priv;
    return virt_netdev->mac;
}

static uint32_t
virtio_net_dev_start(struct ethernet_device * eth_dev)
{
    struct pci_device * pdev = eth_dev->priv;
    while(virtio_net_dev_supply_packet(pdev));
    virtio_net_device_virtqueue_enable_interrupt(pdev, 0);
    virtio_net_device_virtqueue_enable_interrupt(pdev, 1);
    virtio_net_device_virtqueue_enable_interrupt(pdev, 2);
    return OK;
}

static uint32_t
virtio_net_dev_stop(struct ethernet_device * eth_dev)
{
    struct pci_device * pdev = eth_dev->priv;
    virtio_net_device_virtqueue_disable_interrupt(pdev, 0);
    virtio_net_device_virtqueue_disable_interrupt(pdev, 1);
    virtio_net_device_virtqueue_disable_interrupt(pdev, 2);
    return OK;
}

static struct  ethernet_device_operation virtio_net_dev_ops = {
    .get_mac = virtio_net_dev_get_mac,
    .start = virtio_net_dev_start,
    .stop = virtio_net_dev_stop,
};

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
            //virtio_net_device_virtqueue_disable_interrupt(pdev, queue_index);
        }
    }
    virtio_dev_set_status(pdev, VIRTIO_PCI_STATUS_DRIVER_OK);
    // here we enable interrupt line, and do not use MSI-X signal
    // also register the linked interrupt handler
    pci_device_enable_interrupt_line(pdev);
    register_pci_interrupt_linked_interrupt_handler(pdev,
        virtio_device_interrupt_handler,
        pdev);
    // register ethernet device then.
    {
        uint8_t dev_name[128];
        sprintf((char *)dev_name, "Ethernet:%x:%x.%x",
            pdev->bus,
            pdev->device,
            pdev->function);
        netdev->ifaceid = register_ethernet_device(dev_name,
            &virtio_net_dev_ops,
            pdev);
        ASSERT(netdev->ifaceid >= 0);
        
    }
    // determine the MTU of the device initially
    netdev->mtu = 1514;
    while(virtio_net_dev_supply_packet(pdev));
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
