/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <lib/include/heap_sort.h>
#include <kernel/include/printk.h>
/*
 * Search the last parent in the Bi-Tree
 * node the root is allowed to be NULL in case it's a empty tree.
 * the found node is allowed to have no children
 */
struct binary_tree_node *
search_last_parent(struct binary_tree_node * root)
{
    struct list_elem * _list;
    struct binary_tree_node * _node;
    struct binary_tree_node * target_node = NULL;
    struct list_elem queue;
    list_init(&queue);
    if (!root)
        return NULL;
    list_init(&root->list);
    list_append(&queue, &root->list);
    while ((_list = list_fetch(&queue))) {
        _node = CONTAINER_OF(_list, struct binary_tree_node, list);
        if (!_node->left || !_node->right) {
            if (!_node->left && _node->right) {
                __not_reach();
            }
            target_node = _node;
            break;
        }
        if (_node->left) {
            list_init(&_node->left->list);
            list_append(&queue, &_node->left->list);
        }
        if (_node->right) {
            list_init(&_node->right->list);
            list_append(&queue, &_node->right->list);
        }
    }
    if (target_node) {
        ASSERT(!target_node->right);
    }
    return target_node; 
}

void
swap_nodes(struct binary_tree_node ** proot,
    struct binary_tree_node * parent,
    struct binary_tree_node * child)
{
    int is_left_child = 0;
    struct binary_tree_node * left_of_child = NULL;
    struct binary_tree_node * right_of_child = NULL;
    ASSERT(*proot);
    ASSERT(child && parent);
    ASSERT(child->parent = parent);

    left_of_child = child->left;
    right_of_child = child->right;

    is_left_child = child == parent->left;

    child->parent = parent->parent;
    if (parent->parent) {
        if (parent->parent->left == parent) {
            parent->parent->left = child;
        } else {
            ASSERT(parent->parent->right == parent);
            parent->parent->right = child;
        }
    }

    if (is_left_child) {
        child->left = parent;
        child->right = parent->right;
        child->right->parent = child;
    } else {
        child->right = parent;
        child->left = parent->left;
        child->left->parent = child;
    }
    parent->parent = child;

    parent->left = left_of_child;
    parent->right = right_of_child;
    if (parent->left)
        parent->left->parent = parent;
    if (parent->right)
        parent->right->parent = parent;

    if (*proot == parent)
        *proot = child;
}


void
attach_heap_node(struct binary_tree_node ** proot,
    struct binary_tree_node * node,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *))
{
    int32_t left_is_smaller = 0;
    int32_t right_is_smaller = 0;
    struct binary_tree_node * current_node = NULL;
    struct binary_tree_node * last_parent = NULL;
    ASSERT(!node->parent);
    ASSERT(!node->left);
    ASSERT(!node->right);
    last_parent = search_last_parent(*proot);
    if (!last_parent) {
        ASSERT(!*proot);
        *proot = node;
        return;
    }
    ASSERT(!last_parent->right);
    if (!last_parent->left) {
        last_parent->left = node;
    } else {
        last_parent->right = node;
    }
    node->parent = last_parent;
    // Adjust Heap Tree from Last_parent
    current_node = last_parent;
    while (current_node) {
        left_is_smaller = 0;
        right_is_smaller = 0;
        if (current_node->left &&
            compare(current_node->left, current_node) < 0)
            left_is_smaller = 1;
        if (current_node->right) {
            ASSERT(current_node->left);
            if (left_is_smaller) {
                right_is_smaller = compare(current_node->right,
                    current_node->left) < 0;
            } else {
                right_is_smaller =
                    compare(current_node->right, current_node) < 0;
            }
        }
        if (right_is_smaller) {
            swap_nodes(proot, current_node, current_node->right);
            ASSERT(current_node->parent);
            current_node = current_node->parent->parent;
        } else if (left_is_smaller) {
            swap_nodes(proot, current_node, current_node->left);
            ASSERT(current_node->parent);
            current_node = current_node->parent->parent;
        } else {
            break;
        }
    }
}

struct binary_tree_node *
detach_heap_node(struct binary_tree_node ** proot,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *))
{
    int left_is_smaller = 0;
    struct binary_tree_node * current_node = *proot;
    if (!*proot)
        return NULL;
    while (current_node->left || current_node->right) {
        left_is_smaller = 1;
        ASSERT(current_node->left);
        if (current_node->right)
            left_is_smaller = compare(current_node->left,
                current_node->right) < 0;
        if (left_is_smaller) {
            swap_nodes(proot, current_node, current_node->left);
        } else {
            swap_nodes(proot, current_node, current_node->right);
        }
    }
    ASSERT(current_node);
    ASSERT(!current_node->left && !current_node->right);
    if (current_node->parent) {
        if (current_node->parent->left == current_node)
            current_node->parent->left = NULL;
        else {
            ASSERT(current_node->parent->right == current_node);
            current_node->parent->right = NULL;
        }
        current_node->parent = NULL;
    } else {
        ASSERT(*proot == current_node);
    }
    return current_node;
}
