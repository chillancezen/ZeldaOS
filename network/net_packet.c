/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <memory/include/malloc.h>
#include <network/include/net_packet.h>
#include <kernel/include/printk.h>
#include <memory/include/paging.h>

static uint32_t packet_pool_base;
static struct list_elem packet_pool_head;

struct packet *
get_packet(void)
{
    struct packet * pkt = NULL;
    if (!list_empty(&packet_pool_head)) {
        struct list_elem * _list = list_fetch(&packet_pool_head);
        ASSERT(_list);
        pkt = CONTAINER_OF(_list, struct packet, list);
        packet_reset(pkt);
    }
    return pkt;
}
/*
 * XXX: even though this function is designed to be reentrant, we'd better not
 * call them more than once for a packet.
 */
void
put_packet(struct packet * pkt)
{
    if (!element_in_list(&packet_pool_head, &pkt->list)) {
        list_append(&packet_pool_head, &pkt->list);
    }
}

void
packet_reset(struct packet * pkt)
{
    uint32_t kernel_page_directory = get_kernel_page_directory();
    pkt->packet_paddr = virt2phy((uint32_t *)kernel_page_directory,
        (uint32_t)pkt);
    pkt->payload_offset = INITIAL_HEADER_ROOM_SIZE;
    pkt->payload_length = 0x0;
}

int32_t
packet_extend_left(struct packet * pkt, int32_t nr_room)
{
    uint32_t target_payload_offset = pkt->payload_offset - nr_room;
    if (target_payload_offset < MINIMAL_HEADER_ROOM_SIZE)
        return -ERR_INVALID_ARG;
    pkt->payload_offset -= nr_room;
    pkt->payload_length += nr_room;
    return OK;
}

int32_t
packet_extend_right(struct packet * pkt, int32_t nr_room)
{
    uint32_t target_payload_length =  pkt->payload_length + nr_room;
    uint32_t target_payload_boundary = pkt->payload_offset +
        target_payload_length;
    if (target_payload_boundary > NET_PACKET_TOTAL_SIZE)
        return -ERR_INVALID_ARG;
    pkt->payload_length += nr_room;
    return OK;
}

int32_t
packet_shrink_left(struct packet * pkt, int32_t nr_room)
{
    if (nr_room > pkt->payload_length)
        return -ERR_INVALID_ARG;
    pkt->payload_offset += nr_room;
    pkt->payload_length -= nr_room;
    return OK;
}

int32_t
packet_shrink_right(struct packet * pkt, int32_t nr_room)
{
    if (nr_room > pkt->payload_length)
        return -ERR_INVALID_ARG;
    pkt->payload_length -=nr_room;
    return OK;
}

static void
packet_pool_init(void)
{
    packet_pool_base = (uint32_t)malloc_align_mapped(
        DEFAULT_NET_PACKET_AMOUNT * NET_PACKET_TOTAL_SIZE,
        PAGE_SIZE);
    ASSERT(packet_pool_base);
    LOG_INFO("net packets pool base:0x%x length:%d\n", packet_pool_base,
        DEFAULT_NET_PACKET_AMOUNT * NET_PACKET_TOTAL_SIZE);
    list_init(&packet_pool_head);
    {
        int idx = 0;
        struct packet * pkt = NULL;
        for (idx = 0; idx < DEFAULT_NET_PACKET_AMOUNT; idx++) {
            pkt =(struct packet *)
                (packet_pool_base + idx * NET_PACKET_TOTAL_SIZE);
            list_init(&pkt->list);
            packet_reset(pkt);
            list_append(&packet_pool_head, &pkt->list);
        }
    }
}

void
net_packet_init(void)
{
    packet_pool_init();
#if defined(INLINE_TEST)
    {
        // packet structure test
        uint8_t blob[4096] __attribute__((aligned(4096)));
        struct packet * pkt = (struct packet *)blob;
        packet_reset(pkt);
        ASSERT(pkt->payload_offset == INITIAL_HEADER_ROOM_SIZE);
        ASSERT(pkt->payload_length == 0);
        ASSERT(packet_extend_right(pkt, NET_PACKET_TOTAL_SIZE));
        ASSERT(pkt->payload_length == 0);
        ASSERT(pkt->payload_offset == INITIAL_HEADER_ROOM_SIZE);
        ASSERT(packet_extend_right(pkt,
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE + 1));
        ASSERT(!packet_extend_right(pkt,
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE));
        ASSERT(pkt->payload_offset == INITIAL_HEADER_ROOM_SIZE);
        ASSERT(pkt->payload_length == 
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE);

        ASSERT(packet_shrink_right(pkt,
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE + 1));
        ASSERT(pkt->payload_length ==
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE);
        ASSERT(!packet_shrink_right(pkt,
            NET_PACKET_TOTAL_SIZE - INITIAL_HEADER_ROOM_SIZE - 1));
        ASSERT(pkt->payload_length == 1);
        ASSERT(!packet_shrink_right(pkt, 1));
        ASSERT(pkt->payload_length == 0);
        ASSERT(pkt->payload_offset == INITIAL_HEADER_ROOM_SIZE);

        ASSERT(packet_extend_left(pkt, 0x81));
        ASSERT(!packet_extend_left(pkt, 0x80));
        ASSERT(pkt->payload_offset = MINIMAL_HEADER_ROOM_SIZE);
        ASSERT(pkt->payload_length == 0x80);
        ASSERT(packet_extend_left(pkt, 0x1));

        ASSERT(packet_shrink_left(pkt, 0x81));
        ASSERT(!packet_shrink_left(pkt, 0x7f));

        ASSERT(pkt->payload_length == 1);
        ASSERT(pkt->payload_offset == (MINIMAL_HEADER_ROOM_SIZE + 0x7f));
        ASSERT(!packet_shrink_left(pkt, 1));
        ASSERT(pkt->payload_length == 0);
        ASSERT(pkt->payload_offset == INITIAL_HEADER_ROOM_SIZE);

        // packet pool test
        {
            int idx = 0;
            struct list_elem tmp_head;
            struct list_elem * _list;
            list_init(&tmp_head);
            for (idx = 0; idx < DEFAULT_NET_PACKET_AMOUNT; idx++) {
                pkt = get_packet();
                ASSERT(pkt);
                list_append(&tmp_head, &pkt->list);
            }
            ASSERT(!get_packet());
            ASSERT(list_empty(&packet_pool_head));
            for (idx = 0; idx < DEFAULT_NET_PACKET_AMOUNT; idx++) {
                _list = list_fetch(&tmp_head);
                ASSERT(_list);
                pkt = CONTAINER_OF(_list, struct packet, list);
                put_packet(pkt);
            }
            ASSERT(!list_fetch(&tmp_head));
            ASSERT(list_empty(&tmp_head));
            ASSERT(!list_empty(&packet_pool_head));
        }
    }
#endif
}
