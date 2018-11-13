/*
 * Copyright (c) 2018 Jie Zheng
 */
#ifndef _HASH_TABLE
#define _HASH_TABLE
#include <lib/include/types.h>
#include <lib/include/list.h>

#define hash_node list_elem

struct hash_stub {
    // the `stub_mask` must be power of 2 minus 1
    // this requires the length of heads must be stub_mask + 1
    int stub_mask;
    struct hash_node * heads;
};

struct hash_node *
search_hash_node(struct hash_stub * stub,
    void * blob,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob));

int32_t
add_hash_node(struct hash_stub * stub,
    void * blob,
    struct hash_node * node,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob));

int32_t
delete_hash_node(struct hash_stub * stub,
    void * blob,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob));


#endif
