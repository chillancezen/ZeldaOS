/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _BRIDGE_H
#define _BRIDGE_H
#include <kernel/include/work_queue.h>
#include <network/include/net_packet.h>

struct switch_port;
struct switch_port_operation {
    uint32_t (*port_xmit)(struct switch_port * port,
        struct packet **pkts,
        int32_t nr_port);

    uint32_t (*port_receive)(struct switch_port * port,
        struct packet **pkts,
        int32_t nr_port);
};

struct switch_port {
    uint16_t port_id;
    struct switch_port_operation * port_ops;
};

struct ethernet_switch;
struct ethernet_switch_operation {
    uint32_t (*add_port)(struct ethernet_switch * etherswitch);
};
struct ethernet_switch {
    uint16_t switch_id;
    struct ethernet_switch_operation * switch_ops;
};
#endif
