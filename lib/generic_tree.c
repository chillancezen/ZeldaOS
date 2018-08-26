/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/errorcode.h>
#include <lib/include/generic_tree.h>
#include <kernel/include/printk.h>
#include <lib/include/string.h>

int
add_sibling(struct generic_tree * current, struct generic_tree * node)
{
    struct generic_tree * ptr = current;
    if (!ptr)
        return -ERR_INVALID_ARG;
    for(;ptr->right; ptr=ptr->right);
    node->parent = ptr;
    ptr->right = node;
    /*
     * For the purpose of merging two seperate sub trees
     * do not touch node->left and node->right.
     */
    return OK;
}


int
add_child(struct generic_tree * current, struct generic_tree * node)
{
    if(!current)
        return -ERR_INVALID_ARG;
    if(!current->left) {
        current->left = node;
        node->parent = current;
    } else {
        return add_sibling(current->left, node);
    }
    return OK;
}


#if defined(INLINE_TEST)
void
test_generic_tree(void)
{
    struct generic_tree root;
    struct generic_tree child0;
    struct generic_tree child1;
    struct generic_tree child2;
    struct generic_tree child3;
    struct generic_tree * ptr;
    memset(&root, 0x0, sizeof(struct generic_tree));
    memset(&child0, 0x0, sizeof(struct generic_tree));
    memset(&child1, 0x0, sizeof(struct generic_tree));
    memset(&child2, 0x0, sizeof(struct generic_tree));
    memset(&child3, 0x0, sizeof(struct generic_tree));
    add_child(&root, &child0);
    add_child(&root, &child1);
    add_child(&child1, &child2);
    add_child(&child1, &child3);
    FOREACH_CHILD_NODE_START(&root, ptr) {
        LOG_DEBUG("test_generic_tree1:0x%x\n", ptr);
    }
    FOREACH_CHILD_NODE_END();

    FOREACH_SIBLING_NODE_START(&child1, ptr) {
        LOG_DEBUG("test_generic_tree2:0x%x\n", ptr);
    }
    FOREACH_SIBLING_NODE_END();

    FOREACH_CHILD_NODE_START(&child1, ptr) {
        LOG_DEBUG("test_generic_tree3:0x%x\n", ptr);
    }
    FOREACH_CHILD_NODE_END();
    FOREACH_SIBLING_NODE_START(&child3, ptr) {
        LOG_DEBUG("test_generic_tree4:0x%x\n", ptr);
    }
    FOREACH_SIBLING_NODE_END();
}

#endif
