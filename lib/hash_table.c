/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/hash_table.h>
#include <kernel/include/printk.h>

struct hash_node *
search_hash_node(struct hash_stub * stub,
    void * blob,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob))
{
    struct hash_node * target_node = NULL;
    uint32_t hash_value = hash(blob);
    struct list_elem * head = NULL;
    struct hash_node * _node = NULL;
    hash_value = hash_value & stub->stub_mask;
    head = &stub->heads[hash_value];
    LIST_FOREACH_START(head, _node) {
        if (identity(_node, blob)) {
            target_node = _node;
            break;
        }
    }
    LIST_FOREACH_END();
    return target_node;
}

int32_t
add_hash_node(struct hash_stub * stub,
    void * blob,
    struct hash_node * node,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob))
{
    uint32_t hash_value;
    struct hash_node * target_node = NULL;
    struct list_elem * head = NULL;
    ASSERT(identity(node, blob));
    target_node = search_hash_node(stub, blob, hash, identity);
    if (target_node)
        return -ERR_EXIST;
    hash_value = hash(blob);
    hash_value = hash_value & stub->stub_mask;
    head = &stub->heads[hash_value];
    list_append(head, node);
    target_node = search_hash_node(stub, blob, hash, identity);
    ASSERT(target_node);
    return OK;
}
int32_t
delete_hash_node(struct hash_stub * stub,
    void * blob,
    uint32_t (*hash)(void * blob),
    uint32_t (*identity)(struct hash_node * node, void * blob))
{
    struct hash_node * target_node = NULL;
    struct list_elem * head = NULL;
    uint32_t hash_value;
    target_node = search_hash_node(stub, blob, hash, identity);
    if (!target_node)
        return -ERR_NOT_PRESENT; 
    hash_value = hash(blob);
    hash_value = hash_value & stub->stub_mask;
    head = &stub->heads[hash_value];
    list_delete(head, target_node);
    ASSERT(list_node_detached(head, target_node));
    target_node = search_hash_node(stub, blob, hash, identity);
    ASSERT(!target_node);
    return OK;
}
#if defined(INLINE_TEST)
#include <lib/include/string.h>
struct dummy_hash_node {
    int32_t id;
    struct hash_node node;
};
static uint32_t
dummy_hash(void * id)
{
    return (uint32_t)id;
}
static uint32_t
dummy_identity(struct hash_node * node, void * blob)
{
    struct dummy_hash_node * dummy = NULL;
    dummy = CONTAINER_OF(node, struct dummy_hash_node, node);
    return dummy->id == (int32_t)blob;
}
__attribute__((constructor)) void
hash_table_test(void)
{
    struct list_elem heads[1024];
    struct hash_stub stub;
    memset(heads, 0x0, sizeof(heads));
    memset(&stub, 0x0, sizeof(stub));
    stub.stub_mask = 1023;
    stub.heads = heads;

    {
        struct dummy_hash_node node0, node1;
        struct dummy_hash_node * dummy = NULL;
        struct hash_node * node = search_hash_node(&stub, (void *)0x1,
            dummy_hash,
            dummy_identity);
        ASSERT(!node);

        node0.id = 1;
        list_init(&node0.node);
        ASSERT(OK == add_hash_node(&stub, (void *)node0.id, &node0.node,
            dummy_hash,
            dummy_identity));

        node = search_hash_node(&stub, (void *)0x1, dummy_hash, dummy_identity);
        ASSERT(node);
        dummy = CONTAINER_OF(node, struct dummy_hash_node, node);
        ASSERT(dummy->id == 0x1);

        node1.id = 1025;
        list_init(&node1.node);
        ASSERT(OK == add_hash_node(&stub, (void *)node1.id, &node1.node,
            dummy_hash,
            dummy_identity));
        node = search_hash_node(&stub, (void *)0x1, dummy_hash, dummy_identity);
        ASSERT(node);
        node = search_hash_node(&stub, (void *)1025,
            dummy_hash, dummy_identity);
        ASSERT(node);
        ASSERT(delete_hash_node(&stub, (void *)2, dummy_hash, dummy_identity));
        ASSERT(!delete_hash_node(&stub, (void *)1, dummy_hash, dummy_identity));
        node = search_hash_node(&stub, (void *)0x1, dummy_hash, dummy_identity);
        ASSERT(!node);
        ASSERT(!delete_hash_node(&stub,
            (void *)1025, dummy_hash, dummy_identity));
        node = search_hash_node(&stub,
            (void *)1025, dummy_hash, dummy_identity);
        ASSERT(!node);
        ASSERT(delete_hash_node(&stub,
            (void *)1025, dummy_hash, dummy_identity));
    }
}
#endif
