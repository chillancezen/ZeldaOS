/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <kernel/include/work_queue.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>

static uint8_t eth_dev_rx_mask[MAX_NR_ETHERNET_DEVCIES];

static void
ethernet_do_rx(void * blob)
{
    printk("ethernet_do_rx\n");
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
