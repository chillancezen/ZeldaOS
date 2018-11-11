/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _HEAP_SORT_H
#define _HEAP_SORT_H
#include <lib/include/generic_tree.h>
// This is going to construct and maintain Min heap semantically.
// User should realize the logic to compare any two nodes.
// We reuse binary_tree_node data structucture...

struct heap_stub {
    struct binary_tree_node * root;
    struct binary_tree_node * last_parent;
    struct binary_tree_node * last_node;
};

#define heap_node_detached(stub, node) (!(node)->left &&\
    !(node)->right && \
    !(node)->parent && \
    (stub)->root != (node))


struct binary_tree_node *
search_last_parent(struct binary_tree_node * root);

void
swap_nodes(struct heap_stub * heap,
    struct binary_tree_node * parent,
    struct binary_tree_node * child);

void
attach_heap_node(struct heap_stub * heap,
    struct binary_tree_node * node,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *));

struct binary_tree_node *
detach_heap_node(struct heap_stub * heap,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *));

void
delete_heap_node(struct heap_stub * heap,
    struct binary_tree_node * node,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *));

#if defined(INLINE_TEST)
void
heap_sort_test(void);
#endif

#endif
