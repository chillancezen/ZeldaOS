/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _VIRTIO_H
#define _VIRTIO_H
#include <device/include/virtio_pci.h>

#define VIRTIO_NET_F_CSUM           (1 << 0) // Device is able to check partial sum
#define VIRTIO_NET_F_GUEST_CSUM     (1 << 1) // DRIVER negotiates to use CSUM
#define VIRTIO_NET_F_GUEST_OFFLOADS (1 << 2) // Control channel offloads reconfig
#define VIRTIO_NET_F_MAC            (1 << 5) // Device has given the MAC address
#define VIRTIO_NET_F_GUEST_TSO4     (1 << 7) // driver will utilize TSOv4 offload
#define VIRTIO_NET_F_GUEST_TSO6     (1 << 8) // ....................TSOv6........
#define VIRTIO_NET_F_GUEST_ECN      (1 << 9) // Driver can receive TSO with ECN
#define VIRTIO_NET_F_GUEST_UFO      (1 << 10)// Driver can receive UFO
#define VIRTIO_NET_F_HOST_TSO4      (1 << 11)
#define VIRTIO_NET_F_HOST_TSO6      (1 << 12)
#define VIRTIO_NET_F_HOST_ECN       (1 << 13)
#define VIRTIO_NET_F_HOST_UFO       (1 << 14)
#define VIRTIO_NET_F_MRG_RXBUF      (1 << 15)
#define VIRTIO_NET_F_STATUS         (1 << 16)
#define VIRTIO_NET_F_CTRL_VQ        (1 << 17)
#define VIRTIO_NET_F_CTRL_RX        (1 << 18)
#define VIRTIO_NET_F_CTRL_VLAN      (1 << 19)
#define VIRTIO_NET_F_GUEST_ANNOUNCE (1 << 21)
#define VIRTIO_NET_F_MQ             (1 << 22)
#define VIRTIO_NET_F_CTRL_MAC_ADDR  (1 << 23)

#define VIRTIO_NET_DEVICE_CONFIG_STATUS_OFFSET  (VIRTIO_PCI_DEVICE_CONFIG + 6)
#define VIRTIO_NET_DEVICE_CONFIG_NR_VQ_OFFSET   (VIRTIO_PCI_DEVICE_CONFIG + 8)
#define MAX_VIRTIO_NET_VQ_PAIRS     4
 
struct virtio_net {
    uint32_t feature;
    uint8_t mac[6];

    // cache of status and max_vq_pairs
    #define VIRTIO_NET_S_LINK_UP    0x1
    #define VIRTIO_NET_S_ANNOUNCE   0x2
    uint16_t status;
    uint16_t max_vq_pairs;

    struct {
        uint32_t queue_size;
        // note this linear address `queue_vaddr` is not physical address 
        uint32_t queue_vaddr;
        uint32_t nr_pages;
    } virtqueues[MAX_VIRTIO_NET_VQ_PAIRS];
};
void
virtio_net_init(void);
#endif
