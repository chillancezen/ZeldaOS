/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _ETHERNET_H
#define _ETHERNET_H
#include <network/include/net_packet.h>
#include <filesystem/include/devfs.h>
#include <lib/include/list.h>
#include <device/include/pci.h>

struct ethernet_device;
struct ethernet_device_operation {
    uint8_t * (*get_mac)(struct ethernet_device * eth_dev);
    uint32_t  (*start)(struct ethernet_device * eth_dev);
    uint32_t  (*stop)(struct ethernet_device * eth_dev);
    uint32_t  (*transmit)(struct ethernet_device * eth_dev,
        struct packet ** pkts,
        int32_t nr_pkt);
    uint32_t (*receive)(struct ethernet_device * eth_dev,
        struct packet ** pkts,
        int32_t nr_pkt);
};

#define KERNEL_STACK_PATH_UNDEFINED  0x0
#define KERNEL_STACK_PATH_BRIDGING  0x1
#define KERNEL_STACK_PATH_ROUTING   0x2
#define KERNEL_STACK_PATH_USERLAND  0x3

struct ethernet_device {
    uint16_t device_index;
    uint8_t name[64];
    // the stack path when a packet is received
    uint8_t kernel_path;
    // L2 bridging related fields
    uint8_t bridge_domain;

    // L3 routing related fields
    uint8_t route_domain;

    // This is the file export to userland
    struct file * net_dev_file;
    // ethernet device operation
    struct ethernet_device_operation * net_ops;
    // the private data which depends on the ethernet backing
    // in Virtio-Net, the priv is `struct pci_device`
    void * priv;
};


uint32_t register_ethernet_device(uint8_t * name,
    struct ethernet_device_operation * net_ops,
    void * priv);

struct ethernet_device *
search_ethernet_device_by_id(int device_id);

void
netdev_rx(int32_t device_index);

void
ethernet_rx_post_init(void);
#endif
