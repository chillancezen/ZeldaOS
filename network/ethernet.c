/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <network/include/ethernet.h>
#include <memory/include/malloc.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>

static struct ethernet_device * ether_devs[MAX_NR_ETHERNET_DEVCIES];

struct ethernet_device *
search_ethernet_device_by_id(int device_id)
{
    return (device_id < 0 || device_id >= MAX_NR_ETHERNET_DEVCIES) ?
        NULL :
        ether_devs[device_id];
}

static struct file_operation net_dev_file_ops = {
    .size = NULL,
    .stat = NULL,
    .read = NULL,
    .write = NULL,
    .truncate = NULL,
    .ioctl = NULL,
};

uint32_t register_ethernet_device(uint8_t * name,
    struct ethernet_device_operation * net_ops,
    void * priv)
{
    int device_index = -1;
    int idx = 0;
    struct ethernet_device * ethdev = NULL;
    struct file_system * dev_fs = get_dev_filesystem();
    uint8_t dev_file_name[128];
    memset(dev_file_name, 0x0, sizeof(dev_file_name));
    sprintf((char *)dev_file_name, "/net/%s", name);
    for(idx = 0; idx < MAX_NR_ETHERNET_DEVCIES; idx++) {
        if (ether_devs[idx] && !strcmp(ether_devs[idx]->name, name))
            return -ERR_INVALID_ARG;
        if (!ether_devs[idx] && device_index < 0) {
            device_index = idx;
        }
    }
    if (device_index < 0) {
        return -ERR_OUT_OF_RESOURCE;
    }
    ASSERT((ethdev = malloc_mapped(sizeof(struct ethernet_device))));
    memset(ethdev, 0x0, sizeof(struct ethernet_device));
    strcpy_safe(ethdev->name, name, sizeof(ethdev->name));
    ethdev->device_index = device_index;
    ethdev->net_ops = net_ops;
    ethdev->priv = priv;
    ethdev->net_dev_file = register_dev_node(dev_fs,
        dev_file_name,
        0x0,
        &net_dev_file_ops,
        ethdev);
    ASSERT(ethdev->net_dev_file);
    LOG_INFO("Register ethernet device: %s [ops:0x%x] as port %d\n",
        name, net_ops, device_index);
    return device_index;
}

__attribute__((constructor)) void
ethernet_device_pre_init(void)
{
    memset(ether_devs, 0x0, sizeof(ether_devs));
}
