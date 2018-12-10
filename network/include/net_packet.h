/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _NET_PACKTE_H
#define _NET_PACKTE_H
#include <lib/include/list.h>

/*
 * To simplify packet transmission and reception, here we slpit a page into two
 * net packets, 2048 bytes for each. there is a lot of room for a packet.
 * by doing so:
 *  . we can support baby giant frame or even larger packet
 *  . avoid packet fragment and chaining.
 *  . it's really simple to realize, I love it.
 */
// The packet is always to be aligned with NET_PACKET_TOTAL_SIZE,
// aka Half a page
#define NET_PACKET_TOTAL_SIZE   2048
struct packet {
    struct list_elem list;
    uint32_t packet_paddr;
    // the payload_offset is the offset to the packet itself
    uint32_t payload_offset;
    uint32_t payload_length;
}__attribute__((aligned(4)));

#define PACKET_HEADER_SIZE          ((int32_t)(sizeof(struct packet)))
#define INITIAL_HEADER_ROOM_SIZE    (PACKET_HEADER_SIZE + 0x80)
#define MINIMAL_HEADER_ROOM_SIZE    PACKET_HEADER_SIZE
#define MAX_PAYLOAD_SIZE            (NET_PACKET_TOTAL_SIZE - PACKET_HEADER_SIZE)

#define packet_payload(packet, type) \
    ((type)((packet)->payload_offset + (uint32_t)(packet)))

void
packet_reset(struct packet * pkt);

int32_t
packet_extend_left(struct packet * pkt, int32_t nr_room);

int32_t
packet_extend_right(struct packet * pkt, int32_t nr_room);

int32_t
packet_shrink_left(struct packet * pkt, int32_t nr_room);

int32_t
packet_shrink_right(struct packet * pkt, int32_t nr_room);

struct packet *
get_packet(void);

void
put_packet(struct packet * pkt);

void
net_packet_init(void);
#endif
