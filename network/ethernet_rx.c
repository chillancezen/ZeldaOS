/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/work_queue.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>
#include <network/include/ethernet.h>

static uint8_t eth_dev_rx_mask[MAX_NR_ETHERNET_DEVCIES];

// This is to dispatch the packets to L2/L3/Userland backlog queue
static void
__do_ethernet_dev_receive(struct ethernet_device * eth_dev)
{
#define ONESHOT_RECEIVE_LENGTH  64
    struct packet * pkts[ONESHOT_RECEIVE_LENGTH];
    int32_t nr_recv = 0;
    int idx = 0;
    while ((nr_recv = eth_dev->net_ops->receive(eth_dev,
            pkts,
            ONESHOT_RECEIVE_LENGTH))) {
        for (idx = 0; idx < nr_recv; idx++)
            put_packet(pkts[idx]);
        printk("%d  %d\n", eth_dev->device_index, nr_recv);
    }
}

static void
ethernet_do_rx(void * blob)
{
    int idx = 0;
    struct ethernet_device * eth_dev = NULL;
    for (idx = 0; idx < MAX_NR_ETHERNET_DEVCIES; idx++) {
        if (!eth_dev_rx_mask[idx])
            continue;
        eth_dev = search_ethernet_device_by_id(idx);
        if (!eth_dev) {
            eth_dev_rx_mask[idx] = 0;
            continue;
        }
        __do_ethernet_dev_receive(eth_dev);
    }
}

static struct work_queue netdev_rx_wq = {
    .to_terminate = 0,
    .blob = NULL,
    .do_work = ethernet_do_rx, 
};

void
netdev_rx(int32_t device_index)
{
    ASSERT(device_index >= 0 && device_index < MAX_NR_ETHERNET_DEVCIES);
    eth_dev_rx_mask[device_index] = 1;
    ASSERT(!notify_work_queue(&netdev_rx_wq));
}

void
ethernet_rx_post_init(void)
{
    memset(eth_dev_rx_mask, 0x0, sizeof(eth_dev_rx_mask));
    initialize_wait_queue_head(&netdev_rx_wq.wq_head);
    ASSERT(!register_work_queue(&netdev_rx_wq, (uint8_t *)"ethernet_rx"));
}

