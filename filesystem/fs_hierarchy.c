/*
 * Copyright (c) 2018 Jie Zheng
 */
#include <filesystem/include/fs_hierarchy.h>
#include <lib/include/string.h>
#include <kernel/include/printk.h>
#include <memory/include/malloc.h>

/*
 * Create the vertical directory on the root_node
 * the sematic is `mkdir -p /foo/bar/dummy`
 */

int32_t
hierarchy_create_directory(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr)
{
    int idx = 0;
    struct generic_tree * _node = NULL;
    struct file * _file = NULL;
    int found = 0;
    struct generic_tree * current_node = root_node;
    for (idx = 0; idx < iptr; idx++) {
        // Try to find the directory entry with the same name
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        // Validate the found directory entry
        if (found) {
            if (_file->type != FILE_TYPE_DIR) {
                return -ERR_EXIST;
            }
            current_node = &_file->fs_node;
        } else {
            // The directory does not exist, Try to create one.
            _file = malloc(sizeof(struct file));
            if (!_file) {
                LOG_TRIVIA("Can not allocate file descriptor\n");
                return -ERR_OUT_OF_MEMORY;
            }
            memset(_file, 0x0, sizeof(struct file));
            strcpy(_file->name, splitted_path[idx]);
            _file->type = FILE_TYPE_DIR;
            _file->mode = 0x0;
            _file->priv = NULL;
            ASSERT(!add_sibling(current_node, &_file->fs_node));
            current_node = &_file->fs_node;
        }
        // Prepare for next lower hierarchy
        ASSERT(_file);
        if (!current_node->left) {
            // No child on current node
            struct file * _file_another = malloc(sizeof(struct file));
            if (!_file_another) {
                LOG_TRIVIA("Can not allocate another file descriptor\n");
                return -ERR_OUT_OF_MEMORY;
            }
            memset(_file_another, 0x0, sizeof(struct file));
            _file_another->type = FILE_TYPE_MARK;
            ASSERT(!add_child(current_node, &_file_another->fs_node));
        }
        current_node = current_node->left;
    }
    return OK;
}

int32_t
hierarchy_create_file(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr)
{
    int idx = 0;
    int found = 0;
    struct file * _file = NULL;
    struct generic_tree * _node = NULL;
    struct generic_tree * current_node = root_node;
    if (iptr < 1) {
        return -ERR_INVALID_ARG;
    }
    for (idx = 0; idx < (iptr - 1); idx++) {
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        if (!found) {
            return -ERR_NOT_PRESENT;
        }
        current_node = _file->fs_node.left;
    }
    // Till this, we arrive at the layer of the target file node, we search the
    // target file first.
    found = 0;
    FOREACH_SIBLING_NODE_START(current_node, _node) {
        _file = CONTAINER_OF(_node, struct file, fs_node);
        if (!strcmp(_file->name, splitted_path[iptr - 1])) {
            found = 1;
            break;
        }
    }
    FOREACH_SIBLING_NODE_END();
    if (found) {
        return -ERR_EXIST;
    } else {
        // File not exist, try to create one
        _file = malloc(sizeof(struct file));
        memset(_file, 0x0, sizeof(struct file));
        strcpy(_file->name, splitted_path[iptr -1]);
        _file->type = FILE_TYPE_REGULAR;
        _file->mode = 0x0;
        _file->priv = NULL;
        ASSERT(!add_sibling(current_node, &_file->fs_node));
    }
    return OK;
}

struct file *
hierarchy_search_file(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr)
{
    int idx = 0;
    int found = 0;
    struct file * result = NULL;
    struct file * _file = NULL;
    struct generic_tree * _node = NULL;
    struct generic_tree * current_node = root_node;
    for (idx = 0; idx < iptr; idx++) {
        found = 0;
        FOREACH_SIBLING_NODE_START(current_node, _node) {
            _file = CONTAINER_OF(_node, struct file, fs_node);
            if (!strcmp(_file->name, splitted_path[idx])) {
                found = 1;
                break;
            }
        }
        FOREACH_SIBLING_NODE_END();
        if (!found) {
            _file = NULL;
            break;
        }
        current_node = _file->fs_node.left;
    }
    result = _file;
    return result;
}
/*
 * Delete the file/directory path, all the sub-path will be removed.
 */
int32_t
hierarchy_delete_file(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr,
    void (*per_file_free)(struct file *))
{
    struct list_elem queue = {
        .prev = NULL,
        .next = NULL
    };
    struct list_elem * list;
    struct generic_tree * node;
    struct file * file = hierarchy_search_file(root_node,
        splitted_path,
        iptr);
    if (!file)
        return -ERR_NOT_FOUND;
    generic_delete_node(&file->fs_node);
    list_init(&file->fs_node.list);
    list_append(&queue, &file->fs_node.list);
    
    while(!list_empty(&queue)) {
        list = list_fetch(&queue);
        ASSERT(list);
        node = CONTAINER_OF(list, struct generic_tree, list);
        file = CONTAINER_OF(node, struct file, fs_node);
        if (node->left) {
            list_init(&node->left->list);
            list_append(&queue, &node->left->list);
        }
        if (node->right) {
            list_init(&node->right->list);
            list_append(&queue, &node->right->list);
        }
        ASSERT(per_file_free);
        per_file_free(file);
    }
    return OK;
}
