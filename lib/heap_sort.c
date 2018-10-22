/*
 * Copyright (c) 2018 Jie Zheng
 */

#include <lib/include/heap_sort.h>
#include <kernel/include/printk.h>
/*
 * FIXME: Find a fast way to search last node and last parent
 * , for example, always to store last node and parent. O(n) really degrade the
 * performance to search them both.
 */
struct binary_tree_node *
search_last_node(struct binary_tree_node * root)
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
        target_node = _node;
        if (_node->left) {
            list_init(&_node->left->list);
            list_append(&queue, &_node->left->list);
        }
        if (_node->right) {
            list_init(&_node->right->list);
            list_append(&queue, &_node->right->list);
        }
    }
    ASSERT(target_node);
    ASSERT(!target_node->left);
    ASSERT(!target_node->right);
    return target_node;
}
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

void
adjust_heap_node(struct binary_tree_node ** proot,
    struct binary_tree_node * node,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *))
{
    int32_t left_is_smaller;
    int32_t right_is_smaller;
    struct binary_tree_node * current_node = node;
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
        } else if (left_is_smaller) {
            swap_nodes(proot, current_node, current_node->left);
            ASSERT(current_node->parent);
        } else {
            break;
        }
    }
}
struct binary_tree_node *
detach_heap_node(struct binary_tree_node ** proot,
    int32_t (*compare)(struct binary_tree_node *, struct binary_tree_node *))
{
    struct binary_tree_node * detached_node = NULL;
    struct binary_tree_node * last_node = NULL;
    if (!*proot)
        return NULL;
    last_node = search_last_node(*proot);
    ASSERT(!last_node->left && !last_node->right);
    
    {
        // Detach the last node
        if (!last_node->parent) {
            ASSERT(*proot == last_node);
            *proot = NULL;
        } else if (last_node->parent->left == last_node) {
            ASSERT(!last_node->parent->right);
            last_node->parent->left = NULL;
            last_node->parent = NULL;
        } else {
            ASSERT(last_node->parent->right == last_node);
            last_node->parent->right = NULL;
            last_node->parent = NULL;
        }
    }
    {
        // Swap the last node with the heap top element
        // Note the heap top may be NULL
        if (!*proot) {
            detached_node = last_node;
        } else {
            detached_node = *proot;
            last_node->left = detached_node->left;
            last_node->right = detached_node->right;
            *proot = last_node;
            detached_node->left = NULL;
            detached_node->right = NULL;
            ASSERT(!detached_node->parent);
        }
    }
    adjust_heap_node(proot, *proot, compare);
    return detached_node;
}

void
delete_heap_node(struct binary_tree_node ** proot,
    struct binary_tree_node * node)
{
    // Make sure `node` is in the heap tree
    struct binary_tree_node * current_node = node;
    for (; current_node && current_node->parent; current_node = current_node->parent);
    if (*proot != node)
        return;
                
}

#if defined(INLINE_TEST)
#include <lib/include/string.h>

struct dummy_node {
    int val;
    struct binary_tree_node node;
};

int32_t
compare(struct binary_tree_node * node0, struct binary_tree_node * node1)
{
    struct dummy_node * dummy0 = CONTAINER_OF(node0, struct dummy_node, node);
    struct dummy_node * dummy1 = CONTAINER_OF(node1, struct dummy_node, node);
    return dummy0->val - dummy1->val;
}

void
heap_sort_test(void)
{
    int32_t last_val = -1;
    struct binary_tree_node * current_node;
    struct dummy_node * dummy = NULL;
    struct binary_tree_node * root = NULL;
    struct dummy_node node0;
    struct dummy_node node1;
    struct dummy_node node2;
    struct dummy_node node3;
    struct dummy_node node4;
    printk("0:%x\n", &node0.node);
    printk("1:%x\n", &node1.node);
    printk("2:%x\n", &node2.node);
    printk("3:%x\n", &node3.node);
    printk("4:%x\n", &node4.node);
    memset(&node0, 0x0, sizeof(struct dummy_node));
    memset(&node1, 0x0, sizeof(struct dummy_node));
    memset(&node2, 0x0, sizeof(struct dummy_node));
    memset(&node3, 0x0, sizeof(struct dummy_node));
    memset(&node4, 0x0, sizeof(struct dummy_node));
    node0.val = 0x10;
    node1.val = 0x11;
    node2.val = 0x5;
    node3.val = 0x2;
    node4.val = 0x11;
    attach_heap_node(&root, &node1.node, compare);
    attach_heap_node(&root, &node0.node, compare);
    attach_heap_node(&root, &node2.node, compare);
    attach_heap_node(&root, &node3.node, compare);
    attach_heap_node(&root, &node4.node, compare);
    while ((current_node = detach_heap_node(&root, compare))) {
        dummy = CONTAINER_OF(current_node, struct dummy_node, node);
        ASSERT(last_val <= dummy->val);
        printk("current val:%d\n", dummy->val);
        last_val = dummy->val;
    }
}
#endif
