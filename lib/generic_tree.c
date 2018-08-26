/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <lib/include/errorcode.h>
#include <lib/include/generic_tree.h>

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

