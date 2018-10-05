/*
 * Copyright (c) 2018 Jie Zheng
 */

#ifndef _FS_HIERARCHY_H
#define _FS_HIERARCHY_H
#include <filesystem/include/file.h>
#include <filesystem/include/filesystem.h>
#include <filesystem/include/vfs.h>
int32_t
hierarchy_create_directory(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr);

int32_t
hierarchy_create_file(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr);

struct file *
hierarchy_search_file(struct generic_tree * root_node,
    uint8_t ** splitted_path,
    int iptr);

#endif
